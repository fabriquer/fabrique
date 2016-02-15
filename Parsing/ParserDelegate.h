/** @file Parsing/ParserDelegate.h Declaration of @ref fabrique::parser::ParserDelegate. */
/*
 * Copyright (c) 2015, 2016 Jonathan Anderson
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

#ifndef PARSER_DELEGATE_H
#define PARSER_DELEGATE_H

#include "AST/forward-decls.h"
#include "Parsing/ErrorReporter.h"
#include "Support/ABI.h"
#include "Support/Bytestream.h"
#include "Support/ErrorReport.h"
#include "Support/String.h"

#include <memory>
#include <stack>


namespace fabrique {

class TypeContext;

namespace parser {

struct Grammar;


/**
 * Class that actually parses content by supplying callbacks to Pegmatite.
 */
class ParserDelegate : public pegmatite::ASTParserDelegate
{
	public:
	ParserDelegate(const Grammar&, TypeContext&, UniqPtrVec<ErrorReport>&);
	virtual ~ParserDelegate();

	/// Parse an input file and return an ast::Scope containing ast::Value objects.
	UniqPtr<ast::Scope> Parse(pegmatite::Input&, const ast::Scope& container);
	UniqPtr<ast::Value> ParseValue(pegmatite::Input&, const ast::Scope& container);

	ErrorReporter& errors() { return errors_; }

#if 0
	ast::Scope& CurrentScope();

	/**
	 * Enter an AST @ref Scope. Should be called before parsing anything
	 * that belongs in the scope, e.g. parameters:
	 *
	 * [EnterScope] function (x:int [Parameter]) { ... } [ExitScope]
	 *
	 * @param  name            a name used to describe the scope (for debugging)
	 * @param  argumentsType   the type of the arguments (if any) being passed
	 *                         into the scope: either a @ref RecordType or
	 *                         else @ref TypeContext::nilType()
	 * @param  src             the entire extent of the scope in source
	 */
	ast::Scope& EnterScope(const std::string& name, const Type& argumentsType,
	                  SourceRange src = SourceRange::None());

	//! A convenience wrapper around @ref #EnterScope with no 'args' type.
	ast::Scope& EnterScope(const std::string& name);

	/**
	 * Take an AST @ref Scope and push it on the stack.
	 */
	ast::Scope& EnterScope(ast::Scope&& s);

	/**
	 * Leave an AST @ref Scope, returning ownership of that scope
	 * (and, transitively, everything it contains).
	 */
	std::unique_ptr<ast::Scope> ExitScope();
#endif


	private:
	template<class ASTType>
	void BindType(const pegmatite::Rule& rule)
	{
		BindParser<typename ASTType::Parser>(rule);
	}

	template<class ParserType>
	void BindParser(const pegmatite::Rule& rule)
	{
		pegmatite::ErrorReporter err = pegErr();

		bind_parse_proc(rule, [err](const ParserInput& input, void *data)
		{
			auto &stack = *reinterpret_cast<ParserStack*>(data);

			Bytestream& dbg = Bytestream::Debug("parser.node");
			if (dbg)
			{
				SourceRange src(input);

				dbg
					<< Bytestream::Action << "parsing "
					<< Bytestream::Type << Demangle(typeid(ParserType))
					<< Bytestream::Operator << " «"
					<< Bytestream::Literal << input.str()
					<< Bytestream::Operator << "»:"
					<< Bytestream::Reset << "\n"
					;

				src.PrintSource(dbg, 0, 0);
				dbg
					<< Bytestream::Reset << " on stack:\n"
					;

				for (auto i = stack.rbegin(); i != stack.rend(); i++)
				{
					auto& node = *i->second;
					const std::string typeName = TypeName(node);

					dbg
						<< Bytestream::Operator << "   - "
						<< Bytestream::Type << typeName
						<< Bytestream::Reset << "\n"
						;
				}

				dbg
					<< Bytestream::Operator << "   ("
					<< Bytestream::Definition << "bottom of stack"
					<< Bytestream::Operator << ")"
					<< Bytestream::Reset << "\n"
					;
			}

			std::unique_ptr<pegmatite::ASTNode> parser(new ParserType());

			if (not parser->construct(input, stack, err))
				return false;

			if (dbg)
			{
				dbg
					<< Bytestream::Action << "parsed: "
					<< Bytestream::Type << TypeName(*parser)
					<< Bytestream::Reset << "\n\n"
					;
			}

			stack.emplace_back(input, std::move(parser));
			return true;
		});
	}

	pegmatite::ErrorReporter pegErr();


	/*
	using ParseFn = std::function<NodePtr (const Input&, Stack*)>;

	bool ParseNode(ParseFn, std::string name,
	               const Pos& begin, const Pos& end, void *stack);
	*/

#if 0
	NodePtr TypeRef(const Input&, Stack*);
	NodePtr ParametricTypeRef(const Input&, Stack*);

	//! Find or create a @ref Type.
	const Type& getType(const std::string& name,
	                    const SourceRange& begin, const SourceRange& end,
	                    const PtrVec<Type>& params = PtrVec<Type>());

	NodePtr True(const Input&, Stack*);
	NodePtr False(const Input&, Stack*);
	NodePtr Integer(const Input&, Stack*);
	NodePtr String(const Input&, Stack*);

	NodePtr UntypedIdentifier(const Input&, Stack*);
	NodePtr TypedIdentifier(const Input&, Stack*);
	NodePtr TypedId(const Input&, Stack*);

	NodePtr File(const Input&, Stack*);
	NodePtr Files(const Input&, Stack*);
	NodePtr FilesWithArgs(const Input&, Stack*);
	NodePtr Filename(const Input&, Stack*);

	NodePtr NamedArgument(const Input&, Stack*);
	NodePtr UnnamedArgument(const Input&, Stack*);

	NodePtr List(const Input&, Stack*);

	NodePtr Value(const Input&, Stack*);

	NodePtr ScopedValues(const Input&, Stack*);
#endif

	const Grammar& grammar_;
	TypeContext& types_;
	ErrorReporter errors_;

	std::stack<std::unique_ptr<ast::Scope>> scopes_;
};





#if 0
class Parser
{
public:
	Parser(TypeContext&, plugin::Registry&, plugin::Loader&, std::string srcroot);

	//! Parse Fabrique fragments defined at, e.g., the command line.
	const Type& ParseDefinitions(const std::vector<std::string>& defs);

	//! Parse Fabrique input (usually a file) into a @ref Scope.
	std::unique_ptr<Scope> ParseFile(
		std::istream& input, const Type& arguments, std::string name = "",
		StringMap<std::string> builtins = StringMap<std::string>(),
		SourceRange openedFrom = SourceRange::None());

	//! Errors encountered during parsing.
	const UniqPtrVec<ErrorReport>& errors() const { return errs_; }

	//! Input files encountered during parsing.
	const std::vector<std::string>& files() const { return files_; }


	//! Find or create a @ref Type.
	const Type& getType(const std::string& name,
	                    const SourceRange& begin, const SourceRange& end,
	                    const PtrVec<Type>& params = PtrVec<Type>());

	const Type& getType(UniqPtr<Identifier>&&,
	                    UniqPtr<const PtrVec<Type>>&& params = nullptr);

	const FunctionType& FnType(const PtrVec<Type>& inputs,
	                           const Type& output, SourceRange);

	const RecordType* CreateRecordType(UniqPtr<UniqPtrVec<Identifier>>& fields,
	                                   SourceRange);


	//! Define a build @ref Action.
	Action* DefineAction(UniqPtr<UniqPtrVec<Argument>>& args,
	                     const SourceRange&,
	                     UniqPtr<UniqPtrVec<Parameter>>&& params = nullptr);

	//! Parse an @ref Argument to a @ref Function, build @ref Action, etc.
	Argument* Arg(UniqPtr<Expression>& value,

			std::unique_ptr<typename ASTType::Parser> builder
			{
				new typename ASTType::Parser()
			};
			builder->construct(input, stack);

			stack.emplace_back(input, std::move(builder));
			return true;
		});
	}


	/*
	using ParseFn = std::function<NodePtr (const Input&, Stack*)>;

	bool ParseNode(ParseFn, std::string name,
	               const Pos& begin, const Pos& end, void *stack);
	*/

#if 0
	NodePtr TypeRef(const Input&, Stack*);
	NodePtr ParametricTypeRef(const Input&, Stack*);

	//! Find or create a @ref Type.
	const Type& getType(const std::string& name,
	                    const SourceRange& begin, const SourceRange& end,
	                    const PtrVec<Type>& params = PtrVec<Type>());

	NodePtr True(const Input&, Stack*);
	NodePtr False(const Input&, Stack*);
	NodePtr Integer(const Input&, Stack*);
	NodePtr String(const Input&, Stack*);

	NodePtr UntypedIdentifier(const Input&, Stack*);
	NodePtr TypedIdentifier(const Input&, Stack*);
	NodePtr TypedId(const Input&, Stack*);

	NodePtr File(const Input&, Stack*);
	NodePtr Files(const Input&, Stack*);
	NodePtr FilesWithArgs(const Input&, Stack*);
	NodePtr Filename(const Input&, Stack*);

	NodePtr NamedArgument(const Input&, Stack*);
	NodePtr UnnamedArgument(const Input&, Stack*);

	NodePtr List(const Input&, Stack*);

	NodePtr Value(const Input&, Stack*);

	NodePtr ScopedValues(const Input&, Stack*);
#endif

	ErrorReport& ReportError(const std::string&, const SourceRange&,
		ErrorReport::Severity = ErrorReport::Severity::Error);
	ErrorReport& ReportError(const std::string&, const HasSource&,
		ErrorReport::Severity = ErrorReport::Severity::Error);

	const Grammar& grammar_;
	TypeContext& types_;
	UniqPtrVec<ErrorReport>& errors_;

	std::stack<std::unique_ptr<ast::Scope>> scopes_;
};





#if 0
class Parser
{
public:
	Parser(TypeContext&, plugin::Registry&, plugin::Loader&, std::string srcroot);

	//! Parse Fabrique fragments defined at, e.g., the command line.
	const Type& ParseDefinitions(const std::vector<std::string>& defs);

	//! Parse Fabrique input (usually a file) into a @ref Scope.
	std::unique_ptr<Scope> ParseFile(
		std::istream& input, const Type& arguments, std::string name = "",
		StringMap<std::string> builtins = StringMap<std::string>(),
		SourceRange openedFrom = SourceRange::None());

	//! Errors encountered during parsing.
	const UniqPtrVec<ErrorReport>& errors() const { return errs_; }

	//! Input files encountered during parsing.
	const std::vector<std::string>& files() const { return files_; }


	//! Find or create a @ref Type.
	const Type& getType(const std::string& name,
	                    const SourceRange& begin, const SourceRange& end,
	                    const PtrVec<Type>& params = PtrVec<Type>());

	const Type& getType(UniqPtr<Identifier>&&,
	                    UniqPtr<const PtrVec<Type>>&& params = nullptr);

	const FunctionType& FnType(const PtrVec<Type>& inputs,
	                           const Type& output, SourceRange);

	const RecordType* CreateRecordType(UniqPtr<UniqPtrVec<Identifier>>& fields,
	                                   SourceRange);


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

	//! A call to an @ref Action or @ref Function.
	Call* CreateCall(UniqPtr<Expression>&, UniqPtr<UniqPtrVec<Argument>>&,
	                 const SourceRange& end);

	//! An expression that can (optionally) include intermediate values.
	CompoundExpression* CompoundExpr(UniqPtr<Expression>& result,
	                                 SourceRange beg = SourceRange::None(),
	                                 SourceRange end = SourceRange::None());

	//! An expression that indirects into a record.
	FieldAccess* FieldAccess(UniqPtr<Expression>& record,
	                         UniqPtr<Identifier>& field);

	//! A test to see if a record contains a field.
	FieldQuery* FieldQuery(UniqPtr<Expression>& record,
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
	 */
	ForeachExpr* Foreach(UniqPtr<Mapping>&, UniqPtr<Expression>& body,
	                     const SourceRange& start);


	/**
	 * A type declaration:
	 *   `x:type = type[foo:int, bar:string];`
	 */
	TypeDeclaration* DeclareType(const RecordType&, SourceRange);


	Function* DefineFunction(const SourceRange& begin,
	                         UniqPtr<UniqPtrVec<Parameter>>& params,
	                         UniqPtr<Expression>& body,
	                         const Type *ty = nullptr);


	//! An untyped @ref ast::Identifier: just a name.
	Identifier* Id(UniqPtr<Token>&&);

	//! A typed @ref ast::Identifier.
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

	//! Turn the current scope into a record instantiation.
	Record* Record(SourceRange);


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


	//! Create a @ref DebugTracePoint.
	DebugTracePoint* TracePoint(UniqPtr<Expression>&, SourceRange);


	//! Create a @ref UnaryOperation (currently just 'not').
	UnaryOperation* UnaryOp(UnaryOperation::Operator,
	                        const SourceRange& operatorLocation,
	                        UniqPtr<Expression>&);


	//! Define a @ref Value in the current scope.
	bool DefineValue(UniqPtr<Identifier>&, UniqPtr<Expression>&,
	                 bool builtin = false);

	//! Define an unnamed @ref Value in the current scope.
	bool DefineValue(UniqPtr<Expression>&);


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

	//! Define a builtin value.
	bool Builtin(std::string name, UniqPtr<Expression>& value, SourceRange);

	//! Convenience method for the full @ref #Builtin().
	bool Builtin(std::string name, int value, SourceRange);
	bool Builtin(std::string name, std::string value, SourceRange);
	bool Builtin(std::string name, UniqPtr<Scope>& scope, SourceRange);

	//! Add an @ref Argument vector to the current scope.
	void AddToScope(const PtrVec<Argument>&);

	const ErrorReport& ReportError(const std::string&, const SourceRange&,
		ErrorReport::Severity = ErrorReport::Severity::Error);
	const ErrorReport& ReportError(const std::string&, const HasSource&,
		ErrorReport::Severity = ErrorReport::Severity::Error);

	TypeContext& ctx_;
	Lexer& lexer_;

	plugin::Registry& pluginRegistry_;
	plugin::Loader& pluginLoader_;

	//! Pre-defined values (e.g., from the command line).
	UniqPtr<Scope> definitions_;

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
#endif
#endif

} // namespace parser
} // namespace fabrique

#endif
