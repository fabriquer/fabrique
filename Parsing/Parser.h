/** @file Lexer.h    Declaration of @ref Lexer. */
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

#ifndef PARSER_H
#define PARSER_H

#include "AST/Scope.h"
#include "AST/ast.h"
#include "Support/ErrorReport.h"

#include <map>
#include <stack>

namespace fabrique {

class Lexer;

namespace ast {


/**
 * Parses
 */
class Parser
{
public:
	Parser(const Lexer& lex);
	~Parser();

	const PtrVec<ErrorReport>& errors() const { return errs; }

	void EnterScope();

	SourceRange* CurrentTokenRange() const;

	//! Save the current token's location for future reference.
	void SaveLoc();
	const SourceRange& LastSavedLocation() const { return savedLoc; }

	//! Save the current token's type for future reference.
	void SaveType(const Type&);

	void SetRoot(PtrVec<Value>*);
	const Scope& getRoot() const { return root; }


	//! Find or create a @ref Type.
	const Type* getType(const std::string& name,
	                    const PtrVec<Type>& params = PtrVec<Type>());

	const Type* getType(const std::string& name, const Type& typeParam);

	/**
	 * Find or create a @ref Type, taking ownership of the name and
	 * type parameter vector. We already own the elements of this vector.
	 */
	const Type* TakeType(Identifier*, const PtrVec<Type>* params = NULL);


	//! Define a @ref Value in the current scope.
	Value* Define(Identifier*, Expression*);

	//! Reference a @ref Value in scope.
	SymbolReference* Reference(Identifier*);


	List* ListOf(ExprVec* elements);

	CompoundExpression* CompoundExpr(Expression *result,
	                                 SourceRange *begin = NULL,
	                                 PtrVec<Value> *values = NULL);


	// actions and functions
	Action* DefineAction(PtrVec<Argument>* args, SourceRange*,
	                     PtrVec<Parameter>* params = NULL);
	Function* DefineFunction(PtrVec<Parameter> *params, const Type *ty,
                                 CompoundExpression *body);

	Call* CreateCall(Identifier*, PtrVec<Argument>*);


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
	Conditional* IfElse(SourceRange *ifLocation, Expression *condition,
	                    Expression *thenResult, Expression *elseResult);

	/**
	 * An expression for mapping list elements into another list:
	 *   foreach x in some_list: x + 1
	 * .
	 */
	ForeachExpr* Foreach(Expression *source, const Parameter *loopParam,
	                     CompoundExpression *body, SourceRange *start);

	//! A foreach loop iteration parameter, whose type can be inferred.
	Parameter* ForeachParam(Identifier*);

	// files
	/**
	 * Create a source file, which is expected to be present on disk when
	 * fab is run.
	 */
	Filename* Source(Expression *name, SourceRange *source,
	                 PtrVec<Argument> *arguments = NULL);

	/**
	 * Create a target file, which only exists at build time as a result
	 * of a build action.
	 */
	Filename* Target(Expression *name, SourceRange *source,
	                 PtrVec<Argument> *arguments = NULL);

	/** Create a list of files, which may have shared arguments. */
	FileList* Files(PtrVec<Filename>*, PtrVec<Argument> *args = NULL);


	// binary operations
	BinaryOperation* Concat(Expression*, Expression*);
	BinaryOperation* Prefix(Expression*, Expression*);
	BinaryOperation* ScalarAdd(Expression*, Expression*);

	//! Parse an @ref Argument to a @ref Function, build @ref Action, etc.
	Argument* Arg(Expression *e, Identifier *name = NULL);

	//! Parse a function @ref Parameter.
	Parameter* Param(Identifier*, Expression *defaultValue = NULL);

	//! Add an @ref Argument vector to the current scope.
	void AddToScope(const PtrVec<Argument>&);

	//! Parse an @ref Identifier.
	Identifier* Id(const std::string&);

	//! Parse an @ref Identifier from an untyped name + a type.
	Identifier* Id(Identifier *untyped, const Type*);

	// literals
	BoolLiteral* True();
	BoolLiteral* False();
	IntLiteral* ParseInt(int);
	StringLiteral* ParseString(const std::string&);


private:
	const Type* fileType();
	const Type* fileListType();

	const ErrorReport& ReportError(const std::string&, const SourceRange&);
	const ErrorReport& ReportError(const std::string&, const HasSource&);

	Scope& CurrentScope();
	void ExitScope();

	const Lexer& lex;
	SourceRange savedLoc;

	Type const *savedType = NULL;

	PtrVec<ErrorReport> errs;

	typedef std::pair<std::string,PtrVec<Type> > TypeName;
	std::map<TypeName,Type*> types;

	Scope root;
	std::stack<Scope*> scopes;
};

} // namespace ast
} // namespace fabrique

#endif
