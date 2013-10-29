/** @file Conditional.cc    Definition of @ref Conditional. */
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

#include "AST/Conditional.h"
#include "AST/Visitor.h"
#include "Support/Bytestream.h"

using namespace fabrique::ast;


bool Conditional::isStatic() const
{
	// TODO: if the condition is static, we only need *either* the then
	//       or the else clause to be static too.
	return condition->isStatic()
		and thenResult->isStatic()
		and elseResult->isStatic()
		;
}


void Conditional::PrettyPrint(Bytestream& out, int indent) const
{
	out
		<< Bytestream::Operator << "if ("
		<< *condition
		<< Bytestream::Operator << ") { "
		<< *thenResult
		<< Bytestream::Operator << "} else {"
		<< *elseResult
		<< Bytestream::Operator << "}"
		<< Bytestream::Reset
		;
}


void Conditional::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		condition->Accept(v);
		thenResult->Accept(v);
		elseResult->Accept(v);
	}

	v.Leave(*this);
}
