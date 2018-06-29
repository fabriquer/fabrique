/**
 * @file AST/TypeDeclaration.cc
 * Definition of @ref fabrique::ast::TypeDeclaration.
 */
/*
 * Copyright (c) 2015 Jonathan Anderson
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
#include "AST/TypeDeclaration.h"
#include "AST/Visitor.h"
#include "DAG/TypeReference.h"
#include "Support/Bytestream.h"
#include "Types/UserType.h"

using namespace fabrique;
using namespace fabrique::ast;


#if 0
TypeDeclaration::TypeDeclaration(const UserType& t, const SourceRange& loc)
	: Expression(t, loc), declaredType_(t)
{
}

void TypeDeclaration::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	out
		<< Bytestream::Definition << "type"
		<< Bytestream::Operator << '['
		;

	auto fields = declaredType_.userType().fields();
	for (auto i = fields.begin(); i != fields.end(); )
	{
		out
			<< Bytestream::Definition << i->first
			<< Bytestream::Operator << ':'
			;

		i->second.PrettyPrint(out, indent);

		i++;
		if (i != fields.end())
			out
				<< Bytestream::Operator << ", "
				<< Bytestream::Reset
				;
	}

	out
		<< Bytestream::Operator << ']'
		<< Bytestream::Reset
		;
}

void TypeDeclaration::Accept(Visitor& v) const
{
	v.Enter(*this);
	v.Leave(*this);
}

dag::ValuePtr TypeDeclaration::evaluate(EvalContext&) const
{
	return dag::ValuePtr(
		dag::TypeReference::Create(declaredType_, type(), source()));
}
#endif
