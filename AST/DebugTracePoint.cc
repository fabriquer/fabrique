/**
 * @file AST/DebugTracePoint.h
 * Definition of @ref fabrique::ast::DebugTracePoint.
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

#include "AST/DebugTracePoint.h"
#include "AST/Visitor.h"
#include "DAG/Primitive.h"
#include "Support/Bytestream.h"
#include "Support/ErrorReport.h"

using namespace fabrique::ast;
using namespace fabrique;

DebugTracePoint::DebugTracePoint(UniqPtr<Expression>& e, SourceRange src)
	: Expression(src), expr_(std::move(e))
{
}


void DebugTracePoint::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	out
		<< Bytestream::Action << "trace"
		<< Bytestream::Operator << "("
		;

	expr_->PrettyPrint(out, indent);

	out
		<< Bytestream::Operator << ")"
		;
}


void DebugTracePoint::Accept(Visitor& v) const
{
	if (v.Enter(*this))
		expr_->Accept(v);

	v.Leave(*this);
}


dag::ValuePtr DebugTracePoint::evaluate(EvalContext& ctx) const
{
	fabrique::dag::ValuePtr value = expr_->evaluate(ctx);

	UniqPtr<ErrorReport> report(
		ErrorReport::Create("debug trace point", source(),
		                    ErrorReport::Severity::Message, 1)
	);

	Bytestream::Debug("trace") << *report << "value: ";
	Bytestream& out = Bytestream::Stdout();

	// Special case: output string primitives just as they are.
	if (auto s = std::dynamic_pointer_cast<const dag::String>(value))
		out << s->str() << "\n";

	else
		out << *value << "\n";

	return value;
}
