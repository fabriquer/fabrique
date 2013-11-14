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

#include <cassert>

using namespace fabrique::dag;
using std::string;
using std::unique_ptr;


Value::Value(const SourceRange& loc) : loc(loc) {}

#if 0
Value::Value(File *f) : ty(Type::File), file(f) {}
Value::Value(unique_ptr<File>&& f) : Value(f.release()) {}

Value::Value(Rule *r) : ty(Type::Rule), rule(r) {}
Value::Value(unique_ptr<Rule>&& r) : Value(r.release()) {}


Value::operator const Primitive& () const
{
	assert(ty == Type::Primitive);
	return p;
}

Value::operator const List& () const
{
	assert(ty == Type::List);
	return l;
}

Value::operator std::shared_ptr<File> () const
{
	assert(ty == Type::File);
	return file;
}

Value::operator std::shared_ptr<Rule> () const
{
	assert(ty == Type::Rule);
	return rule;
}

void Value::PrettyPrint(Bytestream& b, int indent) const
{
	switch (ty)
	{
		case Type::Primitive:
			p.PrettyPrint(b, indent);
			break;

		case Type::List:
			l.PrettyPrint(b, indent);
			break;

		case Type::File:
			file->PrettyPrint(b, indent);
			break;

		case Type::Rule:
			rule->PrettyPrint(b, indent);
			break;
	}
}
#endif
