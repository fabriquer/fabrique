/** @file plugins/Which.cc   Definition of @ref fabrique::plugins::Which. */
/*
 * Copyright (c) 2014, 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme and at Memorial University
 * of Newfoundland under the NSERC Discovery program (RGPIN-2015-06048).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <fabrique/dag/DAGBuilder.hh>
#include <fabrique/dag/File.hh>
#include <fabrique/dag/List.hh>
#include <fabrique/dag/Parameter.hh>
#include <fabrique/platform/files.hh>
#include <fabrique/plugin/Registry.hh>
#include <fabrique/types/FileType.hh>
#include <fabrique/types/FunctionType.hh>
#include <fabrique/types/RecordType.hh>
#include <fabrique/types/TypeContext.hh>
#include "Support/String.h"
#include "Support/exceptions.h"

#include <cassert>
#include <sstream>

#include <errno.h>
#include <stdlib.h>

using namespace fabrique;
using namespace fabrique::dag;
using fabrique::plugin::Plugin;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
using std::shared_ptr;
using std::string;
using std::vector;


//! Get a named argument if it exists (or throw an exception otherwise)
static ValuePtr GetArgument(const ValueMap &args, const string &name);


namespace fabrique {
namespace plugins {

/**
 * Finds files (executables or any other kind of files) in the
 * PATH environment variable.
 */
class Which : public plugin::Plugin
{
	public:
	virtual shared_ptr<dag::Record>
		Create(dag::DAGBuilder&, const ValueMap& args) const override;

	class Factory : public Plugin::Descriptor
	{
		public:
		virtual string name() const override { return "which"; }
		virtual UniqPtr<Plugin> Instantiate(TypeContext&) const override;

	};

	private:
	Which(const Factory& factory, const RecordType& type,
	      const Type& string, const FileType& file, const Type& files)
		: Plugin(type, factory), string_(string), file_(file), fileList_(files)
	{
	}

	ValuePtr FindExecutable(const ValueMap& args, DAGBuilder &builder,
	                        vector<string> extraPaths) const;

	ValuePtr FindFile(const ValueMap& args, DAGBuilder& builder) const;

	const Type& string_;
	const FileType& file_;
	const Type& fileList_;
};

static const char Directories[] = "directories";
static const char ExecutableFnName[] = "executable";
static const char FileName[] = "filename";
static const char GenericFnName[] = "generic";


UniqPtr<Plugin> Which::Factory::Instantiate(TypeContext& ctx) const
{
	const Type& string = ctx.stringType();
	const FileType& file = ctx.fileType();
	const Type& files = ctx.listOf(file);

	const FunctionType& executable = ctx.functionType(string, file);
	const FunctionType& generic = ctx.functionType({ &string, &files }, file);

	const RecordType& type = ctx.recordType({
		{ ExecutableFnName, executable },
		{ GenericFnName, generic },
	});

	return UniqPtr<Plugin>(new Which(*this, type, string, file, files));
}


shared_ptr<Record> Which::Create(DAGBuilder& builder, const ValueMap& args) const
{
	vector<string> extraPaths;

	for (auto a : args)
	{
		if (a.first == "path")
		{
			ValuePtr paths = a.second;
			const Type& t = paths->type();
			SourceRange src = a.second->source();
			t.CheckSubtype(Type::ListOf(t.context().fileType()), src);

			auto list = std::dynamic_pointer_cast<List>(paths);

			for (auto f : *list)
			{
				auto file = std::dynamic_pointer_cast<File>(f);
				extraPaths.push_back(file->fullName());
			}

			continue;
		}

		throw SemanticException("unknown argument", a.second->source());
	}

	const ValueMap scope;
	const SharedPtrVec<Parameter> name = { builder.Param(FileName, string_) };
	const SharedPtrVec<Parameter> nameAndDirectories = {
		builder.Param(FileName, string_),
		builder.Param(Directories, fileList_),
	};

	ValueMap fields = {
		{
			ExecutableFnName,
			builder.Function(
				std::bind(&Which::FindExecutable,
				          this, _1, _2, extraPaths),
				file_, name)
		},
		{
			GenericFnName,
			builder.Function(
				std::bind(&Which::FindFile, this, _1, _2),
				file_, nameAndDirectories)
		},
	};

	return builder.Record(fields);
}


ValuePtr Which::FindFile(const ValueMap& args, DAGBuilder &builder) const
{
	assert(args.size() == 2);
	const string filename = GetArgument(args, FileName)->str();

	auto *list = GetArgument(args, Directories)->asList();
	assert(list);

	vector<string> directories;
	for (const ValuePtr& v : list->elements())
	{
		auto file = std::dynamic_pointer_cast<File>(v);
		assert(file);

		directories.push_back(file->fullName());
	}

	const string fullName = platform::FindFile(filename, directories);
	return builder.File(fullName);
}


ValuePtr Which::FindExecutable(const ValueMap& args, DAGBuilder& builder,
                               vector<string> extraPaths) const
{
	const string filename = GetArgument(args, FileName)->str();

	return builder.File(platform::FindExecutable(filename, extraPaths));
}


static plugin::Registry::Initializer init(new Which::Factory());

} // plugins namespace
} // fabrique namespace


static ValuePtr GetArgument(const ValueMap &args, const string &name)
{
	auto i = args.find(name);
	FAB_ASSERT(i != args.end(), "missing '" + name + "' argument");

	auto filename = i->second;
	FAB_ASSERT(i->second, name + " is null");

	return i->second;
}
