/** @file AST/ASTDump.cc    Definition of @ref fabrique::ast::ASTDump. */
/*
 * Copyright (c) 2013, 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme and at Memorial University
 * of Newfoundland under the NSERC Discovery program (RGPIN-2015-06048).
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
#include <fabrique/ast/ASTDump.hh>

#include <iomanip>

using namespace fabrique::ast;
using std::string;


ASTDump::ASTDump(Bytestream &o)
	: out_(o), indent_(1)
{
}


#define DUMP_NODE(type)                                                      \
bool ASTDump::Enter(const type& n)                                           \
{                                                                            \
	Write(#type, &n); indent_++; return true;                             \
}                                                                            \
void ASTDump::Leave(const type&) { indent_--; }

DUMP_NODE(Action)
DUMP_NODE(Argument)
DUMP_NODE(BinaryOperation)
DUMP_NODE(BoolLiteral)
DUMP_NODE(Call)
DUMP_NODE(CompoundExpression)
DUMP_NODE(Conditional)
DUMP_NODE(FileList)
DUMP_NODE(ForeachExpr)
DUMP_NODE(Function)
DUMP_NODE(Identifier)
DUMP_NODE(IntLiteral)
DUMP_NODE(List)
DUMP_NODE(Parameter)
DUMP_NODE(StringLiteral)
DUMP_NODE(Type)
DUMP_NODE(Value)

void ASTDump::Write(const string& s, const void *ptr)
{
	const unsigned int indentLen = 2 * this->indent_;
	string indent(indentLen, ' ');

	out_
		<< Bytestream::Line << static_cast<unsigned long>(indent_) << indent
		<< Bytestream::Type << s
		<< Bytestream::Operator << " @ "
		<< Bytestream::Literal << reinterpret_cast<unsigned long>(ptr)
		<< "\n"
		;
}
