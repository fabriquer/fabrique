/** @file plugins/Which.cc   Definition of @ref fabrique::plugins::Which. */
/*
 * Copyright (c) 2014 Jonathan Anderson
 * All rights reserved.
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

#include "DAG/DAGBuilder.h"
#include "DAG/File.h"
#include "DAG/List.h"
#include "DAG/Parameter.h"
#include "Plugin/Registry.h"
#include "Types/FileType.h"
#include "Types/FunctionType.h"
#include "Types/RecordType.h"
#include "Types/TypeContext.h"
#include "Support/PosixError.h"
#include "Support/String.h"
#include "Support/exceptions.h"
#include "Support/os.h"

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
	      const Type& string, const FileType& file, const Type& files,
	      const FunctionType& executable, const FunctionType& generic)
		: Plugin(type, factory), string_(string), file_(file), fileList_(files),
		  executable_(executable), generic_(generic)
	{
	}

	ValuePtr FindExecutable(const ValueMap& /*scope*/, const ValueMap& args,
	                        vector<string> extraPaths, DAGBuilder& builder,
	                        SourceRange src) const;

	ValuePtr FindFile(const ValueMap& /*scope*/, const ValueMap& args,
	                  DAGBuilder& builder, SourceRange src) const;

	const Type& string_;
	const FileType& file_;
	const Type& fileList_;
	const FunctionType& executable_;
	const FunctionType& generic_;
};

static const char Directories[] = "directories";
static const char ExecutableFnName[] = "executable";
static const char FileName[] = "filename";
static const char GenericFnName[] = "generic";


UniqPtr<Plugin> Which::Factory::Instantiate(TypeContext& ctx) const
{
	const SourceRange nowhere = SourceRange::None();

	const Type& string = ctx.stringType();
	const FileType& file = ctx.fileType();
	const Type& files = ctx.listOf(file, nowhere);

	const FunctionType& executable = ctx.functionType(string, file);
	const FunctionType& generic = ctx.functionType({ &string, &files }, file);

	const RecordType& type = ctx.recordType({
		{ ExecutableFnName, executable },
		{ GenericFnName, generic },
	});

	return UniqPtr<Plugin>(
		new Which(*this, type, string, file, files, executable, generic));
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
			t.CheckSubtype(Type::ListOf(t.context().fileType(), src), src);

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
	const SharedPtrVec<Parameter> name = {
		std::make_shared<Parameter>(FileName, string_, ValuePtr()),
	};
	const SharedPtrVec<Parameter> nameAndDirectories = {
		std::make_shared<Parameter>(FileName, string_, ValuePtr()),
		std::make_shared<Parameter>(Directories, fileList_, ValuePtr()),
	};

	ValueMap fields = {
		{
			ExecutableFnName,
			builder.Function(
				std::bind(&Which::FindExecutable,
				          this, _1, _2, extraPaths, _3, _4),
				scope, name, executable_)
		},
		{
			GenericFnName,
			builder.Function(
				std::bind(&Which::FindFile, this, _1, _2, _3, _4),
				scope, nameAndDirectories, generic_)
		},
	};

	return builder.Record(fields);
}


ValuePtr Which::FindFile(const ValueMap& /*scope*/, const ValueMap& args,
                         DAGBuilder& builder, SourceRange src) const
{
	assert(args.size() == 2);
	const string filename = args.find(FileName)->second->str();

	auto list = args.find(Directories)->second->asList();
	assert(list);

	vector<string> directories;
	for (const ValuePtr& v : list->elements())
	{
		auto file = std::dynamic_pointer_cast<File>(v);
		assert(file);

		directories.push_back(file->fullName());
	}

	const string fullName = ::FindFile(filename, directories);
	return builder.File(fullName, ValueMap(), file_, src);
}


ValuePtr Which::FindExecutable(const ValueMap& /*scope*/, const ValueMap& args,
                               vector<string> extraPaths, DAGBuilder& builder,
                               SourceRange src) const
{
	const string filename = args.find(FileName)->second->str();

	return builder.File(fabrique::FindExecutable(filename, extraPaths),
	                    ValueMap(), file_, src);
}


static plugin::Registry::Initializer init(new Which::Factory());

} // plugins namespace
} // fabrique namespace
