//! @file  builtins.cc    Definitions of builtin functions
/*
 * Copyright (c) 2018-2019 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed at Memorial University of Newfoundland
 * under the NSERC Discovery program (RGPIN-2015-06048).
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

#include <fabrique/builtins.hh>
#include <fabrique/names.hh>
#include <fabrique/Bytestream.hh>
#include <fabrique/ast/EvalContext.hh>
#include <fabrique/dag/DAGBuilder.hh>
#include <fabrique/dag/File.hh>
#include <fabrique/dag/Parameter.hh>
#include <fabrique/dag/TypeReference.hh>
#include <fabrique/parsing/Parser.hh>
#include <fabrique/plugin/Loader.hh>
#include <fabrique/plugin/Plugin.hh>
#include <fabrique/plugin/Registry.hh>
#include <fabrique/platform/files.hh>
#include <fabrique/types/FileType.hh>
#include <fabrique/types/TypeContext.hh>

#include <fstream>

using namespace fabrique;
using namespace fabrique::builtins;
using namespace fabrique::dag;
using namespace fabrique::platform;
using std::string;


static ValuePtr FieldsImpl(ValueMap arguments, DAGBuilder &b, SourceRange src)
{
	auto v = arguments["value"];
	SemaCheck(v, src, "null value");

	const auto &t = v->type();

	ValueMap fields;
	for (auto i : t.fields())
	{
		fields[i.first] = TypeReference::Create(i.second, src);
	}

	return ValuePtr(b.Record(fields, src));
}

static ValuePtr OpenFileImpl(ValueMap arguments, DAGBuilder &b, SourceRange src)
{
	auto filename = arguments["filename"];
	SemaCheck(filename, src, "missing filename");

	// TODO: look up subdirectory in the call context?
	string subdir;
	if (auto s = arguments[names::Subdirectory])
	{
		subdir = s->str();
	}

	return b.File(subdir, filename->str(), arguments, src);
}

static ValuePtr PrintImpl(ValueMap arguments, DAGBuilder &b, SourceRange src)
{
	Bytestream &out = Bytestream::Stdout();

	auto v = arguments["value"];
	SemaCheck(v, src, "printing nil");

	auto &types = b.typeContext();
	if (v->type().isSubtype(types.stringType()))
	{
		out << Bytestream::Info << v->str();
	}
	else
	{
		v->PrettyPrint(out);
	}

	out << Bytestream::Reset << "\n";

	return v;
}

static ValuePtr StringifyImpl(ValueMap arguments, DAGBuilder &b, SourceRange src)
{
	auto v = arguments["value"];
	SemaCheck(v, src, "null value");

	return b.String(v->str(), src);
}

static ValuePtr TypeImpl(ValueMap arguments, DAGBuilder &, SourceRange src)
{
	auto v = arguments["value"];
	SemaCheck(v, src, "null value");

	return TypeReference::Create(v->type(), src);
}


dag::ValuePtr builtins::Fields(DAGBuilder &b)
{
	TypeContext &types = b.typeContext();
	SharedPtrVec<dag::Parameter> params;
	params.emplace_back(new Parameter("value", types.nilType()));

	return b.Function(FieldsImpl, types.listOf(types.stringType()), params);
}

fabrique::dag::ValuePtr fabrique::builtins::OpenFile(fabrique::dag::DAGBuilder &b)
{
	TypeContext &types = b.typeContext();

	SharedPtrVec<dag::Parameter> params;
	params.emplace_back(new Parameter("filename", types.stringType()));

	return b.Function(OpenFileImpl, types.fileType(), params,
	                  SourceRange::None(), true);
}


static std::shared_ptr<Record>
ImportFile(string filename, string subdir, ValueMap arguments, SourceRange src,
           parsing::Parser &p, ast::EvalContext &eval, Bytestream &dbg)
{
	dbg
		<< Bytestream::Action << "importing "
		<< Bytestream::Type << "file"
		<< Bytestream::Operator << "'"
		<< Bytestream::Literal << filename
		<< Bytestream::Operator << "'"
		<< Bytestream::Reset << " from "
		<< Bytestream::Literal << subdir
		<< Bytestream::Reset << "\n"
		;

	DAGBuilder &b = eval.builder();
	auto sub = b.File(subdir);
	arguments[names::Subdirectory] = sub;

	auto scope = eval.EnterScope(filename);
	scope.DefineReserved(names::Arguments, b.Record(arguments));
	scope.DefineReserved(names::BuildDirectory, sub);
	scope.Define(names::Subdirectory, sub);

	std::ifstream infile(filename.c_str());
	SemaCheck(infile, src, "failed to open '" + filename + "'");

	auto parse = p.ParseFile(infile, filename);
	for (auto &e : parse.errors())
	{
		Bytestream::Stderr() << e << "\n";
	}

	SemaCheck(parse, src, "failed to import '" + filename + "'");

	ValueMap values;
	for (auto &v : parse.ok())
	{
		auto val = eval.Define(*v);
		if (auto &name = v->name())
		{
			values[name->name()] = val;
		}
	}

	return b.Record(values, src);
}


ValuePtr
fabrique::builtins::Import(parsing::Parser &p, plugin::Loader &pluginLoader,
                           string srcroot, ast::EvalContext &eval)
{
	FAB_ASSERT(PathIsAbsolute(srcroot), "srcroot must be an absolute path");

	DAGBuilder &b = eval.builder();
	TypeContext &types = b.typeContext();

	SharedPtrVec<dag::Parameter> params;
	params.emplace_back(new Parameter("module", types.stringType()));

	dag::Function::Evaluator import =
		[&p, &eval, &pluginLoader, srcroot]
		(dag::ValueMap arguments, dag::DAGBuilder &builder, SourceRange src)
	{
		Bytestream &dbg = Bytestream::Debug("module.import");

		auto n = arguments["module"];
		SemaCheck(n, src, "missing module or file name");
		arguments.erase("module");
		const string name = n->str();

		auto s = arguments[names::Subdirectory];
		SemaCheck(s, src, "missing subdir");
		arguments.erase(names::Subdirectory);

		auto currentSubdir = std::dynamic_pointer_cast<dag::File>(s);
		SemaCheck(currentSubdir, src, "subdir is not a File");

		dbg
			<< Bytestream::Action << "importing "
			<< Bytestream::Operator << "'"
			<< Bytestream::Literal << name
			<< Bytestream::Operator << "'"
			<< Bytestream::Reset << " from subdir '"
			<< Bytestream::Literal << *currentSubdir
			<< Bytestream::Reset << "'...\n"
			;

		const string filename = PathIsAbsolute(name)
			? name
			: JoinPath({ srcroot, currentSubdir->str(), name })
			;

		if (PathIsFile(filename))
		{
			const string subdir =
				JoinPath(currentSubdir->str(), DirectoryOf(name));

			return ImportFile(filename, subdir, arguments, src, p, eval, dbg);
		}

		if (PathIsDirectory(filename))
		{
			const string subdir = JoinPath(currentSubdir->str(), name);
			const string fabfile = JoinPath(filename, "fabfile");

			SemaCheck(PathIsFile(fabfile), src,
			          "directory does not contain 'fabfile'");

			return ImportFile(fabfile, subdir, arguments, src, p, eval, dbg);
		}

		auto descriptor = plugin::Registry::get().lookup(name).lock();
		if (not descriptor)
		{
			descriptor = pluginLoader.Load(name).lock();
		}
		SemaCheck(descriptor, n->source(),
			"no such file or plugin ('" + name + "')");

		auto plugin = descriptor->Create(builder, arguments);
		SemaCheck(plugin, src, "failed to create plugin with arguments");

		dbg
			<< Bytestream::Action << "instantiated "
			<< Bytestream::Type << "plugin"
			<< Bytestream::Operator << "'"
			<< Bytestream::Literal << name
			<< Bytestream::Operator << "': "
			<< Bytestream::Reset << plugin->type()
			<< "\n"
			;

		return plugin;
	};

	return b.Function(import, types.nilType(), params,
	                  SourceRange::None(), true);
}


ValuePtr builtins::Print(DAGBuilder &b)
{
	TypeContext &types = b.typeContext();

	SharedPtrVec<dag::Parameter> params;
	params.emplace_back(new Parameter("value", types.nilType()));

	return b.Function(PrintImpl, types.nilType(), params,
	                  SourceRange::None(), true);
}


ValuePtr builtins::Stringify(DAGBuilder &b)
{
	TypeContext &types = b.typeContext();
	SharedPtrVec<dag::Parameter> params;
	params.emplace_back(new Parameter("value", types.nilType()));

	return b.Function(StringifyImpl, types.stringType(), params);
}


dag::ValuePtr builtins::Type(DAGBuilder &b)
{
	TypeContext &types = b.typeContext();
	SharedPtrVec<dag::Parameter> params;
	params.emplace_back(new Parameter("value", types.nilType()));

	return b.Function(TypeImpl, types.typeType(), params);
}
