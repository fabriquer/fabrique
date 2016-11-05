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

#include "AST/EvalContext.h"
#include "AST/Scope.h"
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
	: types_(types), delegate_(Grammar::get(), types_, errs_),
	  pluginRegistry_(pluginRegistry), pluginLoader_(pluginLoader),
	  srcroot_(srcroot)
{
	currentSubdirectory_.push("");
}


dag::ValueMap Parser::ParseDefinitions(const std::vector<string>& definitions,
                                       ast::EvalContext& evalCtx)
{
	Type::NamedTypeVec args;

	const Scope& noScope = Scope::None(types_);
	dag::ValueMap values;

	for (const string& d : definitions)
	{
		std::istringstream stream(d + ";");
		pegmatite::StreamInput input =
			pegmatite::StreamInput::Create("definition", stream);

		UniqPtr<ast::Value> astVal(delegate_.ParseValue(input, noScope));
		if (not astVal)
			throw UserError("invalid definition '" + d + "'");

		const string name = astVal->name().name();
		dag::ValuePtr v = astVal->evaluate(evalCtx);

		Bytestream::Debug("parser.cli.defines")
			<< Bytestream::Action << "Parsed definition"
			<< Bytestream::Definition << name
			<< Bytestream::Operator << " = "
			<< Bytestream::Reset << *v
			<< Bytestream::Reset << "\n"
			;

		values.emplace(name, v);
	}

	return values;
}


UniqPtr<Scope> Parser::ParseFile(std::istream& input, string name,
                                 StringMap<dag::ValuePtr> builtins,
                                 SourceRange openedFrom)
{
	Bytestream& dbg = Bytestream::Debug("parser.file");
	dbg
		<< Bytestream::Action << "Parsing"
		<< Bytestream::Type << " file"
		<< Bytestream::Operator << " '"
		<< Bytestream::Literal << name
		<< Bytestream::Operator << "'"
		<< Bytestream::Reset << " with "
		<< Bytestream::Definition << "builtins"
		<< Bytestream::Operator << " ["
		<< Bytestream::Reset
		;

	for (auto i : builtins)
		dbg
			<< " "
			<< Bytestream::Definition << i.first
			<< Bytestream::Operator << ":"
			<< i.second->type()
			;

	dbg
		<< Bytestream::Operator << " ]"
		<< Bytestream::Reset << "\n"
		;

	if (not name.empty())
	{
		const string relativeName =
			(name.find(srcroot_) == 0)
			? name.substr(srcroot_.length() + 1)
			: name;

		files_.push_back(relativeName);
	}

	pegmatite::StreamInput file(pegmatite::StreamInput::Create(name, input));

	Scope::Parameters parameters;
	for (auto i : builtins)
	{
		parameters.emplace(i.first, i.second->type());
	}

	return delegate_.Parse(file, types_, parameters);
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
