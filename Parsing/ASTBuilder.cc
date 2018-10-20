/** @file Parsing/ASTBuilder.cc    Definition of @ref fabrique::ast::ASTBuilder. */
/*
 * Copyright (c) 2018 Jonathan Anderson
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
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "AST/ast.h"
#include "Parsing/ASTBuilder.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;
using std::make_unique;
using std::string;


ASTBuilder::ASTBuilder(std::string filename)
	: debug_(Bytestream::Debug("ast.parser")),
	  fullDebug_(Bytestream::Debug("ast.parser.detail")),
	  filename_(std::move(filename))
{
}

ASTBuilder::~ASTBuilder()
{
}


//
// Top-level file and values:
//

antlrcpp::Any ASTBuilder::visitFile(FabParser::FileContext *ctx)
{
	return visitChildren(ctx);
}

antlrcpp::Any ASTBuilder::visitValue(FabParser::ValueContext *ctx)
{
	ParseChildren(ctx);

	UniqPtr<Identifier> id;
	if (auto *name = ctx->name)
	{
		id = identifier(name);
	}

	auto e = pop<Expression>(ctx->expression());
	assert(e && "Value initializer is null");

	UniqPtr<TypeReference> explicitType;
	if (auto *t = ctx->type())
	{
		explicitType = pop<TypeReference>(t);
		check(explicitType, source(*t), "failed to parse value type");
	}

	return push<Value>(std::move(id), std::move(explicitType), std::move(e));
}

UniqPtrVec<Value> ASTBuilder::takeValues()
{
	UniqPtrVec<Value> values = popChildren<Value>(nullptr);
	assert(nodes_.empty());

	return values;
}


//
// Expressions:
//

antlrcpp::Any ASTBuilder::visitExpression(FabParser::ExpressionContext *ctx)
{
	ParseChildren(ctx);

	auto subexprs = ctx->expression();
	if (subexprs.empty())
	{
		return true;
	}
	else if (subexprs.size() == 1)
	{
		// TODO
		return false;
	}

	// Otherwise: binary operation
	check(subexprs.size() == 2, ctx, "must be a binary operation");
	auto rhs = pop<Expression>(subexprs[1]);
	auto lhs = pop<Expression>(subexprs[0]);

	BinaryOperation::Operator op = BinaryOperation::Operator::Invalid;

	if (auto *multOp = ctx->multOp())
	{
		op = BinaryOperation::Op(multOp->getText());
	}
	else if (auto *addOp = ctx->addOp())
	{
		op = BinaryOperation::Op(addOp->getText());
	}
	else if (auto *compareOp = ctx->compareOp())
	{
		op = BinaryOperation::Op(compareOp->getText());
	}
	else if (auto *logicOp = ctx->logicOp())
	{
		op = BinaryOperation::Op(logicOp->getText());
	}

	return push<BinaryOperation>(std::move(lhs), std::move(rhs), op, source(*ctx));
}

antlrcpp::Any ASTBuilder::visitCall(FabParser::CallContext *ctx)
{
	ParseChildren(ctx);

	UniqPtr<Arguments> args;
	if (auto *a = ctx->arguments())
	{
		args = pop<Arguments>(a);
	}

	auto target = pop<Expression>(ctx->target);

	return push<Call>(std::move(target), std::move(args), source(*ctx));
}

antlrcpp::Any ASTBuilder::visitConditional(FabParser::ConditionalContext *ctx)
{
	ParseChildren(ctx);

	auto elseClause = pop<Expression>(ctx->elseClause);
	auto thenClause = pop<Expression>(ctx->thenClause);
	auto condition = pop<Expression>(ctx->condition);

	return push<Conditional>(std::move(condition), std::move(thenClause),
	                         std::move(elseClause), source(*ctx));
}

antlrcpp::Any ASTBuilder::visitForeach(FabParser::ForeachContext *ctx)
{
	ParseChildren(ctx);

	auto body = pop<Expression>(ctx->body);
	auto src = pop<Expression>(ctx->src);

	UniqPtr<TypeReference> explicitType;
	if (auto *t = ctx->type())
	{
		explicitType = pop<TypeReference>(t);
	}

	auto loopVarName = identifier(ctx->loopVarName);

	return push<ForeachExpr>(std::move(loopVarName), std::move(explicitType),
	                         std::move(src), std::move(body), source(*ctx));
}

antlrcpp::Any ASTBuilder::visitFunction(FabParser::FunctionContext *ctx)
{
	ParseChildren(ctx);

	auto body = pop<Expression>(ctx->body);

	UniqPtr<TypeReference> resultType;
	if (auto *t = ctx->type())
	{
		resultType = pop<TypeReference>(t);
	}
	check(resultType, source(*ctx), "missing result type");

	auto params = popChildren<Parameter>(ctx->parameters());

	return push<Function>(std::move(params), std::move(resultType), std::move(body),
	                      source(*ctx));
}


//
// Terms:
//

antlrcpp::Any ASTBuilder::visitBuildAction(FabParser::BuildActionContext *ctx)
{
	ParseChildren(ctx);

	auto parameters = popChildren<Parameter>(ctx->parameters());
	auto args = pop<Arguments>(ctx->arguments());

	return push<Action>(std::move(args), std::move(parameters), source(*ctx));
}

antlrcpp::Any ASTBuilder::visitCompoundExpr(FabParser::CompoundExprContext *ctx)
{
	ParseChildren(ctx);

	auto result = pop<Expression>(ctx->result);
	check(result, ctx->result, "compound expression has no result");

	auto values = popChildren<Value>(ctx);

	return push<CompoundExpression>(std::move(values), std::move(result),
	                                source(*ctx));
}

antlrcpp::Any ASTBuilder::visitFileList(FabParser::FileListContext *ctx)
{
	ParseChildren(ctx);

	UniqPtrVec<FilenameLiteral> files;
	for (auto *f : ctx->files)
	{
		files.emplace_back(make_unique<FilenameLiteral>(f->getText(), source(*f)));
	}

	UniqPtrVec<Argument> args;
	if (auto *kwargs = ctx->keywordArguments())
	{
		args = popChildren<Argument>(kwargs);
	}

	return push<FileList>(std::move(files), std::move(args), source(*ctx));
}

antlrcpp::Any ASTBuilder::visitList(FabParser::ListContext *ctx)
{
	visitChildren(ctx);

	UniqPtrVec<Expression> expressions = popChildren<Expression>(ctx);

	return push<List>(std::move(expressions), source(*ctx));
}

antlrcpp::Any ASTBuilder::visitLiteral(FabParser::LiteralContext *ctx)
{
	SourceRange src = source(*ctx);

	if (auto *b = ctx->BoolLiteral())
	{
		string text = b->getText();

		check(text == "true" or text == "false", src,
		              "boolean literal must be 'true' or 'false'");

		bool value = (text == "true");

		return push<BoolLiteral>(value, src);
	}
	else if (auto *i = ctx->IntLiteral())
	{
		return push<IntLiteral>(stoi(i->getText()), src);
	}
	else if (auto *s = ctx->StringLiteral())
	{
		string quoted = s->getText();

		check(quoted.length() >= 2, src,
		              "string literal must have at least two characters: quotes");

		check(quoted.front() == quoted.back(), src,
		              "quotes around string literal must match");

		string value = quoted.substr(1, quoted.length() - 2);

		return push<StringLiteral>(value, src);
	}

	assert(false && "literal must be bool, int or string");
	return false;
}

antlrcpp::Any ASTBuilder::visitNameReference(FabParser::NameReferenceContext *ctx)
{
	string name = ctx->Identifier()->getText();
	UniqPtr<Identifier> id(new Identifier(name, source(*ctx)));

	return push<NameReference>(std::move(id));
}


//
// Arguments and parameters:
//

antlrcpp::Any ASTBuilder::visitArguments(FabParser::ArgumentsContext *ctx)
{
	ParseChildren(ctx);

	UniqPtrVec<Argument> kwargs;
	if (auto *kwctx = ctx->keywordArguments())
	{
		kwargs = popChildren<Argument>(kwctx);
	}

	UniqPtrVec<Expression> positionalArgs;
	if (auto *posctx = ctx->positionalArguments())
	{
		positionalArgs = popChildren<Expression>(posctx);
	}

	return push<Arguments>(std::move(positionalArgs), std::move(kwargs), source(*ctx));
}

antlrcpp::Any ASTBuilder::visitKeywordArgument(FabParser::KeywordArgumentContext *ctx)
{
	ParseChildren(ctx);

	auto id = identifier(ctx->Identifier());
	auto initializer = pop<Expression>(ctx->expression());

	return push<Argument>(std::move(id), std::move(initializer));
}

antlrcpp::Any ASTBuilder::visitParameter(FabParser::ParameterContext *ctx)
{
	ParseChildren(ctx);

	UniqPtr<Expression> defaultArgument;
	if (auto *def = ctx->defaultArgument)
	{
		defaultArgument = pop<Expression>(def);
	}

	auto *t = ctx->type();
	auto type = pop<TypeReference>(t);
	check(type, t, "failed to parse parameter type");

	auto id = identifier(ctx->Identifier());

	return push<Parameter>(std::move(id), std::move(type), std::move(defaultArgument));
}


//
// Types:
//

antlrcpp::Any ASTBuilder::visitFieldType(FabParser::FieldTypeContext *ctx)
{
	ParseChildren(ctx);

	return false;
}

antlrcpp::Any ASTBuilder::visitFunctionType(FabParser::FunctionTypeContext *ctx)
{
	ParseChildren(ctx);

	check(ctx->result, source(*ctx), "no result type");
	auto resultType = pop<TypeReference>(ctx->result);

	UniqPtrVec<TypeReference> paramTypes;
	if (auto *p = ctx->params)
	{
		paramTypes = popChildren<TypeReference>(p);
	}

	return push<FunctionTypeReference>(std::move(paramTypes), std::move(resultType),
	                                   source(*ctx));
}

antlrcpp::Any ASTBuilder::visitParametricType(FabParser::ParametricTypeContext *ctx)
{
	ParseChildren(ctx);

	auto params = popChildren<TypeReference>(ctx->params);
	auto base = pop<TypeReference>(ctx->base);
	check(base, ctx->base, "failed to parse parametric type base");

	return push<ParametricTypeReference>(std::move(base), source(*ctx),
	                                     std::move(params));
}

antlrcpp::Any ASTBuilder::visitSimpleType(FabParser::SimpleTypeContext *ctx)
{
	return push<SimpleTypeReference>(identifier(ctx->Identifier()), source(*ctx));
}

antlrcpp::Any ASTBuilder::visitRecordType(FabParser::RecordTypeContext*)
{
	return false;
}


void ASTBuilder::ParseChildren(antlr4::ParserRuleContext *ctx)
{
	check(static_cast<bool>(visitChildren(ctx)), source(*ctx),
	      "failed to parse at least one child AST node");
}


std::unique_ptr<Identifier> ASTBuilder::identifier(antlr4::Token *token)
{
	return make_unique<Identifier>(token->getText(), source(*token));
}


std::unique_ptr<Identifier> ASTBuilder::identifier(antlr4::tree::TerminalNode *node)
{
	return identifier(node->getSymbol());
}


bool ASTBuilder::push(std::unique_ptr<Node> node)
{
	debug_
		<< Bytestream::Operator << "<<< "
		<< Bytestream::Action << "parsed " << node->source()
		<< Bytestream::Operator << " : "
		<< Bytestream::Type << TypeName(*node)
		<< "\n"
		;

	node->source().PrintSource(fullDebug_);

	fullDebug_
		<< Bytestream::Action << "result: "
		<< Bytestream::Reset << *node << "\n\n"
		;

	nodes_.emplace(std::move(node));
	return true;
}

std::unique_ptr<Node> ASTBuilder::popNode(SourceRange range)
{
	if (nodes_.empty())
	{
		return {};
	}

	if (range and not nodes_.top()->source().isInside(range))
	{
		return {};
	}

	std::unique_ptr<Node> node = std::move(nodes_.top());
	nodes_.pop();

	debug_
		<< Bytestream::Operator << ">>> "
		<< Bytestream::Action << "popped "
		<< Bytestream::Type << TypeName(*node)
		<< Bytestream::Operator << ": "
		<< Bytestream::Reset << "\n\t"
		;

	node->PrettyPrint(debug_, 1);
	debug_ << "\n\n";

	return node;
}


SourceRange ASTBuilder::source(antlr4::tree::TerminalNode &node)
{
	return source(*node.getSymbol());
}

SourceRange ASTBuilder::source(const antlr4::ParserRuleContext &ctx)
{
	SourceLocation begin(filename_, ctx.start->getLine(),
	                     ctx.start->getCharPositionInLine() + 1);

	SourceLocation end(filename_, ctx.stop->getLine(),
	                   ctx.stop->getCharPositionInLine() + ctx.stop->getText().length() + 1);

	return SourceRange(begin, end);
}

SourceRange ASTBuilder::source(const antlr4::Token &t)
{
	size_t line = t.getLine();
	size_t col = t.getCharPositionInLine() + 1;
	size_t length = t.getText().length();    // TODO: a better way?

	SourceLocation begin(filename_, line, col);
	SourceLocation end(filename_, line, col + length);

	return SourceRange(begin, end);
}
