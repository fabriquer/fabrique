/** @file AST/Visitor.h    Declaration of @ref fabrique::ast::Visitor. */
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

#ifndef VISITOR_H
#define VISITOR_H

#include "AST/forward-decls.h"

namespace fabrique {
namespace ast {

//! Define entry (which returns true to continue descent) and exit methods.
#define VISIT(type) \
	virtual bool Enter(const type&) { return true; } \
	virtual void Leave(const type&) {}


/**
 * Interface for visitors that walk the AST.
 */
class Visitor
{
public:
	virtual ~Visitor();

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
	VISIT(Import)
	VISIT(IntLiteral)
	VISIT(List)
	VISIT(Parameter)
	VISIT(Scope)
	VISIT(StringLiteral)
	VISIT(SymbolReference)
	VISIT(Type)
	VISIT(UnaryOperation)
	VISIT(Value)
};

#undef VISIT
#define VISIT(type) \
	virtual bool Enter(const type&); \
	virtual void Leave(const type&);

} // namespace ast
} // namespace fabrique

#endif
