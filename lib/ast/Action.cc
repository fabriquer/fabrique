/** @file AST/Action.cc    Definition of @ref fabrique::ast::Action. */
/*
 * Copyright (c) 2013-2014, 2018 Jonathan Anderson
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

#include <fabrique/names.hh>
#include <fabrique/Bytestream.hh>
#include <fabrique/UniqPtr.h>
#include <fabrique/ast/Action.hh>
#include <fabrique/ast/EvalContext.hh>
#include <fabrique/ast/Visitor.hh>
#include <fabrique/dag/Parameter.hh>
#include <fabrique/dag/Primitive.hh>
#include <fabrique/types/FileType.hh>
#include <fabrique/types/FunctionType.hh>
#include <fabrique/types/SequenceType.hh>
#include <fabrique/types/TypeContext.hh>

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;
using std::string;


typedef std::function<bool (const Type&)> TypePredicate;

template<typename Values>
size_t Count(const Values& values, TypePredicate predicate)
{
	size_t count = 0;

	for (auto& v : values)
	{
		const Type& t = v->type();

		// If we have a list of input files, treat it as "more than one".
		if (t.isOrdered())
		{
			auto& seqType = dynamic_cast<const SequenceType&>(t);
			if (auto *et = seqType.elementType())
			{
				if (predicate(*et))
				{
					count += 2;
				}
			}
		}
		else if (predicate(v->type()))
		{
			count += 1;
		}
	}

	return count;
}


Action::Action(UniqPtr<Arguments> args, UniqPtrVec<Parameter> params, SourceRange src)
	: Expression(src), HasParameters(params), args_(std::move(args))
{
}


void Action::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	out
		<< Bytestream::Action << names::Action
		<< Bytestream::Operator << "("
		<< Bytestream::Reset
		;

	args_->PrettyPrint(out, indent + 1);

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
		args_->Accept(v);

		for (auto &p : parameters())
		{
			p->Accept(v);
		}
	}

	v.Leave(*this);
}

dag::ValuePtr Action::evaluate(EvalContext& ctx) const
{
	string command;
	dag::ValueMap arguments;
	TypeContext &types = ctx.types();

	SemaCheck(args_->size() >= 1, source(), "missing action arguments");

	// The only keyword-less argument to action() is its command.
	SemaCheck(args_->positional().size() <= 1, source(),
		"build actions only take one positional argument");

	for (const UniqPtr<Expression>& arg : args_->positional())
	{
		command = arg->evaluate(ctx)->str();
	}

	for (const UniqPtr<Argument>& arg : args_->keyword())
	{
		std::string str = arg->getValue().evaluate(ctx)->str();

		if (arg->getName().name() == "command")
		{
			SemaCheck(command.empty(), arg->source(), "duplicate command");
			command = str;
		}
		else
		{
			dag::ValuePtr v(
				new dag::String(str, types.stringType(), arg->source())
			);
			arguments.emplace(arg->getName().name(), v);
		}
	}

	SharedPtrVec<dag::Parameter> parameters;
	for (const UniqPtr<Parameter>& p : this->parameters())
	{
		auto param = p->evaluate(ctx);
		SemaCheck(param, param->source(), "failed to evaluate");

		// Ensure that files are properly tagged as input or output.
		FileType::CheckFileTags(param->type(), p->source());

		parameters.emplace_back(param);
	}

	const Type& file = types.fileType();
	const Type& fileList = types.fileListType();

	const FunctionType& type = types.functionType(
		Count(parameters, FileType::isInput) == 1 ? file : fileList,
		Count(parameters, FileType::isOutput) == 1 ? file : fileList);

	return ctx.builder().Rule(command, type, arguments, parameters, source());
}
