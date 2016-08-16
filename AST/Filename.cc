/** @file AST/Filename.cc    Definition of @ref fabrique::ast::Filename. */
/*
 * Copyright (c) 2013, 2016 Jonathan Anderson
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

#include "AST/Argument.h"
#include "AST/Builtins.h"
#include "AST/EvalContext.h"
#include "AST/Filename.h"
#include "AST/Visitor.h"
#include "DAG/File.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/FileType.h"

#include <cassert>
#include <set>

using namespace fabrique;
using namespace fabrique::ast;
using std::string;


Filename::Parser::~Parser()
{
}

bool Filename::Parser::construct(const ParserInput& in, ParserStack& s,
                                 const ParseError& err)
{
	raw_ = in.str();
	assert(not raw_.empty());

	return Expression::Parser::construct(in, s, err);
}

Filename* Filename::Parser::Build(const Scope&, TypeContext& types, Err&)
{
	return new Filename(raw_, types.fileType(), source());
}


dag::ValuePtr Filename::evaluate(EvalContext& ctx) const
{
	assert(ctx.Lookup(ast::Subdirectory));
	string subdir = ctx.Lookup(ast::Subdirectory)->str();

	return ctx.builder().File(subdir, name_, dag::ValueMap(), type(), source());
}

Filename::Filename(string name, const FileType& t, const SourceRange& src)
	: File(UniqPtr<Expression>(), UniqPtrVec<Argument>(), t, src), name_(name)
{
}


void Filename::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out << Bytestream::Literal << name_ << Bytestream::Reset;
}


void Filename::Accept(Visitor& v) const
{
	v.Enter(*this);
	v.Leave(*this);
}
