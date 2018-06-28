/**
 * @file AST/HasParameters.cc
 * Definition of @ref fabrique::ast::HasParameters.
 */
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

#include "AST/Argument.h"
#include "AST/HasParameters.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/Type.h"
#include "Types/TypeError.h"

using namespace fabrique;
using namespace fabrique::ast;
using std::string;
using std::vector;


HasParameters::HasParameters(UniqPtrVec<Parameter>& params)
	: params_(std::move(params))
{
	for (auto& p : params_)
		paramNames_.insert(p->getName().name());
}

const UniqPtrVec<Parameter>& HasParameters::parameters() const
{
	return params_;
}

const std::set<string>& HasParameters::parameterNames() const
{
	return paramNames_;
}

void HasParameters::CheckArguments(const UniqPtrVec<Argument>& args,
                              const SourceRange& src) const
{
	// TODO: check DAG values?
#if 0
	const auto& names = parameterNames();

	for (auto& a : args)
	{
		if (a->hasName())
		{
			const string name(a->getName().name());
			if (find(names.begin(), names.end(), name) == names.end())
				throw SemanticException(
					"invalid parameter", a->source());
		}
	}

	StringMap<const Argument*> namedArguments = NameArguments(args);

	for (auto& p : params_)
	{
		const string& name = p->getName().name();
		const Argument *arg = namedArguments[name];

		if (not arg and not p->defaultValue())
			throw SemanticException(
				"missing argument to '" + name + "'", src);

		if (arg)
			arg->type().CheckSubtype(p->type(), arg->source());
	}
#endif
}


vector<string>
HasParameters::NameArguments(const vector<string>& args, SourceRange src) const
{
	vector<string> namedArgs;

	Bytestream& dbg = Bytestream::Debug("parser.callable");

	dbg << "matching arguments:\n ";
	for (auto& a : args)
		dbg << " " << (a.empty() ? "<unnamed>" : a);

	dbg << "\n to parameters:\n ";
	for (auto& p : params_)
		dbg << " " << *p;

	dbg << "\n";

	bool doneWithPositionalArgs = false;
	ParamIterator nextParameter = params_.begin();

	for (string argName : args)
	{
		if (argName.empty())
		{
			if (doneWithPositionalArgs)
				throw SyntaxError(
					"positional argument after keywords",
					src);

			if (nextParameter == params_.end())
				throw SyntaxError(
					"too many positional arguments", src);

			const Parameter& p = **nextParameter++;
			argName = p.getName().name();
		}
		else
			doneWithPositionalArgs = true;

		namedArgs.push_back(argName);
	}

	return namedArgs;
}
