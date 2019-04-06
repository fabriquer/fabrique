/** @file parsing/ASTBuilder.hh    Declaration of @ref fabrique::parsing::ASTBuilder. */
/*
 * Copyright (c) 2018-2019 Jonathan Anderson
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

#ifndef FAB_PARSING_PARSE_TREE_VISITOR_H_
#define FAB_PARSING_PARSE_TREE_VISITOR_H_

#include <fabrique/AssertionFailure.hh>
#include <fabrique/ast/ast.hh>
#include <fabrique/parsing/ParserError.hh>
#include <fabrique/platform/ABI.hh>

#include <generated-grammar/FabParserBaseVisitor.h>

#include <stack>

namespace fabrique {

class Bytestream;

namespace parsing {

/**
 * A parse tree builder that can be used to generate an AST.
 */
class ASTBuilder : public FabParserBaseVisitor
{
public:
	using Any = antlrcpp::Any;

	ASTBuilder(std::string filename);
	virtual ~ASTBuilder() override;

	//
	// Top-level file and values:
	//
	antlrcpp::Any visitFile(FabParser::FileContext*) override;
	antlrcpp::Any visitValue(FabParser::ValueContext*) override;
	UniqPtrVec<ast::Value> takeValues();

	//
	// Expressions:
	//
	antlrcpp::Any visitExpression(FabParser::ExpressionContext*) override;
	antlrcpp::Any visitConditional(FabParser::ConditionalContext*) override;
	antlrcpp::Any visitForeach(FabParser::ForeachContext*) override;
	antlrcpp::Any visitFunction(FabParser::FunctionContext*) override;
	antlrcpp::Any visitUnaryOperation(FabParser::UnaryOperationContext*) override;

	//
	// Terms:
	//
	antlrcpp::Any visitTerm(FabParser::TermContext*) override;
	antlrcpp::Any visitBuildAction(FabParser::BuildActionContext*) override;
	antlrcpp::Any visitCompoundExpr(FabParser::CompoundExprContext*) override;
	antlrcpp::Any visitFieldQuery(FabParser::FieldQueryContext*) override;
	antlrcpp::Any visitFileList(FabParser::FileListContext*) override;
	antlrcpp::Any visitList(FabParser::ListContext*) override;
	antlrcpp::Any visitLiteral(FabParser::LiteralContext*) override;
	antlrcpp::Any visitNameReference(FabParser::NameReferenceContext*) override;
	antlrcpp::Any visitRecord(FabParser::RecordContext*) override;
	antlrcpp::Any visitTypeDeclaration(FabParser::TypeDeclarationContext*) override;

	//
	// Arguments and parameters:
	//
	antlrcpp::Any visitArguments(FabParser::ArgumentsContext*) override;
	antlrcpp::Any visitKeywordArgument(FabParser::KeywordArgumentContext*) override;
	antlrcpp::Any visitParameter(FabParser::ParameterContext*) override;

	//
	// Types:
	//
	antlrcpp::Any visitFieldType(FabParser::FieldTypeContext*) override;
	antlrcpp::Any visitFunctionType(FabParser::FunctionTypeContext*) override;
	antlrcpp::Any visitParametricType(FabParser::ParametricTypeContext*) override;
	antlrcpp::Any visitSimpleType(FabParser::SimpleTypeContext*) override;
	antlrcpp::Any visitRecordType(FabParser::RecordTypeContext*) override;

	antlrcpp::Any defaultResult() override { return true; }

private:
	std::unique_ptr<ast::Identifier> identifier(antlr4::Token*);
	std::unique_ptr<ast::Identifier> identifier(antlr4::tree::TerminalNode*);

	using RuleContext = antlr4::ParserRuleContext;

	//! Assert that some parsing condition is true.
	template<typename T>
	static void check(const T &condition, SourceRange src, std::string message)
	{
		if (not condition)
		{
			throw ParserError(std::move(message), src);
		}
	}

	template<typename T>
	void check(const T &condition, RuleContext *ctx, std::string message)
	{
		check(condition, source(*ctx), std::move(message));
	}

	//! Parse all child AST nodes
	void ParseChildren(antlr4::ParserRuleContext*);

	/**
	 * Push an AST node onto the current AST-building stack.
	 */
	bool push(std::unique_ptr<ast::Node>);

	/**
	 * Forward values into a new AST node and push it onto the stack.
	 */
	template<class T, typename... Args>
	bool push(Args... args)
	{
		return push(std::make_unique<T>(std::forward<Args>(args)...));
	}


	/**
	 * Remove an AST node from the top of the stack _iff_ it falls in a certain range.
	 *
	 * @param   range     If not SourceRange::None(), this is the range in which
	 *                    the value at the top of the stack is expected to be found.
	 *                    Otherwise, an empty unique_ptr will be returned.
	 */
	std::unique_ptr<ast::Node> popNode(SourceRange range = SourceRange::None());

	/**
	 * Pop an AST node of a specific type from the stack.
	 *
	 * @param   range     Works as with `popNode(range)`.
	 *
	 * @pre     the top of the stack is of type T
	 */
	template<typename T>
	std::unique_ptr<T> pop(SourceRange range)
	{
		std::unique_ptr<ast::Node> top = popNode(range);

		check(top, range, "syntax error");

		check(dynamic_cast<T*>(top.get()), top->source(),
			platform::TypeName(*top) + " is not a "
			+ platform::Demangle(typeid(T)) + " (internal parser error)");

		return std::unique_ptr<T>(dynamic_cast<T*>(top.release()));
	}

	template<typename T, typename ToSrc>
	std::unique_ptr<T> pop(ToSrc *ctx)
	{
		FAB_ASSERT(ctx, "popping empty source context from AST-building stack");
		return pop<T>(source(*ctx));
	}

	/**
	 * Pop all of the children at the top of the stack that are found within a
	 * specific source code range.
	 *
	 * @param    ctx     Works as with `popNode(range)`.
	 *
	 * @pre      all nodes on top of the stack that fall within the specified context
	 *           are of type T
	 */
	template<typename T>
	UniqPtrVec<T> popChildren(RuleContext *ctx)
	{
		UniqPtrVec<T> children;
		while (not nodes_.empty())
		{
			SourceRange range = SourceRange::None();
			if (ctx)
			{
				range = source(*ctx);
				if (not nodes_.top()->source().isInside(range))
				{
					break;
				}
			}

			children.push_back(pop<T>(range));
			assert(children.back());
		}

		std::reverse(children.begin(), children.end());
		return children;
	}

	SourceRange source(antlr4::tree::TerminalNode&);
	SourceRange source(const antlr4::ParserRuleContext&);
	SourceRange source(const antlr4::Token&);

	Bytestream& debug_;
	Bytestream& fullDebug_;

	const std::string filename_;
	std::stack<std::unique_ptr<ast::Node>> nodes_;
};

} // namespace parsing
} // namespace fabrique

#endif
