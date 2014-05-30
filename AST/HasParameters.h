/**
 * @file AST/HasParameters.h
 * Declaration of @ref fabrique::ast::HasParameters.
 */
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

#ifndef CALLABLE_H
#define CALLABLE_H

#include "ADT/PtrVec.h"
#include "ADT/StringMap.h"

#include "AST/Parameter.h"
#include "AST/SymbolReference.h"

#include <memory>
#include <set>
#include <string>

namespace fabrique {
namespace ast {

class Argument;


/**
 * A mixin type for something that can be called with parameters.
 */
class HasParameters
{
public:
	HasParameters(UniqPtrVec<Parameter>&);

	const UniqPtrVec<Parameter>& parameters() const;
	const std::set<std::string>& parameterNames() const;
	void CheckArguments(const UniqPtrVec<Argument>&,
	                    const SourceRange&) const;

	using ParamIterator = UniqPtrVec<Parameter>::const_iterator;
	ParamIterator begin() const { return params_.begin(); }
	ParamIterator end() const { return params_.end(); }

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

private:
	std::vector<std::string>
	NameArguments(const std::vector<std::string>&, SourceRange src) const;

	UniqPtrVec<Parameter> params_;
	std::set<std::string> paramNames_;
};

} // namespace ast
} // namespace fabrique

#endif
