//! @file ast/Arguments.cc    Definition of @ref fabrique::ast::Arguments
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

#include <fabrique/Bytestream.hh>
#include <fabrique/ast/Arguments.hh>
#include <fabrique/ast/Visitor.hh>

using namespace fabrique;
using namespace fabrique::ast;

Arguments::Arguments(UniqPtrVec<Expression> positional, UniqPtrVec<Argument> keyword,
                     SourceRange src)
	: Node(src), positional_(std::move(positional)), keyword_(std::move(keyword))
{
}

void Arguments::PrettyPrint(Bytestream &out, unsigned int indent) const
{
	for (size_t i = 0; i < positional_.size(); )
	{
		positional_[i]->PrettyPrint(out, indent + 1);
		if (++i < positional_.size() or not keyword_.empty())
			out
				<< Bytestream::Operator << ", "
				<< Bytestream::Reset;
	}

	for (size_t i = 0; i < keyword_.size(); )
	{
		keyword_[i]->PrettyPrint(out, indent + 1);
		if (++i < keyword_.size())
			out
				<< Bytestream::Operator << ", "
				<< Bytestream::Reset;
	}
}

void Arguments::Accept(Visitor &v) const
{
	if (v.Enter(*this))
	{
		for (auto &p : positional_)
		{
			p->Accept(v);
		}

		for (auto &kwarg : keyword_)
		{
			kwarg->Accept(v);
		}
	}

	v.Leave(*this);
}
