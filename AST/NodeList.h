/** @file AST/NodeList.h    Declaration of @ref fabrique::ast::NodeList. */
/*
 * Copyright (c) 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed at Memorial University of Newfoundland under
 * the NSERC Discovery program (RGPIN-2015-06048).
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

#ifndef AST_NODE_LIST_H
#define AST_NODE_LIST_H

#include "AST/Node.h"
#include "Support/Bytestream.h"

#include <cassert>
#include <list>

namespace fabrique {
namespace ast {

/**
 * A list of same-typed nodes.
 */
template<class T>
class NodeList : public Node
{
public:
	NodeList(NodePtr<T> firstValue, SourceRange src = SourceRange::None())
		: Node(src)
	{
		nodes_.emplace_back(std::move(firstValue));
	}

	NodeList(NodeList&& l)
		: Node(l.source()), nodes_(std::move(l.nodes_))
	{
	}

	virtual ~NodeList() override
	{
	}

	using ConstIterator = typename std::list<NodePtr<T>>::const_iterator;
	ConstIterator begin() const { return nodes_.begin(); }
	ConstIterator end() const { return nodes_.end(); }

	NodeList& prepend(NodePtr<T> n)
	{
		nodes_.push_front(std::move(n));
		return *this;
	}

	std::list<NodePtr<T>> takeAll()
	{
		return std::move(nodes_);
	}

	virtual void PrettyPrint(Bytestream& out, unsigned int indent = 0) const override
	{
		for (const auto &n : nodes_)
		{
			n->PrettyPrint(out, indent);
			out << " ";
		}
	}

	virtual void Accept(Visitor&) const override
	{
		assert(false && "unreachable: NodeList never present in completed AST");
	}

private:
	std::list<NodePtr<T>> nodes_;
};

} // namespace ast
} // namespace fabrique

#endif
