/** @file Identifier.h    Declaration of @ref Identifier. */
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

#ifndef IDENTIFIER_H
#define IDENTIFIER_H

#include "AST/Node.h"
#include "Types/Typed.h"

#include <string>

namespace fabrique {
namespace ast {

class Visitor;


/**
 * The name of a value, function, parameter or argument.
 */
class Identifier : public Node
{
public:
	Identifier(const std::string& s, const Type *ty, const SourceRange& loc)
		: Node(loc), id(s), ty(ty)
	{
	}

	bool isTyped() const { return (ty != NULL); }
	const Type* type() const { return ty; }

	void PrettyPrint(Bytestream&, int indent = 0) const;
	const std::string& name() const { return id; }

	bool operator == (const Identifier&) const;
	bool operator < (const Identifier&) const;

	virtual void Accept(Visitor&) const;

private:
	const std::string id;
	const Type *ty;
};

} // namespace ast
} // namespace fabrique

#endif
