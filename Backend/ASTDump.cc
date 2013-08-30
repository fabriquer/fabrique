/** @file ASTDump.cc    Definition of @ref ASTDump. */
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

#include "Backend/ASTDump.h"
#include <iomanip>

using std::string;


ASTDump* ASTDump::Create(std::ostream& out)
{
	return new ASTDump(out);
}

#define DUMP_NODE(type) \
void ASTDump::Enter(const type& n) { Write(#type, &n); indent++; } \
void ASTDump::Leave(const type& n) { indent--;  }

DUMP_NODE(Action)
DUMP_NODE(Argument)
DUMP_NODE(BinaryOperation)
DUMP_NODE(BoolLiteral)
DUMP_NODE(Call)
DUMP_NODE(CompoundExpression)
DUMP_NODE(Conditional)
DUMP_NODE(File)
DUMP_NODE(FileList)
DUMP_NODE(ForeachExpr)
DUMP_NODE(Function)
DUMP_NODE(Identifier)
DUMP_NODE(IntLiteral)
DUMP_NODE(List)
DUMP_NODE(Parameter)
DUMP_NODE(StringLiteral)
DUMP_NODE(SymbolReference)
DUMP_NODE(Type)
DUMP_NODE(Value)

void ASTDump::Write(const string& s, const void *ptr)
{
	const size_t indentLen = 2 * this->indent;
	std::vector<char> indentChars(indentLen, ' ');
	string indent(indentChars.data(), indentLen);

	out
		<< indent << s
		<< " @ 0x" << std::hex << (unsigned long long) ptr
		<< std::endl
		;
}
