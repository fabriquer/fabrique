/** @file AST/Callable.cc    Definition of @ref fabrique::ast::Callable. */
/*
 * Copyright (c) 2014 Jonathan Anderson
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

#include "DAG/Callable.h"
#include "DAG/Parameter.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/Type.h"
#include "Types/TypeError.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::dag;
using std::string;
using std::vector;


Callable::Callable(const SharedPtrVec<Parameter>& p)
	: parameters_(p)
{
	for (auto& param : parameters_)
		assert(param);
}

Callable::~Callable()
{
}

const SharedPtrVec<Parameter>& Callable::parameters() const
{
	return parameters_;
}


bool Callable::hasParameterNamed(const std::string& name) const
{
	auto nameMatches = [&name](const std::shared_ptr<Parameter>& p)
	{
		return p->name() == name;
	};

	const auto& p = parameters_;
	return (find_if(p.begin(), p.end(), nameMatches) != p.end());
}


void Callable::CheckArguments(const ValueMap& args,
                              const StringMap<SourceRange>& argLocations,
                              const SourceRange& callLocation) const
{
	for (auto& p : parameters_)
	{
		const string& name = p->name();
		const Type& t = p->type();

		auto i = args.find(name);
		if (i == args.end())
		{
			// Some arguments are optional.
			if (p->defaultValue())
				continue;

			throw SemanticException(
				"missing argument to '" + name + "'", callLocation);
		}

		const ValuePtr& arg = i->second;
		assert(arg);

		if (not arg->type().isSubtype(t))
		{
			const SourceRange& argSource(argLocations.find(name)->second);
			throw WrongTypeException(t, arg->type(), argSource);
		}
	}
}


vector<string>
Callable::NameArguments(const vector<string>& args, SourceRange src) const
{
	vector<string> namedArgs;

	Bytestream& dbg = Bytestream::Debug("parser.callable");

	dbg << "matching arguments:\n ";
	for (auto& a : args)
		dbg << " " << (a.empty() ? "<unnamed>" : a);

	dbg << "\n to parameters:\n ";
	for (auto& p : parameters_)
	{
		assert(p);
		dbg << " "
			<< Bytestream::Definition << p->name()
			<< Bytestream::Operator << ":"
			<< p->type()
			<< Bytestream::Reset
			;
	}

	dbg << "\n";

	bool doneWithPositionalArgs = false;
	auto nextParameter = parameters_.begin();

	for (string argName : args)
	{
		if (argName.empty())
		{
			if (doneWithPositionalArgs)
				throw SyntaxError(
					"positional argument after keywords",
					src);

			if (nextParameter == parameters_.end())
				throw SyntaxError(
					"too many positional arguments", src);

			const Parameter& p = **nextParameter++;
			argName = p.name();
		}
		else
			doneWithPositionalArgs = true;

		namedArgs.push_back(argName);
	}

	return namedArgs;
}
