/** @file AST/Parameter.cc    Definition of @ref fabrique::ast::Parameter. */
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

#include "AST/Parameter.h"
#include "AST/Visitor.h"
#include "Support/Bytestream.h"

using namespace fabrique::ast;


Parameter::Parameter(UniqPtr<Identifier>& name, const Type& resultTy,
                     UniqPtr<Expression>&& defaultValue)
	: Expression(resultTy, SourceRange::Over(name, defaultValue)),
	  name_(std::move(name)), defaultValue_(std::move(defaultValue))
{
}


void Parameter::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out << *name_;

	if (defaultValue_)
		out
			<< Bytestream::Operator << " = "
			<< *defaultValue_
			;
}


void Parameter::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		name_->Accept(v);
		if (defaultValue_)
			defaultValue_->Accept(v);
	}

	v.Leave(*this);
}
