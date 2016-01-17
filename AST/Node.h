/** @file AST/Node.h    Declaration of @ref fabrique::ast::Node. */
/*
 * Copyright (c) 2014-2016 Jonathan Anderson
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

#ifndef AST_NODE_H
#define AST_NODE_H

#include "Support/Printable.h"
#include "Support/SourceLocation.h"
#include "Support/Uncopyable.h"
#include "Support/Visitable.h"
#include "Types/OptionallyTyped.h"
#include "Types/TypeContext.h"

#include "Pegmatite/ast.hh"

namespace fabrique {

typedef pegmatite::ASTStack ParserStack;
typedef pegmatite::InputRange ParserInput;

namespace parser {
class ErrorReporter;
}

namespace ast {

class Node;
class Scope;
class Visitor;

typedef std::unique_ptr<Node> NodePtr;

/**
 * Base class for expressions that can be evaluated.
 */
class Node : public HasSource, public OptionallyTyped, public Printable,
             public Visitable<Visitor>, private Uncopyable
{
public:
	virtual ~Node();

protected:
	Node(const SourceRange& src, const Type &t)
		: HasSource(src), OptionallyTyped(t)
	{
	}

	Node(const SourceRange& src, const Type *t = nullptr)
		: HasSource(src), OptionallyTyped(t)
	{
	}

	class Parser : public pegmatite::ASTContainer
	{
		public:
		template<class T, bool Optional = false>
		using ChildNodeParser = pegmatite::ASTPtr<typename T::Parser, Optional>;

		template<class T>
		using ChildNodes = pegmatite::ASTList<typename T::Parser>;

		using Err = parser::ErrorReporter;

		Parser() : source_(SourceRange::None()), type_(nullptr) {}
		virtual ~Parser();

		virtual Node* Build(const Scope&, TypeContext&, Err&) const = 0;

		SourceRange source() const { return source_; }

		protected:
		SourceRange source_;
		Type *type_;
	};

	//using ParseContainer = pegmatite::ASTContainer;
	using ParseError = pegmatite::ErrorReporter;
};

} // namespace ast
} // namespace fabrique

#endif
