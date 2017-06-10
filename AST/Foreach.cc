/** @file AST/ForeachExpr.cc    Declaration of @ref fabrique::ast::ForeachExpr. */
/*
 * Copyright (c) 2013-2014, 2016 Jonathan Anderson
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
#include "AST/Foreach.h"
#include "AST/Parameter.h"
#include "AST/Value.h"
#include "AST/Visitor.h"
#include "DAG/List.h"
#include "Parsing/ErrorReporter.h"
#include "Support/Bytestream.h"
#include "Types/SequenceType.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


ForeachExpr::ForeachExpr(UniqPtr<Identifier>& loopVar,
                         UniqPtr<TypeReference>& explicitType,
                         UniqPtr<Expression>& sourceValue,
                         UniqPtr<Scope>& scope,
                         UniqPtr<Expression>& body,
                         const Type& type,
                         const SourceRange& source)
	: Expression(type, source),
	  loopVariable_(std::move(loopVar)),
	  explicitType_(std::move(explicitType)),
	  sourceValue_(std::move(sourceValue)),
	  scope_(std::move(scope)),
	  body_(std::move(body))
{
}


void ForeachExpr::PrettyPrint(Bytestream& out, size_t indent) const
{
	out << Bytestream::Operator << "foreach " << Bytestream::Reset;

	loopVariable_->PrettyPrint(out, indent);

	if (explicitType_)
	{
		out << Bytestream::Operator << ":" << Bytestream::Reset;
		explicitType_->PrettyPrint(out, indent);
	}

	out << Bytestream::Operator << " <= ";

	sourceValue_->PrettyPrint(out, indent);

	out << "\n" << std::string(indent + 1, '\t');

	body_->PrettyPrint(out, indent + 1);

	out << Bytestream::Reset;
}


void ForeachExpr::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		loopVariable_->Accept(v);
		sourceValue_->Accept(v);
		body_->Accept(v);
	}

	v.Leave(*this);
}


ForeachExpr::Parser::~Parser()
{
}


ForeachExpr* ForeachExpr::Parser::Build(const Scope& s, TypeContext& t, Err& err)
{
	UniqPtr<Identifier> loopVariable(loopVariable_->Build(s, t, err));
	if (not loopVariable)
		return nullptr;

	UniqPtr<TypeReference> explicitType;
	if (explicitType_)
	{
		explicitType.reset(explicitType_->Build(s, t, err));
		if (not explicitType)
			return nullptr;
	}

	UniqPtr<Expression> sourceValue(sourceValue_->Build(s, t, err));
	if (not sourceValue)
		return nullptr;

	const Type& sourceType = sourceValue->type();
	if (not sourceType.isOrdered())
	{
		err.ReportError("cannot iterate over " + sourceType.str(),
		                sourceValue->source());
		return nullptr;
	}

	const Type& loopVarType =
		explicitType
		? explicitType->referencedType()
		: sourceType[0]
		;

	Type::TypeMap params = { { loopVariable->name(), loopVarType } };
	UniqPtr<Scope> containingScope(Scope::Create(params, t.nilType(), &s));
	UniqPtr<Expression> body(body_->Build(*containingScope, t, err));

	if (not body)
		return nullptr;

	const Type& type = Type::ListOf(body->type(), body->source());

	return new ForeachExpr(loopVariable, explicitType, sourceValue, containingScope,
	                       body, type, source());
}


dag::ValuePtr ForeachExpr::evaluate(EvalContext& ctx) const
{
	SharedPtrVec<dag::Value> values;

	auto target = sourceValue_->evaluate(ctx);
	assert(target->type().isOrdered());
	assert(target->asList());

	//
	// For each input element, put its value in scope as the loop parameter
	// and then evaluate the CompoundExpression.
	//
	const Identifier& loopVar = *loopVariable_;
	for (const dag::ValuePtr& element : *target->asList())
	{
		assert(element->type().isSubtype(loopVar.type()));

		auto scope(ctx.EnterScope("foreach body"));
		scope.set(loopVar.name(), element);

		dag::ValuePtr result = body_->evaluate(ctx);
		assert(result);
		assert(result->type().isSubtype(body_->type()));

		values.push_back(std::move(result));
	}

	return dag::ValuePtr(dag::List::of(values, source(), type().context()));
}
