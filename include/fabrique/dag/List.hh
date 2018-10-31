/** @file DAG/List.h    Declaration of @ref fabrique::dag::List. */
/*
 * Copyright (c) 2013-2014, 2018 Jonathan Anderson
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

#ifndef DAG_LIST_H
#define DAG_LIST_H

#include <fabrique/dag/Value.hh>
#include <fabrique/types/SequenceType.hh>

#include <memory>
#include <vector>


namespace fabrique {
namespace dag {


//! The result of evaluating an expression.
class List : public Value
{
public:
	template<class T>
	static List* of(const SharedPtrVec<T>& values, const SourceRange& src,
	                TypeContext& ctx)
	{
		SharedPtrVec<Value> v;
		for (const std::shared_ptr<T>& x : values)
			v.push_back(x);

		return List::of(v, src, ctx);
	}

	static List* of(const SharedPtrVec<Value>&, const SourceRange&,
	                TypeContext& ctx);

	virtual const SequenceType& type() const override;

	typedef SharedPtrVec<Value>::const_iterator iterator;

	iterator begin() const;
	iterator end() const;
	const SharedPtrVec<Value>& elements() const { return elements_; }
	size_t size() const;
	const Value& operator [] (size_t) const;

	//! List addition is concatenation.
	virtual ValuePtr Add(ValuePtr&) const override;
	virtual ValuePtr PrefixWith(ValuePtr&) const override;
	virtual const List* asList() const override { return this; }
	virtual bool canScalarAdd(const Value&) const override;

	virtual void PrettyPrint(Bytestream&, unsigned int indent = 0) const override;
	void Accept(Visitor& v) const override;

private:
	List(const SharedPtrVec<Value>&, const Type&, const SourceRange&);

	const SharedPtrVec<Value> elements_;
	const Type& elementType_;
};

} // namespace dag
} // namespace fabrique

#endif // !DAG_LIST_H
