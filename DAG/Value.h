/** @file Value.h    Declaration of @ref Value. */
/*
 * Copyright (c) 2013-2014 Jonathan Anderson
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

#ifndef DAG_VALUE_H
#define DAG_VALUE_H

#include "ADT/StringMap.h"
#include "Support/Printable.h"
#include "Support/SourceLocation.h"
#include "Types/Typed.h"

#include <string>

namespace fabrique {

class Type;

namespace dag {


//! The result of evaluating an expression.
class Value : public HasSource, public Printable, public Typed
{
public:
	//! Unary 'not' operator.
	virtual std::shared_ptr<Value> Negate(const SourceRange& loc) const;

	/**
	 * Add this @ref Value to a following @ref Value.
	 *
	 * The implementation of addition is type-dependent: it might make
	 * sense to add, concatenate or apply a logical AND.
	 */
	virtual std::shared_ptr<Value> Add(std::shared_ptr<Value>&);

	//! Apply the prefix operation: prefix this value with another value.
	virtual std::shared_ptr<Value> PrefixWith(std::shared_ptr<Value>&);

	//! Add another @ref Value scalar-wise across this @ref Value.
	virtual std::shared_ptr<Value> ScalarAdd(std::shared_ptr<Value>&);

	/** Logical and. */
	virtual std::shared_ptr<Value> And(std::shared_ptr<Value>&);

	/** Logical or. */
	virtual std::shared_ptr<Value> Or(std::shared_ptr<Value>&);

	/** Logical xor. */
	virtual std::shared_ptr<Value> Xor(std::shared_ptr<Value>&);

	/**
	 * This @ref Value add @a v to itself in a scalar fashion.
	 * For instance, [ 1 2 ] can add 3 to itself but not vice versa.
	 */
	virtual bool canScalarAdd(const Value& v) { return false; }

protected:
	Value(const Type&, const SourceRange&);
};

typedef StringMap<std::shared_ptr<Value>> ValueMap;

} // namespace dag
} // namespace fabrique

#endif // !DAG_VALUE_H
