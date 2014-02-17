/** @file Scope.h    Declaration of @ref Scope. */
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

#ifndef SCOPE_H
#define SCOPE_H

#include "ADT/PtrVec.h"
#include "ADT/StringMap.h"
#include "Support/Uncopyable.h"

#include <map>
#include <memory>
#include <string>

namespace fabrique {
namespace ast {

class Argument;
class Expression;
class Identifier;
class Parameter;
class Value;
class Visitor;


/**
 * A scope is a container for name->value mappings.
 *
 * A scope can have a parent scope for recursive name lookups.
 */
class Scope : public Uncopyable
{
public:
	Scope(const Scope *parent, const std::string& name);
	Scope(Scope&&);

	typedef StringMap<const Expression*> SymbolMap;
	typedef SymbolMap::const_iterator iterator;

	iterator begin() const { return symbols.begin(); }
	iterator end() const { return symbols.end(); }

	const std::string& name() const { return debugName; }
	const UniqPtrVec<Value>& values() const { return ownedValues; }

	const Expression* Lookup(const Identifier&) const;

	void Register(const Argument*);
	void Register(const Parameter*);
	void Register(const Value&);

	void Take(Value*);

	virtual void Accept(Visitor&) const;

private:
	void Register(const Identifier&, const Expression*);

	const Scope *parent;
	const std::string debugName;

	SymbolMap symbols;
	UniqPtrVec<Value> ownedValues;
};

} // namespace ast
} // namespace fabrique

#endif
