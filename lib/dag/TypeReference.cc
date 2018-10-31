/**
 * @file DAG/TypeReference.cc
 * Definition of @ref fabrique::dag::TypeReference.
 */
/*
 * Copyright (c) 2015, 2018 Jonathan Anderson
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

#include <fabrique/dag/TypeReference.hh>
#include <fabrique/dag/Visitor.hh>
#include "Support/Bytestream.h"
#include "Types/TypeContext.h"
using namespace fabrique::dag;


TypeReference::~TypeReference() {}


ValuePtr TypeReference::Create(const Type& t, SourceRange src)
{
	return ValuePtr(new TypeReference(t, src));
}


const fabrique::Type& TypeReference::referencedType() const
{
	return referencedType_;
}


void TypeReference::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	referencedType_.PrettyPrint(out, indent);
}


void TypeReference::Accept(Visitor& v) const
{
	v.Visit(*this);
}


TypeReference::TypeReference(const Type& t, SourceRange src)
	: Value(t.context().typeType(), src), referencedType_(t)
{
}
