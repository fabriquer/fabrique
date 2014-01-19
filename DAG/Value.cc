/** @file Value.cc    Definition of @ref Value. */
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

#include "DAG/Value.h"
#include "DAG/File.h"
#include "DAG/Rule.h"
#include "Support/exceptions.h"
#include "Types/Type.h"

using namespace fabrique::dag;
using std::string;
using std::shared_ptr;


Value::Value(const Type& t, const SourceRange& loc)
	: ty(t), loc(loc)
{
}


shared_ptr<Value> Value::Add(shared_ptr<Value>&)
{
	throw SemanticException(
		"addition unsupported by " + ty.name(),
		getSource());
}

shared_ptr<Value> Value::PrefixWith(shared_ptr<Value>&)
{
	throw SemanticException(
		"prefix operation unsupported by " + ty.name(),
		getSource());
}

shared_ptr<Value> Value::ScalarAdd(shared_ptr<Value>&)
{
	throw SemanticException(
		"scalar addition unsupported by " + ty.name(),
		getSource());
}
