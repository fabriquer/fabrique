/** @file DAG/Callable.h    Declaration of @ref fabrique::dag::Callable. */
/*
 * Copyright (c) 2014 Jonathan Anderson
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

#ifndef DAG_CALLABLE_H
#define DAG_CALLABLE_H

#include "ADT/PtrVec.h"
#include "ADT/StringMap.h"
#include "DAG/Value.h"
#include "Support/SourceLocation.h"

#include <functional>
#include <string>

namespace fabrique {

class Type;

namespace dag {

class Argument;
class DAGBuilder;
class Parameter;


/**
 * A mixin type for something that can be called with parameters.
 */
class Callable
{
public:
	virtual ~Callable();

	//! Call this function with (named) arguments.
	virtual ValuePtr Call(const ValueMap& arguments, DAGBuilder&,
	                      SourceRange = SourceRange::None()) const;

	const SharedPtrVec<Parameter>& parameters() const;

	bool hasParameterNamed(const std::string&) const;
	void CheckArguments(const ValueMap& args,
	                    const StringMap<SourceRange>& argLocations,
	                    const SourceRange& callLocation) const;

	/**
	 * Name all of the arguments in @a v according to the rules for
	 * positional and keyword arguments.
	 */
	template<class T>
	StringMap<const T*> NameArguments(const UniqPtrVec<T>& v) const
	{
		std::vector<std::string> names;
		SourceLocation begin, end;

		// Build a vector of what we currently know about the
		// arguments' names.
		for (auto& i : v)
		{
			const SourceRange& src = i->source();
			if (not begin) begin = src.begin;
			end = src.end;

			names.push_back(
				i->hasName() ? i->getName().name() : "");
		}

		// Fill in any gaps with knowledge about formal parameters.
		names = NameArguments(names, SourceRange(begin, end));

		// Return the result.
		StringMap<const T*> result;
		for (size_t i = 0; i < v.size(); i++)
			result.emplace(names[i], v[i].get());

		return result;
	}

protected:
	typedef std::function<
		ValuePtr (const ValueMap&, DAGBuilder&, SourceRange)>
		Evaluator;
	Callable(const SharedPtrVec<Parameter>&, Evaluator);

private:
	std::vector<std::string>
	NameArguments(const std::vector<std::string>&, SourceRange src) const;

	const SharedPtrVec<Parameter> parameters_;
	const Evaluator evaluator_;
};

} // namespace ast
} // namespace fabrique

#endif
