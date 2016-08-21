/** @file AST/Parameter.h    Declaration of @ref fabrique::ast::Parameter. */
/*
 * Copyright (c) 2013, 2015-2016 Jonathan Anderson
 * All rights reserved.
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

#ifndef PARAMETER_H
#define PARAMETER_H

#include "AST/Identifier.h"
#include "AST/TypeReference.h"
#include "Types/Typed.h"

#include <memory>

namespace fabrique {

namespace dag {
class Parameter;
}

namespace ast {

class EvalContext;
class Expression;

/**
 * A formal parameter in a @ref fabrique::ast::Function.
 */
class Parameter : public Node
{
public:
	const Identifier& getName() const { return *name_; }
	const UniqPtr<Expression>& defaultArgument() const
	{
		return defaultArgument_;
	}

	virtual void PrettyPrint(Bytestream&, size_t indent = 0) const override;
	virtual void Accept(Visitor&) const override;

	class Parser : public Node::Parser
	{
	public:
		virtual ~Parser();
		virtual Parameter* Build(const Scope&, TypeContext&, Err&)
			override;

	protected:
		ChildNodeParser<Identifier> name_;
		ChildNodeParser<TypeReference> type_;
	};

	class WithDefault : public Parser
	{
	public:
		virtual ~WithDefault();
		Parameter* Build(const Scope&, TypeContext&, Err&) override;

	private:
		ChildNodeParser<Expression> defaultArgument_;
	};

	virtual std::shared_ptr<dag::Parameter> evaluate(EvalContext&) const;

private:
	Parameter(UniqPtr<Identifier>& id, UniqPtr<TypeReference>& t,
	          UniqPtr<Expression> defaultArgument);

	const UniqPtr<Identifier> name_;
	const UniqPtr<TypeReference> type_;
	const UniqPtr<Expression> defaultArgument_;
};

} // namespace ast
} // namespace fabrique

#endif
