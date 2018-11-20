/** @file AST/Arguments.h    Declaration of @ref fabrique::ast::Arguments. */
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

#ifndef AST_ARGUMENTS_H
#define AST_ARGUMENTS_H

#include <fabrique/UniqPtr.h>
#include <fabrique/ast/Argument.hh>
#include <fabrique/ast/Node.hh>

namespace fabrique {
namespace ast {


/**
 * Arguments to something Callable: positional arguments followed by keyword arguments.
 */
class Arguments : public Node
{
public:
	Arguments(UniqPtrVec<Expression> positional, UniqPtrVec<Argument> keyword,
	          SourceRange);

	const UniqPtrVec<Expression>& positional() const { return positional_; }
	const UniqPtrVec<Argument>& keyword() const { return keyword_; }

	size_t size() const { return positional_.size() + keyword_.size(); }

	virtual void PrettyPrint(Bytestream&, unsigned int indent = 0) const override;
	virtual void Accept(Visitor&) const override;

private:
	const UniqPtrVec<Expression> positional_;
	const UniqPtrVec<Argument> keyword_;
};

} // namespace ast
} // namespace fabrique

#endif
