/** @file AST/Identifier.cc    Definition of @ref fabrique::ast::Identifier. */
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

#include "AST/Identifier.h"
#include "AST/TypeReference.h"
#include "AST/Visitor.h"
#include "Parsing/ErrorReporter.h"
#include "Support/Bytestream.h"
#include "Types/Type.h"

using namespace fabrique::ast;


static const char* ReservedNames[] =
{
	"args",
	"buildroot",
	"in",
	"out",
	"srcroot",
};


Identifier::Parser::Parser()
	: source_(SourceRange::None())
{
}


bool Identifier::Parser::construct(const ParserInput& input, ParserStack&,
                                   const ParseError&)
{
	source_ = input;
	name_ = input.str();
	return true;
}


Identifier* Identifier::Parser::Build(const Scope&, TypeContext&, Err& err)
{
	if (name_.length() == 0)
	{
		err.ReportError("empty identifier", source_);
		return nullptr;
	}

	return new Identifier(name_, source_);
}


Identifier::Identifier(const std::string& name, const SourceRange& src)
	: Node(src), name_(name)
{
}


bool Identifier::reservedName() const
{
	for (const char *reserved : ReservedNames)
		if (name_ == reserved)
			return true;

	return false;
}


void Identifier::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out << Bytestream::Reference << name_ << Bytestream::Reset;
}


void Identifier::Accept(Visitor& v) const { v.Enter(*this); v.Leave(*this); }


bool Identifier::operator == (const Identifier& other) const
{
	if (name() != other.name())
		return false;

	if (isTyped() != other.isTyped())
		return false;

	if (isTyped() and type() != other.type())
		return false;

	return true;
}

bool Identifier::operator < (const Identifier& other) const
{
	if (name() < other.name())
		return true;

	if (not isTyped() and other.isTyped())
		return true;

	if (isTyped() and type() < other.type())
		return true;

	return false;
}
