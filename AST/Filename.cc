/** @file Filename.cc    Definition of @ref Filename. */
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

#include "AST/Filename.h"
#include "AST/Type.h"
#include "AST/Visitor.h"
#include "Support/Bytestream.h"

#include <set>

using namespace fabrique::ast;
using std::string;


bool Filename::isStatic() const
{
	for (auto *a : args)
		if (!a->isStatic())
			return false;

	return true;
}

void Filename::PrettyPrint(Bytestream& out, int indent) const
{
	bool explicitFile =
		(args.size() > 0) or (!name->isStatic());

	if (explicitFile)
		out
			<< Bytestream::Action << "file"
			<< Bytestream::Operator << "(";

	out << Bytestream::Filename << *name << Bytestream::Reset;

	for (auto *a : args)
		out
			<< Bytestream::Operator << ", "
			<< Bytestream::Reset << *a;

	if (explicitFile)
		out
			<< Bytestream::Operator << ")"
			<< Bytestream::Reset;
}


void Filename::Accept(Visitor& v) const
{
	v.Enter(*this);

	name->Accept(v);
	for (auto *a : args)
		a->Accept(v);

	v.Leave(*this);
}