/** @file AST/literals.h    Declaration of several literal expression types. */
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

#ifndef LITERALS_H
#define LITERALS_H

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
	const T& value() const { return value_; }
	virtual std::string str() const override = 0;

protected:
	Literal(const T& value, const Type& ty, const SourceRange& loc)
		: Expression(ty, loc), value_(value)
	{
	}

private:
	const T value_;
};


//! A literal 'true' or 'false' value in code.
class BoolLiteral : public Literal<bool>
{
public:
	std::string str() const override;

	virtual dag::ValuePtr evaluate(EvalContext&) const override;
	virtual void PrettyPrint(Bytestream&, size_t indent = 0) const override;
	virtual void Accept(Visitor&) const override;

	class Parser : public Expression::Parser
	{
	public:
		bool construct(const ParserInput&, ParserStack&,
		               const ParseError&) override;

		BoolLiteral* Build(const Scope&, TypeContext&, Err&) override;

		bool value_;
	};

private:
	BoolLiteral(bool value, const Type& ty, const SourceRange& loc)
		: Literal(value, ty, loc)
	{
	}
};


//! An integer value in code.
class IntLiteral : public Literal<int>
{
public:
	std::string str() const override;

	virtual dag::ValuePtr evaluate(EvalContext&) const override;
	virtual void PrettyPrint(Bytestream&, size_t indent = 0) const override;
	virtual void Accept(Visitor&) const override;

	class Parser : public Expression::Parser
	{
	public:
		bool construct(const ParserInput&, ParserStack&,
		               const ParseError&) override;

		IntLiteral* Build(const Scope&, TypeContext&, Err&) override;

		int value_;
	};

private:
	IntLiteral(int value, const Type& ty, const SourceRange& loc)
		: Literal(value, ty, loc)
	{
	}
};


//! A string value enclosed by single or double quotes.
class StringLiteral : public Literal<std::string>
{
public:
	std::string str() const override;

	virtual dag::ValuePtr evaluate(EvalContext&) const override;
	virtual void PrettyPrint(Bytestream&, size_t indent = 0) const override;
	virtual void Accept(Visitor&) const override;

	class Parser : public Expression::Parser
	{
	public:
		bool construct(const ParserInput&, ParserStack&,
		               const ParseError&) override;

		StringLiteral* Build(const Scope&, TypeContext&, Err&) override;

		std::string value_;
		std::string quote_; // single quote, double quote, etc.
	};

private:
	StringLiteral(const std::string& s, const Type& ty, std::string quote,
	              const SourceRange& loc)
		: Literal(s, ty, loc), quote_(quote)
	{
	}

	const std::string quote_;
};

} // namespace ast
} // namespace fabrique

#endif
