/** @file AST/FilenameLiteral.cc    Definition of @ref fabrique::ast::FilenameLiteral. */
/*
 * Copyright (c) 2018 Jonathan Anderson
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

#include <fabrique/names.hh>
#include <fabrique/Bytestream.hh>
#include <fabrique/SemanticException.hh>
#include <fabrique/ast/Argument.hh>
#include <fabrique/ast/EvalContext.hh>
#include <fabrique/ast/FilenameLiteral.hh>
#include <fabrique/ast/Visitor.hh>
#include <fabrique/dag/File.hh>

#include <cassert>
#include <set>

using namespace fabrique;
using namespace fabrique::ast;
using std::string;


FilenameLiteral::FilenameLiteral(string name, SourceRange src)
	: Expression(src), name_(name)
{
}


void FilenameLiteral::PrettyPrint(Bytestream &out, unsigned int /*indent*/) const
{
	out << Bytestream::Filename << name_ << Bytestream::Reset;
}

void FilenameLiteral::Accept(Visitor& v) const
{
	v.Enter(*this);
	v.Leave(*this);
}

dag::ValuePtr FilenameLiteral::evaluate(EvalContext& ctx) const
{
	SemaCheck(ctx.Lookup(names::Subdirectory, source()), source(),
	          "subdir not defined");

	string subdirectory = ctx.Lookup(names::Subdirectory, source())->str();

	return ctx.builder().File(subdirectory, name_, {}, source());
}
