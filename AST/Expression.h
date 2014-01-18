/** @file Expression.h    Declaration of @ref Expression. */
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

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "ADT/PtrVec.h"
#include "Support/Location.h"
#include "Support/Printable.h"
#include "Support/Uncopyable.h"
#include "Support/Visitable.h"
#include "Types/Typed.h"

namespace fabrique {
namespace ast {

/**
 * Base class for expressions that can be evaluated.
 */
class Expression
	: public HasSource, public Printable, public Typed, public Visitable,
	  Uncopyable
{
public:
	virtual ~Expression() {}

	const Type& getType() const { return ty; }
	const SourceRange& getSource() const { return loc; }

	/**
	 * Can this expression be evaluated at parse time, independent of
	 * e.g., the state of source files on disk?
	 */
	virtual bool isStatic() const = 0;

protected:
	Expression(const Type& ty, const SourceRange& loc) : ty(ty), loc(loc) {}

private:
	const Type& ty;
	const SourceRange loc;
};

typedef PtrVec<Expression> ExprVec;

} // namespace ast
} // namespace fabrique

#endif
