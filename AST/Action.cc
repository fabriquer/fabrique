/** @file Action.cc    Definition of @ref Action. */
/*
 * Copyright (c) 2013 Jonathan Anderson
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
	const Type *inType = NULL, *outType = NULL;
	if (params)
		for (auto& p : *params)
		{
			const std::string& name = p->getName().name();

			if (name == "in")
				inType = &p->type();

			else if (name == "out")
				outType = &p->type();
		}

	//
	// Build the parameter map.
	//
	static SourceRange nowhere = SourceRange::None;
	const Type *fileListTy = ctx.fileListType();
	UniqPtrVec<Parameter> parameters;

	// If we don't have explicit 'in' or 'out' parameters, generate them.
	if (not inType)
	{
		inType = fileListTy;
		UniqPtr<Identifier> name(new Identifier("in", inType, nowhere));
		parameters.push_back(Take(new Parameter(name, *inType)));
	}

	if (not outType)
	{
		outType = fileListTy;
		UniqPtr<Identifier> name(
			new Identifier("out", outType, nowhere));
		parameters.push_back(Take(new Parameter(name, *outType)));
	}

	// Add all explicit parameters.
	if (params)
		for (auto& p : *params)
			parameters.push_back(std::move(p));

	const FunctionType& type = *ctx.functionType(*inType, *outType);

	return new Action(args, parameters, type, src);
}


Action::Action(UniqPtrVec<Argument>& args, UniqPtrVec<Parameter>& params,
               const FunctionType& ty, const SourceRange& loc)
	: Expression(ty, loc), Callable(params), args(std::move(args))
{
}


void Action::PrettyPrint(Bytestream& out, int indent) const
{
	out
		<< Bytestream::Action << "action"
		<< Bytestream::Operator << "("
		<< Bytestream::Reset
		;

	for (size_t i = 0; i < args.size(); )
	{
		assert(args[i] != NULL);

		out << *args[i];
		if (++i < args.size())
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

	const PtrVec<Parameter> params = parameters();
	if (not parameters().empty())
	{
		out << Bytestream::Operator << " <- ";

		for (size_t i = 0; i < params.size(); )
		{
			assert(params[i] != NULL);

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
		for (auto& a : args)
			a->Accept(v);

		for (auto& p : parameters())
			p->Accept(v);
	}

	v.Leave(*this);
}
