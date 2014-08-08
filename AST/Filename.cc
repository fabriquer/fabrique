/** @file AST/Filename.cc    Definition of @ref fabrique::ast::Filename. */
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

#include "AST/Argument.h"
#include "AST/Filename.h"
#include "AST/Visitor.h"
#include "Support/Bytestream.h"
#include "Types/Type.h"

#include <set>

using namespace fabrique::ast;
using std::string;


Filename::Filename(UniqPtr<Expression>& name, UniqPtrVec<Argument>& args,
                   const Type& t, const SourceRange& src)
	: Expression(t, src), unqualName_(std::move(name)),
	  args_(std::move(args))
{
}


const Expression* Filename::Lookup(const Identifier& id) const
{
	const string& name = id.name();

	if (name == "name")
		return unqualName_.get();

	/*
	 * TODO: implement subdir, generated and other values whose types
	 *       we understand at parse time but whose values we do not
	 */

	for (auto& arg : args_)
		if (name == arg->getName().name())
			return arg.get();

	return nullptr;
}


void Filename::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out
		<< Bytestream::Action << "file"
		<< Bytestream::Operator << "(";

	out << Bytestream::Filename << *unqualName_ << Bytestream::Reset;

	for (auto& a : args_)
		out
			<< Bytestream::Operator << ", "
			<< Bytestream::Reset << *a;

	out
		<< Bytestream::Operator << ")"
		<< Bytestream::Reset;
}


void Filename::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		unqualName_->Accept(v);
		for (auto& a : args_)
			a->Accept(v);
	}

	v.Leave(*this);
}
