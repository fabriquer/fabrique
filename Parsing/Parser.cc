/** @file Parsing/Parser.cc    Definition of @ref fabrique::parser::Parser. */
/*
 * Copyright (c) 2013-2014 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
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

#include "Parsing/Grammar.h"
#include "Parsing/Parser.h"
#include "Parsing/ParserDelegate.h"
#include "Plugin/Loader.h"
#include "Plugin/Plugin.h"
#include "Plugin/Registry.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/RecordType.h"
#include "Types/TypeContext.h"
#include "Support/os.h"

#include <cassert>
#include <fstream>
#include <sstream>

using namespace fabrique;
using namespace fabrique::ast;
using namespace std::placeholders;

using fabrique::parser::Grammar;
using fabrique::parser::Parser;
using std::string;
using std::unique_ptr;


Parser::Parser(TypeContext& types, plugin::Registry& pluginRegistry,
               plugin::Loader& pluginLoader, string srcroot)
	: types_(types), pluginRegistry_(pluginRegistry), pluginLoader_(pluginLoader),
	  srcroot_(srcroot)
{
	currentSubdirectory_.push("");
}


const Type& Parser::ParseDefinitions(const std::vector<string>& definitions)
{
	if (definitions_)
		throw UserError("arguments already defined");

	Type::NamedTypeVec args;
	const Type& nil = types_.nilType();

	UniqPtr<Scope> scope { new Scope(nullptr/*, "definitions", nil*/) };

	for (const string& d : definitions)
	{
		std::istringstream input(d + ";");
		UniqPtr<Scope> definitionTree { ParseFile(input, nil) };
		if (not definitionTree)
			throw UserError("invalid definition '" + d + "'");

#if 0
		for (UniqPtr<Value>& value : definitionTree->ReleaseValues())
		{
			const string name = value->name().name();

			if (name == Arguments)
				continue;

			Bytestream::Debug("parser.cli.defines")
				<< Bytestream::Action << "Parsed definition"
				<< Bytestream::Operator << ": "
				<< *value
				<< Bytestream::Reset << "\n"
				;

			args.emplace_back(name, value->type());
			scope->Take(value);
		}
#endif

		definitionTree.reset();
	}

	definitions_.swap(scope);

	return types_.recordType(args);
}


UniqPtr<Scope> Parser::ParseFile(std::istream& input, const Type& args,
                                 string name, StringMap<string> /*builtins*/,
                                 SourceRange /*openedFrom*/)
{
	Bytestream& dbg = Bytestream::Debug("parser.file");
	dbg
		<< Bytestream::Action << "Parsing"
		<< Bytestream::Type << " file"
		<< Bytestream::Operator << " '"
		<< Bytestream::Literal << name
		<< Bytestream::Operator << "'"
		;

	if (args.valid())
	{
		dbg
			<< Bytestream::Reset << " with "
			<< Bytestream::Definition << "args"
			<< Bytestream::Operator << ": "
			;

		for (auto& i : args.fields())
			dbg
				<< Bytestream::Definition << i.first
				<< Bytestream::Operator << ":"
				<< i.second
				;
	}

	dbg << Bytestream::Reset << "\n";

	if (not name.empty())
	{
		const string relativeName =
			(name.find(srcroot_) == 0)
			? name.substr(srcroot_.length() + 1)
			: name;

		files_.push_back(relativeName);
	}

	pegmatite::StreamInput file(pegmatite::StreamInput::Create(name, input));
	ParserDelegate p(Grammar::get(), types_, errs_);

	/*
	unique_ptr<ast::Scope> builtins = p.DefineBuiltins(builtins);
	p.EnterScope(builtins);
	*/
	p.EnterScope(name, args);

	return p.Parse(file);
}



unique_ptr<plugin::Plugin> Parser::FindPlugin(std::string name)
{
	if (auto plugin = pluginRegistry_.lookup(name).lock())
	{
		UniqPtr<plugin::Plugin> instance(plugin->Instantiate(types_));

		Bytestream::Debug("plugin.loader")
			<< Bytestream::Action << "found"
			<< Bytestream::Type << " plugin "
			<< Bytestream::Definition << plugin->name()
			<< Bytestream::Reset << " with type "
			<< instance->type()
			<< "\n"
			;

		return instance;
	}

	if (auto plugin = pluginLoader_.Load(name).lock())
	{
		UniqPtr<plugin::Plugin> instance(plugin->Instantiate(types_));

		Bytestream::Debug("plugin.loader")
			<< Bytestream::Action << "loaded"
			<< Bytestream::Type << " plugin "
			<< Bytestream::Definition << plugin->name()
			<< Bytestream::Reset << " with type "
			<< instance->type()
			<< "\n"
			;

		return instance;
	}

	return unique_ptr<plugin::Plugin>();
}

#if 0
	const string subdir(currentSubdirectory_.top());
	string filename;
	try { filename = FindModule(srcroot_, subdir, name->str()); }
	catch (const UserError& e)
	{
		ReportError(e.message(), src);
		return nullptr;
	}

	const string directory = DirectoryOf(filename);

	currentSubdirectory_.push(directory);

	const string absolute =
		PathIsAbsolute(filename) ? filename : JoinPath(srcroot_, filename);

	dbg
		<< Bytestream::Action << "found"
		<< Bytestream::Type << " module "
		<< Bytestream::Operator << "'"
		<< Bytestream::Literal << filename
		<< Bytestream::Operator << "'"
		<< Bytestream::Reset << " at "
		<< Bytestream::Operator << "'"
		<< Bytestream::Literal << absolute
		<< Bytestream::Operator << "'"
		<< "\n"
		;

	std::ifstream input(absolute);
	if (not input.good())
		throw UserError("Can't open '" + filename + "'");

	Type::NamedTypeVec argTypes;
	for (UniqPtr<Argument>& a : args)
	{
		if (not a->hasName())
		{
			ReportError("import argument must be named",
			            a->source());
			return nullptr;
		}

		argTypes.emplace_back(a->getName().name(), a->type());
	}

	const RecordType& argsType = ctx_.recordType(argTypes);

	UniqPtr<Scope> module = ParseFile(input, argsType, absolute);
	if (not module)
		return nullptr;

	currentSubdirectory_.pop();

	Type::NamedTypeVec fields;
	for (const UniqPtr<Value>& value : module->values())
	{
		const Identifier& id { value->name() };
		if (not id.reservedName())
			fields.emplace_back(id.name(), value->type());
	}
	const RecordType& ty { ctx_.recordType(fields) };

	return new Import(name, args, directory, module, ty, src);
}
#endif
