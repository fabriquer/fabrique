/** @file Action.h    Declaration of @ref Action. */
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

#ifndef ACTION_H
#define ACTION_H

#include "ADT/PtrVec.h"
#include "ADT/StringMap.h"
#include "AST/Argument.h"
#include "AST/Expression.h"
#include "AST/Parameter.h"
#include "Support/Bytestream.h"

namespace fabrique {
namespace ast {


/**
 * A build action that can transform inputs into outputs.
 */
class Action : public Expression
{
public:
	Action(PtrVec<Argument>& args, StringMap<const Parameter*>& params,
	       const Type& ty, const SourceRange& loc);

	const PtrVec<Argument>& arguments() const { return args; }
	const StringMap<const Parameter*>& parameters() const { return params; }

	/**
	 * Name all of the arguments in @a in according to the rules for
	 * positional and keyword arguments.
	 *
	 * For instance, given the action:
	 * foo = action('$FOO $in $out --bar=$bar', bar = 'default_bar')
	 *
	 * and the call:
	 * baz = foo(infile, bar = 'something', out = outfile)
	 *
	 * You could pass [ "", "bar", "out" ] to Action::NameArguments()
	 * and receive in return:
	 *
	 * {
	 *   "in": 0,
	 *   "out": 2,
	 *   "bar": 1,
	 * }
	 */
	StringMap<int> NameArguments(const std::vector<std::string>& in) const;

	virtual void PrettyPrint(Bytestream&, int indent = 0) const;
	virtual void Accept(Visitor&) const;

private:
	PtrVec<Argument> args;
	StringMap<const Parameter*> params;
};

} // namespace ast
} // namespace fabrique

#endif
