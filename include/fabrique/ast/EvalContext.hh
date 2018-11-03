/** @file AST/EvalContext.h    Declaration of @ref fabrique::ast::EvalContext. */
/*
 * Copyright (c) 2014, 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme and at Memorial University
 * of Newfoundland under the NSERC Discovery program (RGPIN-2015-06048).
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

#ifndef EVAL_CONTEXT_H
#define EVAL_CONTEXT_H

#include <fabrique/dag/DAG.hh>
#include <fabrique/dag/DAGBuilder.hh>
#include <fabrique/dag/Record.hh>
#include <fabrique/dag/Value.hh>

#include <deque>
#include <stack>
#include <string>


namespace fabrique {

class CLIArguments;
class FileType;
class FunctionType;
class TypeContext;

namespace dag {
class Build;
class File;
class Parameter;
class Rule;
}

namespace ast {

class Value;


/**
 * A context for evaluating AST @ref Expression objects.
 *
 * This class adds AST scoping logic to @ref dag::DAGBuilder::Context.
 */
class EvalContext : public dag::DAGBuilder::Context
{
public:
	EvalContext(TypeContext& ctx, dag::ValueMap builtins = {});
	~EvalContext() override {}

	std::vector<dag::DAG::BuildTarget> Evaluate(const UniqPtrVec<Value>&);


	/**
	 * A collection of named values that came from a lexical scope.
	 *
	 * These values have been evaluated, having originally come from the same lexical
	 * scope. We can look up values within a scope; if a named value is not found
	 * within it, the parent scope (if there is one) is consulted.
	 *
	 * This class is designed to outlast the EvalContext that creates it, and it
	 * keeps its parent Scope around via shared pointer as long as it exists. This
	 * allows us to retain a Scope for evaluating functions and build actions, both
	 * of which can capture values from their lexical scope.
	 */
	class ScopedValues : public Printable
	{
	public:
		ScopedValues(std::string name, std::shared_ptr<ScopedValues> parent);
		ScopedValues(const ScopedValues&) = delete;

		//! Is the given name defined in this scope?
		bool contains(const std::string &name) const;

		//! Define a name within a scope
		ScopedValues& Define(const std::string &name, dag::ValuePtr,
		                     SourceRange = SourceRange::None());

		//! Look up a given name in this scope or its parents
		dag::ValuePtr Lookup(const std::string &name) const;

		virtual void PrettyPrint(Bytestream&, unsigned int) const override;

	private:
		const std::string name_;
		const std::shared_ptr<ScopedValues> parent_;
		dag::ValueMap values_;
	};

	/**
	 * A lexical scope that is currently being defined.
	 */
	class Scope
	{
	public:
		Scope(std::shared_ptr<ScopedValues>, EvalContext&);
		Scope(const Scope&) = delete;
		Scope(Scope&&);
		~Scope();

		//! Is the given name defined in this scope?
		bool contains(const std::string &name) const;

		//! Define a name within a scope
		Scope& Define(const std::string &name, dag::ValuePtr,
		              SourceRange = SourceRange::None());

	private:
		EvalContext &ctx_;
		std::shared_ptr<ScopedValues> values_;

		/**
		 * This scope is "live", i.e., when we destruct we should pop the current
		 * ScopedValues from the stack.
		 */
		bool live_;
	};

	std::shared_ptr<ScopedValues> CurrentScope();

	/**
	 * Enter a new lexical scope, optionally specifying the parent scope that
	 * should be used in name resolution.
	 *
	 * Normally, the parent scope is the scope that surrounds the code being
	 * evaluated. In the case of function calls, however, we need to use the
	 * scope surrounding the function definition rather than the call if we
	 * want the function to be able to capture values (like a lambda function).
	 */
	Scope EnterScope(const std::string& name,
	                 std::shared_ptr<ScopedValues> parentScope = nullptr);


	/**
	 * An object for declaring the name of the value whose initializer
	 * we are currently evaluating. Will push and pop names appropriately
	 * when initialized and destructed.
	 */
	class ScopedValueName
	{
		public:
		ScopedValueName(const ScopedValueName&) = delete;
		ScopedValueName(ScopedValueName&&);
		~ScopedValueName();

		void done();

		private:
		ScopedValueName(EvalContext& stack, std::string name);

		EvalContext& stack_;
		std::string name_;

		friend class EvalContext;
	};

	ScopedValueName evaluating(const std::string& name);


	dag::DAGBuilder& builder() { return builder_; }

	virtual std::string currentValueName() const override;
	virtual TypeContext& types() const override { return ctx_; }


	//! Define a named @ref dag::Value in the current scope.
	void Define(ScopedValueName& name, dag::ValuePtr value, SourceRange);

	//! Look up a named value from the current scope or a parent scope.
	dag::ValuePtr Lookup(const std::string& name, SourceRange = SourceRange::None());

private:
	std::shared_ptr<ScopedValues> PopScope();

	void PushValueName(const std::string&);
	std::string PopValueName();

	//! The fully-qualified name of the value currently being defined.
	std::string fullyQualifiedName() const;

	TypeContext& ctx_;

	//! Symbols defined in this scope (or the one up from it, or up...).
	std::deque<std::shared_ptr<ScopedValues>> scopes_;

	/** The name of the value we are currently processing. */
	std::deque<std::string> currentValueName_;

	dag::DAGBuilder builder_;

	//! Pre-defined values like `srcroot` and `file`.
	dag::ValueMap builtins_;
};

} // namespace dag
} // namespace fabrique

#endif
