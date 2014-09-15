/** @file DAG/EvalContext.h    Declaration of @ref fabrique::dag::EvalContext. */
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

#include "DAG/Structure.h"
#include "DAG/Value.h"


namespace fabrique {

class Arguments;
class FileType;
class FunctionType;
class TypeContext;

// TODO: fix layering violation with a callback mechanism for function evaluation
namespace ast { class Function; }

namespace dag {

class Build;
class DAG;
class File;
class Parameter;
class Rule;
class Target;

class EvalContext
{
public:
	static UniqPtr<DAG> Evaluate(const ast::Scope&, TypeContext&,
	                             std::string srcroot, std::string buildroot,
	                             const std::vector<std::string>& inputFiles,
	                             std::string outputFile,
	                             const Arguments& regenerateArguments);

	EvalContext(TypeContext& ctx) : ctx_(ctx)
	{
	}

	~EvalContext() {}

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
		void set(std::string name, ValuePtr);

		ValueMap leave();

		private:
		Scope(EvalContext& stack, std::string name, ValueMap&);

		EvalContext& stack_;
		std::string name_;
		ValueMap& symbols_;

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
		AlternateScoping(EvalContext&, std::deque<ValueMap>&&);

		EvalContext& stack_;
		std::deque<ValueMap> originalScopes_;

		friend class EvalContext;
	};

	AlternateScoping ChangeScopeStack(const ValueMap& alternativeScope);


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



	//! Define a named @ref dag::Value in the current scope.
	void Define(ScopedValueName& name, ValuePtr value);

	//! Look up a named value from the current scope or a parent scope.
	ValuePtr Lookup(const std::string& name);


	//! Create a @ref dag::Boolean.
	ValuePtr Bool(bool, SourceRange);

	//! Construct a @ref dag::Build from a @ref dag::Rule and parameters.
	std::shared_ptr<class Build>
	Build(std::shared_ptr<class Rule>, ValueMap, ConstPtrMap<Type>& paramTypes,
	      SourceRange);

	//! Create a @ref dag::File from a path.
	ValuePtr File(std::string fullPath,
	              const ValueMap& attributes, const FileType&,
	              const SourceRange& src = SourceRange::None());

	//! Create a @ref dag::File from a subdirectory and a filename.
	ValuePtr File(std::string subdir, std::string filename,
	              const ValueMap& attributes, const FileType&,
	              const SourceRange& src = SourceRange::None());

	//! Define a @ref dag::Function.
	ValuePtr Function(const ast::Function&, const SharedPtrVec<Parameter>&);

	//! Create a @ref dag::Integer.
	ValuePtr Integer(int, SourceRange);

	//! Create a @ref dag::Rule in the current scope.
	ValuePtr Rule(std::string command, const ValueMap& arguments,
	              const SharedPtrVec<Parameter>& parameters, const Type&,
	              const SourceRange& from = SourceRange::None());

	//! Create a @ref dag::String.
	ValuePtr String(const std::string&, SourceRange = SourceRange::None());

	//! Create a @ref dag::Structure.
	ValuePtr Struct(const std::vector<Structure::NamedValue>&,
	                const Type&, SourceRange);

	//! Create a @ref dag::Target using the current value name.
	ValuePtr Target(const std::shared_ptr<class Build>&);
	ValuePtr Target(const std::shared_ptr<class File>&);
	ValuePtr Target(const std::shared_ptr<class List>&);

	//! Create a new alias for an existing @ref dag::Target.
	void Alias(const std::shared_ptr<class Target>&);

protected:
	ValueMap& CurrentScope();
	ValueMap PopScope();

	//! Make a deep copy of the current scope and all of its parents.
	ValueMap CopyCurrentScope();

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
	std::deque<ValueMap> scopes_;

	// Values we've created:
	SharedPtrVec<class File> files_;
	SharedPtrVec<class Build> builds_;
	SharedPtrMap<class Rule> rules_;
	SharedPtrMap<class Value> variables_;
	SharedPtrMap<class Target> targets_;

private:
	/** The name of the value we are currently processing. */
	std::deque<std::string> currentValueName_;
};

} // namespace dag
} // namespace fabrique

#endif
