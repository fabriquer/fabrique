/** @file AST/Action.cc    Definition of @ref fabrique::ast::Action. */
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

#include "ADT/UniqPtr.h"
#include "AST/Action.h"
#include "AST/Visitor.h"
#include "Support/Bytestream.h"
#include "Types/FileType.h"
#include "Types/FunctionType.h"
#include "FabContext.h"

#include <cassert>

using namespace fabrique::ast;
using fabrique::StringMap;
using fabrique::UniqPtrMap;
using fabrique::UniqPtrVec;
using std::string;


Action* Action::Create(UniqPtrVec<Argument>& args,
                       UniqPtr<UniqPtrVec<Parameter>>& params,
                       const SourceRange& src, FabContext& ctx)
{
	//
	// Identify the input and output types for FunctionType.
	//
	int extraOut = 0;
	const Parameter *in = NULL, *out = NULL;

	if (params)
		for (auto i = params->begin(); i != params->end(); i++)
		{
			const Parameter& p = **i;
			const string name = p.getName().name();

			if (name == "in")
				in = i->get();

			else if (name == "out")
				out = i->get();

			else if (p.type().isFile())
			{
				auto& t = dynamic_cast<const FileType&>(p.type());
				if (t.isOutputFile())
					++extraOut;
			}
		}

	//
	// Build the parameter map.
	//
	const static SourceRange& nowhere = SourceRange::None();
	const Type& fileList = ctx.fileListType();
	const bool noExplicitInput = (not params or not in);
	const bool noExplicitOutput = (not params or not out);
	UniqPtrVec<Parameter> parameters;

	// If we don't have explicit 'in' or 'out' parameters, generate them.
	if (noExplicitInput)
	{
		UniqPtr<Identifier> name(new Identifier("in", &fileList, nowhere));
		parameters.push_back(Take(new Parameter(name, fileList)));
		in = (--parameters.end())->get();
	}

	if (noExplicitOutput)
	{
		UniqPtr<Identifier> name(new Identifier("out", &fileList, nowhere));
		parameters.push_back(Take(new Parameter(name, fileList)));
		out = (--parameters.end())->get();
	}


	// Add all explicit parameters.
	if (params)
		for (auto& p : *params)
			parameters.push_back(std::move(p));

	// Create the Action (and its type).
	assert(in);
	assert(out);

	const Type& returnType =
		(extraOut > 0 or noExplicitOutput)
		? fileList
		: out->type();

	const FunctionType& type = ctx.functionType(in->type(), returnType);

	return new Action(args, parameters, type, src);
}


Action::Action(UniqPtrVec<Argument>& a, UniqPtrVec<Parameter>& params,
               const FunctionType& ty, const SourceRange& loc)
	: Expression(ty, loc), Callable(params), args_(std::move(a))
{
}


void Action::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out
		<< Bytestream::Action << "action"
		<< Bytestream::Operator << "("
		<< Bytestream::Reset
		;

	for (size_t i = 0; i < args_.size(); )
	{
		assert(args_[i] != NULL);

		out << *args_[i];
		if (++i < args_.size())
			out << Bytestream::Operator << ", ";
	}

	const UniqPtrVec<Parameter>& params = parameters();
	if (not params.empty())
	{
		out << Bytestream::Operator << " <- ";

		for (size_t i = 0; i < params.size(); )
		{
			out << *params[i];
			if (++i < params.size())
				out << Bytestream::Operator << ", ";
		}
	}

	out
		<< Bytestream::Operator << ")"
		<< Bytestream::Reset
		;
}


void Action::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		for (auto& a : args_)
			a->Accept(v);

		for (auto& p : parameters())
			p->Accept(v);
	}

	v.Leave(*this);
}
