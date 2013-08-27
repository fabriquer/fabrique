/** @file Visitor.h    Declaration of @ref Visitor. */
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


/**
 * Interface for visitors that walk the AST.
 */
class Visitor
{
public:
	virtual void Visit(const Action&) = 0;
	virtual void Visit(const Argument&) = 0;
	virtual void Visit(const BinaryOperation&) = 0;
	virtual void Visit(const BoolLiteral&) = 0;
	virtual void Visit(const Call&) = 0;
	virtual void Visit(const CompoundExpression&) = 0;
	virtual void Visit(const Conditional&) = 0;
	virtual void Visit(const File&) = 0;
	virtual void Visit(const FileList&) = 0;
	virtual void Visit(const ForeachExpr&) = 0;
	virtual void Visit(const Function&) = 0;
	virtual void Visit(const Identifier&) = 0;
	virtual void Visit(const IntLiteral&) = 0;
	virtual void Visit(const List&) = 0;
	virtual void Visit(const Parameter&) = 0;
	virtual void Visit(const StringLiteral&) = 0;
	virtual void Visit(const SymbolReference&) = 0;
	virtual void Visit(const Type&) = 0;
	virtual void Visit(const Value&) = 0;
};

#endif
