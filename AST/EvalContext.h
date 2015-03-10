/** @file AST/EvalContext.h    Declaration of @ref fabrique::ast::EvalContext. */
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

#ifndef EVAL_CONTEXT_H
#define EVAL_CONTEXT_H

#include <deque>
#include <stack>
#include <string>

#include "DAG/DAG.h"
#include "DAG/DAGBuilder.h"
#include "DAG/Record.h"
#include "DAG/Value.h"


namespace fabrique {

class Arguments;
class FileType;
class FunctionType;
class TypeContext;

namespace dag {
class Build;
class File;
class Parameter;
class Rule;
class Target;
}

namespace ast {
class Scope;


/**
 * A context for evaluating AST @ref Expression objects.
 *
 * This class adds AST scoping logic to @ref dag::DAGBuilder::Context.
 */
class EvalContext : public dag::DAGBuilder::Context
{
public:
	EvalContext(TypeContext& ctx, std::string buildroot, std::string srcroot)
		: ctx_(ctx), builder_(*this),
		  buildroot_(buildroot), srcroot_(srcroot)
	{
	}

	~EvalContext() {}

	std::vector<dag::DAG::BuildTarget> Evaluate(const ast::Scope&);


	/**
	 * An object to represent descending in a call stack. Will push and
	 * pop scopes names appropriately when initialized and destructed.
	 */
	class Scope
	{
		public:
		Scope(const Scope&) = delete;
		Scope(Scope&&);
		~Scope();

		bool contains(const std::string& name);
		void set(std::string name, dag::ValuePtr);

		dag::ValueMap leave();

		private:
		Scope(EvalContext& stack, std::string name, dag::ValueMap&);

		EvalContext& stack_;
		std::string name_;
		dag::ValueMap& symbols_;

		friend class EvalContext;
	};

	Scope EnterScope(const std::string& name);


	/**
	 * An object that represents the use of an alternative scope stack and
	 * that will restore the original stack on destruction.
	 *
	 * For instance, when calling a function, we need to switch to the
	 * function definition's stack rather than the 
	 */
	class AlternateScoping
	{
		public:
		AlternateScoping(AlternateScoping&&);
		~AlternateScoping();

		private:
		AlternateScoping(EvalContext&, std::deque<dag::ValueMap>&&);

		EvalContext& stack_;
		std::deque<dag::ValueMap> originalScopes_;

		friend class EvalContext;
	};

	AlternateScoping ChangeScopeStack(const dag::ValueMap& alternativeScope);


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

	virtual std::string buildroot() const override { return buildroot_; }
	virtual std::string srcroot() const override { return srcroot_; }

	virtual std::string currentValueName() const override;
	virtual TypeContext& types() const override { return ctx_; }


	//! Define a named @ref dag::Value in the current scope.
	void Define(ScopedValueName& name, dag::ValuePtr value);

	//! Look up a named value from the current scope or a parent scope.
	dag::ValuePtr Lookup(const std::string& name);


	//! Define a @ref dag::Function.
	dag::ValuePtr Function(dag::Function::Evaluator,
	                       const SharedPtrVec<dag::Parameter>&,
	                       const FunctionType&,
	                       SourceRange = SourceRange::None());


	//! Create a new alias for an existing @ref dag::Target.
	void Alias(const std::shared_ptr<dag::Target>&);

protected:
	dag::ValueMap& CurrentScope();
	dag::ValueMap PopScope();

	//! Make a deep copy of the current scope and all of its parents.
	dag::ValueMap CopyCurrentScope();

	void DumpScope();

	void PushValueName(const std::string&);
	std::string PopValueName();

	//! The fully-qualified name of the value currently being defined.
	std::string fullyQualifiedName() const;

	std::string qualifyName(std::string name) const;

	TypeContext& ctx_;

	//! The components of the current scope's fully-qualified name.
	std::deque<std::string> scopeName_;

	//! Symbols defined in this scope (or the one up from it, or up...).
	std::deque<dag::ValueMap> scopes_;

	// Values we've created:
	SharedPtrVec<dag::File> files_;
	SharedPtrVec<dag::Build> builds_;
	SharedPtrMap<dag::Rule> rules_;
	SharedPtrMap<dag::Value> variables_;
	SharedPtrMap<dag::Target> targets_;

private:
	/** The name of the value we are currently processing. */
	std::deque<std::string> currentValueName_;

	dag::DAGBuilder builder_;

	const std::string buildroot_;
	const std::string srcroot_;
};

} // namespace dag
} // namespace fabrique

#endif
