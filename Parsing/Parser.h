/** @file Parsing/Parser.h    Declaration of @ref Parser. */
/*
 * Copyright (c) 2013-2014 Jonathan Anderson
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

#ifndef PARSER_H
#define PARSER_H

#include "AST/ast.h"
#include "ADT/UniqPtr.h"
#include "Support/ErrorReport.h"

#include <map>
#include <stack>

namespace fabrique { class Token; }
#include "Parsing/fab.yacc.h"

namespace fabrique {

class TypeContext;
class Lexer;

namespace ast {


/**
 * Parses
 */
class Parser
{
public:
	Parser(TypeContext&, std::string srcroot);

	//! Parse Fabrique fragments defined at, e.g., the command line.
	UniqPtr<Scope> ParseDefinitions(const std::vector<std::string>& fragments);

	//! Parse Fabrique input (usually a file) into a @ref Scope.
	std::unique_ptr<Scope> ParseFile(
		std::istream&, UniqPtr<Scope>& arguments, std::string name = "",
		StringMap<std::string> builtins = StringMap<std::string>(),
		SourceRange openedFrom = SourceRange::None());

	//! Errors encountered during parsing.
	const UniqPtrVec<ErrorReport>& errors() const { return errs_; }

	//! Input files encountered during parsing.
	const std::vector<std::string>& files() const { return files_; }


	/**
	 * Enter an AST @ref Scope. Should be called before parsing anything
	 * that belongs in the scope, e.g. parameters:
	 *
	 * <<EnterScope>> function (x:int <<Parameter>>) { ... } <<ExitScope>>
	 *
	 * @param  name    a name used to describe the scope (for debugging)
	 */
	Scope& EnterScope(const std::string& name);

	/**
	 * Take an AST @ref Scope and push it on the stack.
	 */
	Scope& EnterScope(Scope&& s);


	//! Find or create a @ref Type.
	const Type& getType(const std::string& name,
	                    const SourceRange& begin, const SourceRange& end,
	                    const PtrVec<Type>& params = PtrVec<Type>());

	const Type& getType(UniqPtr<Identifier>&&,
	                    UniqPtr<const PtrVec<Type>>&& params = nullptr);

	const FunctionType& FnType(const PtrVec<Type>& inputs,
	                           const Type& output, SourceRange);


	//! Define a build @ref Action.
	Action* DefineAction(UniqPtr<UniqPtrVec<Argument>>& args,
	                     const SourceRange&,
	                     UniqPtr<UniqPtrVec<Parameter>>&& params = nullptr);

	//! Parse an @ref Argument to a @ref Function, build @ref Action, etc.
	Argument* Arg(UniqPtr<Expression>& value,
	              UniqPtr<Identifier>&& = nullptr);

	//! Create a @ref BinaryOperation (+, ::, ...).
	BinaryOperation* BinaryOp(BinaryOperation::Operator,
	                          UniqPtr<Expression>&, UniqPtr<Expression>&);

	bool Builtin(const std::string& name, int value);
	bool Builtin(const std::string& name, const std::string& value);

	//! A call to an @ref Action or @ref Function.
	Call* CreateCall(UniqPtr<Expression>&, UniqPtr<UniqPtrVec<Argument>>&,
	                 const SourceRange& end);

	//! An expression that can (optionally) include intermediate values.
	CompoundExpression* CompoundExpr(UniqPtr<Expression>& result,
	                                 SourceRange beg = SourceRange::None(),
	                                 SourceRange end = SourceRange::None());

	//! An expression that indirects into a structure.
	FieldAccess* FieldAccess(UniqPtr<Expression>& structure,
	                         UniqPtr<Identifier>& field);

	//! A test to see if a structure contains a field.
	FieldQuery* FieldQuery(UniqPtr<Expression>& structure,
	                       UniqPtr<Identifier>& field,
	                       UniqPtr<Expression>& defaultValue,
	                       SourceRange);

	//! A @ref Filename that is part of the build DAG.
	Filename* File(UniqPtr<Expression>& name, const SourceRange& src,
	               UniqPtr<UniqPtrVec<Argument>>&& arguments = nullptr);

	/** Create a list of files, which may have shared arguments. */
	FileList* Files(const SourceRange&, UniqPtr<UniqPtrVec<Filename>>&,
	                UniqPtr<UniqPtrVec<Argument>>&& args = nullptr);


	/**
	 * An expression for mapping list elements into another list:
	 *   foreach x in some_list: x + 1
	 * .
	 */
	ForeachExpr* Foreach(UniqPtr<Mapping>&, UniqPtr<Expression>& body,
	                     const SourceRange& start);


	Function* DefineFunction(const SourceRange& begin,
	                         UniqPtr<UniqPtrVec<Parameter>>& params,
	                         UniqPtr<Expression>& body,
	                         const Type *ty = nullptr);


	//! An untyped @ref Identifier: just a name.
	Identifier* Id(UniqPtr<Token>&&);

	//! A typed @ref Identifier.
	Identifier* Id(UniqPtr<Identifier>&& untyped, const Type*);

	//! An expression that imports a Fabrique module.
	Import* ImportModule(UniqPtr<StringLiteral>& name,
	                     UniqPtrVec<Argument>& arguments, SourceRange);

	/**
	 * A conditional if-then-else expression
	 * (not a statement, an expression).
	 *
	 * The expression (if condition foo else bar) evaluates to either
	 * @a foo or @a bar, depending on @a condition.
	 * It can also be writen as:
	 *
	 * if (condition)
	 *     foo
	 * else
	 *     bar
	 */
	Conditional* IfElse(const SourceRange& ifLocation,
	                    UniqPtr<Expression>& condition,
	                    UniqPtr<Expression>& thenResult,
	                    UniqPtr<Expression>& elseResult);

	//! Define a @ref List of expressions.
	List* ListOf(UniqPtrVec<Expression>&, const SourceRange&);

	//! Define a mapping from a sequence to a name.
	Mapping* Map(UniqPtr<Expression>& source, UniqPtr<Identifier>& target);

	//! Create a @ref SomeValue (populated maybe object).
	SomeValue* Some(UniqPtr<Expression>&, SourceRange);

	//! Turn the current scope into a structure instantiation.
	StructInstantiation* StructInstantiation(SourceRange);


	// literals
	BoolLiteral* True();
	BoolLiteral* False();
	IntLiteral* ParseInt(int);
	StringLiteral* ParseString(UniqPtr<Token>&&);


	//! Parse a function @ref Parameter.
	Parameter* Param(UniqPtr<Identifier>&& name,
	                 UniqPtr<Expression>&& defaultValue = nullptr);


	//! Reference a @ref Value in scope.
	SymbolReference* Reference(UniqPtr<Identifier>&&);
	SymbolReference* Reference(UniqPtr<class FieldAccess>&&);


	//! Create a @ref UnaryOperation (currently just 'not').
	UnaryOperation* UnaryOp(UnaryOperation::Operator,
	                        const SourceRange& operatorLocation,
	                        UniqPtr<Expression>&);


	//! Define a @ref Value in the current scope.
	bool DefineValue(UniqPtr<Identifier>&, UniqPtr<Expression>&);


	//
	// Low-level but type-safe getters and setters for the YYSTYPE union:
	//
	static Token* ParseToken(YYSTYPE&);
	static bool Set(YYSTYPE&, Node*);


private:
	Scope& CurrentScope();

	/**
	 * Leave an AST @ref Scope, returning ownership of that scope
	 * (and, transitively, everything it contains).
	 */
	std::unique_ptr<Scope> ExitScope();

	//! Add an @ref Argument vector to the current scope.
	void AddToScope(const PtrVec<Argument>&);

	const ErrorReport& ReportError(const std::string&, const SourceRange&);
	const ErrorReport& ReportError(const std::string&, const HasSource&);

	TypeContext& ctx_;
	Lexer& lexer_;

	//! Input files, in order they were parsed.
	std::vector<std::string> files_;
	UniqPtrVec<ErrorReport> errs_;
	std::stack<std::unique_ptr<Scope>> scopes_;

	//! The root of all source files (where the top-level Fabrique file lives).
	std::string srcroot_;

	//! The subdirectory that we are currently working from.
	std::stack<std::string> currentSubdirectory_;
};

} // namespace ast
} // namespace fabrique

#endif
