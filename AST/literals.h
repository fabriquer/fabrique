/** @file literals.h    Declaration of several literal expression types. */
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

#ifndef LITERALS_H
#define LITERALS_H

#include "ADT/CStringRef.h"
#include "AST/Expression.h"

#include <string>

namespace fabrique {
namespace ast {

/**
 * An expression whose value is literally expressed in the source file.
 */
template<class T>
class Literal : public Expression
{
public:
	const T& value() const { return val; }
	bool isStatic() const { return true; }

protected:
	Literal(const T& value, const Type& ty, const SourceRange& loc)
		: Expression(ty, loc), val(value)
	{
	}

private:
	const T val;
};


class BoolLiteral : public Literal<bool>
{
public:
	BoolLiteral(bool value, const Type& ty, const SourceRange& loc)
		: Literal(value, ty, loc)
	{
	}

	void PrettyPrint(std::ostream&, int indent = 0) const;
	virtual void Accept(Visitor&) const;
};

class IntLiteral : public Literal<int>
{
public:
	IntLiteral(int value, const Type& ty, const SourceRange& loc)
		: Literal(value, ty, loc)
	{
	}

	void PrettyPrint(std::ostream&, int indent = 0) const;
	virtual void Accept(Visitor&) const;
};


class StringLiteral : public Literal<std::string>
{
public:
	StringLiteral(const std::string& s, const Type& ty,
	              const SourceRange& loc)
		: Literal(s, ty, loc)
	{
	}

	void PrettyPrint(std::ostream&, int indent = 0) const;
	virtual void Accept(Visitor&) const;
};

} // namespace ast
} // namespace fabrique

#endif
