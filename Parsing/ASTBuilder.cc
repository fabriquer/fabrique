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

	UniqPtr<Identifier> id(new Identifier(ctx->name->getText(), source(*ctx->name)));

	auto e = pop<Expression>(source(*ctx->expression()));
	assert(e && "Value initializer is null");

	UniqPtr<TypeReference> explicitType;
	if (auto *t = ctx->type())
	{
		SourceRange src = source(*t);
		explicitType = pop<TypeReference>(src);
		check(explicitType, src, "failed to parse value type");
	}

	return push<Value>(std::move(id), std::move(explicitType), std::move(e));
}

UniqPtrVec<Value> ASTBuilder::takeValues()
{
	UniqPtrVec<Value> values = popChildren<Value>();
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

	SourceRange src = source(*ctx);

	// Otherwise: binary operation
	check(subexprs.size() == 2, src, "must be a binary operation");
	auto rhs = pop<Expression>(source(*subexprs[1]));
	auto lhs = pop<Expression>(source(*subexprs[0]));

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

	return push<BinaryOperation>(std::move(lhs), std::move(rhs), op, src);
}

antlrcpp::Any ASTBuilder::visitCall(FabParser::CallContext *ctx)
{
	ParseChildren(ctx);

	auto args = pop<Arguments>(source(*ctx->arguments()));
	auto target = pop<Expression>(source(*ctx->target));

	return push<Call>(std::move(target), std::move(args), source(*ctx));
}

antlrcpp::Any ASTBuilder::visitForeach(FabParser::ForeachContext *ctx)
{
	ParseChildren(ctx);

	auto body = pop<Expression>(source(*ctx->body));
	auto src = pop<Expression>(source(*ctx->src));

	UniqPtr<TypeReference> explicitType;
	if (auto *t = ctx->type())
	{
		explicitType = pop<TypeReference>(source(*t));
	}

	auto loopVarName = std::make_unique<Identifier>(ctx->loopVarName->getText(),
	                                                source(*ctx->loopVarName));

	return push<ForeachExpr>(std::move(loopVarName), std::move(explicitType),
	                         std::move(src), std::move(body), source(*ctx));
}

antlrcpp::Any ASTBuilder::visitFunction(FabParser::FunctionContext *ctx)
{
	ParseChildren(ctx);

	auto body = pop<Expression>(source(*ctx->body));

	UniqPtr<TypeReference> resultType;
	if (auto *t = ctx->type())
	{
		resultType = pop<TypeReference>(source(*t));
	}

	auto params = popChildren<Parameter>(source(*ctx->parameters()));

	return false;
}


//
// Terms:
//

antlrcpp::Any ASTBuilder::visitBuildAction(FabParser::BuildActionContext *ctx)
{
	ParseChildren(ctx);

	auto parameters = popChildren<Parameter>(source(*ctx->parameters()));
	auto args = pop<Arguments>(source(*ctx->arguments()));

	return push<Action>(std::move(args), std::move(parameters), source(*ctx));
}

antlrcpp::Any ASTBuilder::visitCompoundExpr(FabParser::CompoundExprContext *ctx)
{
	ParseChildren(ctx);

	SourceRange src = source(*ctx->result);
	auto result = pop<Expression>(src);
	check(result, src, "compound expression has no result");

	auto values = popChildren<Value>(source(*ctx));

	return push<CompoundExpression>(std::move(values), std::move(result), src);
}

antlrcpp::Any ASTBuilder::visitFileList(FabParser::FileListContext *ctx)
{
	ParseChildren(ctx);

	UniqPtrVec<FilenameLiteral> files;
	for (auto *f : ctx->files)
	{
		files.emplace_back(std::make_unique<FilenameLiteral>(f->getText(), source(*f)));
	}

	UniqPtrVec<Argument> args;
	if (auto *kwargs = ctx->keywordArguments())
	{
		args = popChildren<Argument>(source(*kwargs));
	}

	return push<FileList>(std::move(files), std::move(args), source(*ctx));
}

antlrcpp::Any ASTBuilder::visitList(FabParser::ListContext *ctx)
{
	visitChildren(ctx);

	SourceRange src = source(*ctx);
	UniqPtrVec<Expression> expressions = popChildren<Expression>(src);

	return push<List>(std::move(expressions), src);
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
		kwargs = popChildren<Argument>(source(*kwctx));
	}

	UniqPtrVec<Expression> positionalArgs;
	if (auto *posctx = ctx->positionalArguments())
	{
		positionalArgs = popChildren<Expression>(source(*posctx));
	}

	return push<Arguments>(std::move(positionalArgs), std::move(kwargs), source(*ctx));
}

antlrcpp::Any ASTBuilder::visitKeywordArgument(FabParser::KeywordArgumentContext *ctx)
{
	ParseChildren(ctx);

	string name = ctx->Identifier()->getText();
	auto id = std::make_unique<Identifier>(name, source(*ctx->Identifier()));
	auto initializer = pop<Expression>(source(*ctx->expression()));

	return push<Argument>(std::move(id), std::move(initializer));
}

antlrcpp::Any ASTBuilder::visitParameter(FabParser::ParameterContext *ctx)
{
	ParseChildren(ctx);

	UniqPtr<Expression> defaultArgument;
	if (auto *def = ctx->defaultArgument)
	{
		defaultArgument = pop<Expression>(source(*def));
	}

	SourceRange src = source(*ctx->type());
	auto type = pop<TypeReference>(src);
	check(type, src, "failed to parse parameter type");

	auto name = ctx->Identifier()->getText();
	auto id = std::make_unique<Identifier>(name, source(*ctx->Identifier()));

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

	auto resultType = pop<TypeReference>(source(*ctx->result));

	UniqPtrVec<TypeReference> paramTypes;
	if (auto *p = ctx->params)
	{
		paramTypes = popChildren<TypeReference>(source(*p));
	}

	return push<FunctionTypeReference>(std::move(paramTypes), std::move(resultType),
	                                   source(*ctx));
}

antlrcpp::Any ASTBuilder::visitParametricType(FabParser::ParametricTypeContext *ctx)
{
	ParseChildren(ctx);

	auto params = popChildren<TypeReference>(source(*ctx->params));

	SourceRange baseSource = source(*ctx->base);
	auto base = pop<TypeReference>(baseSource);
	check(base, baseSource, "failed to parse parametric type base");

	return push<ParametricTypeReference>(std::move(base), source(*ctx),
	                                     std::move(params));
}

antlrcpp::Any ASTBuilder::visitSimpleType(FabParser::SimpleTypeContext *ctx)
{
	SourceRange src = source(*ctx);

	const std::string name = ctx->Identifier()->getText();
	auto id = std::make_unique<Identifier>(name, src);

	return push<SimpleTypeReference>(std::move(id), src);
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
