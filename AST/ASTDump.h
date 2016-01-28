/** @file AST/ASTDump.h    Declaration of @ref fabrique::ast::ASTDump. */
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

#ifndef AST_DUMP_H
#define AST_DUMP_H

#include "AST/Visitor.h"


namespace fabrique {

class Bytestream;

namespace ast {

/**
 * Backend that prints the AST as a (not very pretty) tree.
 */
class ASTDump : public Visitor
{
public:
	static ASTDump* Create(Bytestream&);

	VISIT(Action)
	VISIT(Argument)
	VISIT(BinaryOperation)
	VISIT(BoolLiteral)
	VISIT(Call)
	VISIT(CompoundExpression)
	VISIT(Conditional)
	VISIT(Filename)
	VISIT(FileList)
	VISIT(ForeachExpr)
	VISIT(Function)
	VISIT(Identifier)
	VISIT(IntLiteral)
	VISIT(List)
	VISIT(NameReference)
	VISIT(Parameter)
	VISIT(StringLiteral)
	VISIT(Type)
	VISIT(Value)

private:
	ASTDump(Bytestream& o)
		: out_(o), indent_(0)
	{
	}

	void Write(const std::string& message, const void *ptr);

	Bytestream& out_;
	size_t indent_;
};

} // namespace ast
} // namespace fabrique

#endif
