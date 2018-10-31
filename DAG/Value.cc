/** @file DAG/Value.cc    Definition of @ref fabrique::dag::Value. */
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

#include "DAG/Value.h"
#include "DAG/File.h"
#include "DAG/Rule.h"
#include "Support/exceptions.h"
#include "Types/Type.h"

using namespace fabrique::dag;
using std::string;
using std::shared_ptr;


Value::Value(const Type& t, const SourceRange& loc)
	: HasSource(loc), Typed(t)
{
}


ValuePtr Value::Negate(const SourceRange& src) const
{
	throw SemanticException("negation unsupported by " + type().name(), src);
}

ValuePtr Value::Not(const SourceRange& opLoc) const
{
	throw SemanticException(
		"logical `not` unsupported by " + type().name(),
		SourceRange(opLoc, source())
	);
}

ValuePtr Value::DivideBy(ValuePtr&) const
{
	throw SemanticException(
		"division unsupported by " + type().name(), source());
}

ValuePtr Value::MultiplyBy(ValuePtr&) const
{
	throw SemanticException(
		"multiplication unsupported by " + type().name(), source());
}

ValuePtr Value::Add(ValuePtr&) const
{
	throw SemanticException(
		"addition unsupported by " + type().name(), source());
}

ValuePtr Value::PrefixWith(ValuePtr&) const
{
	throw SemanticException(
		"prefix operation unsupported by " + type().name(), source());
}

ValuePtr Value::Subtract(ValuePtr&) const
{
	throw SemanticException(
		"subtraction unsupported by " + type().name(), source());
}

ValuePtr Value::And(ValuePtr&) const
{
	throw SemanticException(
		"logial AND unsupported by " + type().name(), source());
}

ValuePtr Value::Or(ValuePtr&) const
{
	throw SemanticException(
		"logial OR unsupported by " + type().name(), source());
}

ValuePtr Value::Xor(ValuePtr&) const
{
	throw SemanticException(
		"logial XOR unsupported by " + type().name(), source());
}

ValuePtr Value::Equals(ValuePtr&) const
{
	throw SemanticException(
		"equivalence test unsupported by " + type().name(), source());
}
