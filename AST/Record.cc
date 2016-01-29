/**
 * @file AST/Record.cc
 * Definition of @ref fabrique::ast::Record.
 */
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

#include "AST/EvalContext.h"
#include "AST/Scope.h"
#include "AST/Record.h"
#include "AST/Visitor.h"
#include "Support/Bytestream.h"
#include "Types/RecordType.h"

using namespace fabrique;
using namespace fabrique::ast;


UniqPtr<Record> Record::Create(UniqPtr<Scope> scope, TypeContext& types)
{
	if (not scope)
		return UniqPtr<Record>();

	Type::NamedTypeVec fieldTypes;
	for (auto& value : scope->values())
	{
		fieldTypes.emplace_back(value->name().name(), value->type());
	}
	const RecordType& t = types.recordType(fieldTypes);

	SourceRange src = scope->source();

	return UniqPtr<Record>(new Record(scope, t, src));
}


Record::Parser::~Parser()
{
}


bool Record::Parser::construct(const ParserInput& input, ParserStack& s, ParseError err)
{
	source_ = input;
	return Node::Parser::construct(input, s, err);
}


Record* Record::Parser::Build(const Scope& s, TypeContext& types, Err& err)
{
	UniqPtr<Scope> scope(values_->Build(s, types, err));
	if (not scope)
		return nullptr;

	Type::NamedTypeVec fieldTypes;
	for (const auto& v : scope->values())
	{
		fieldTypes.emplace_back(v->name().name(), v->type());
	}

	return new Record(scope, types.recordType(fieldTypes), source_);
}


Record::Record(UniqPtr<Scope>& fields, const RecordType& ty, const SourceRange& loc)
	: Expression(ty, loc), HasScope(std::move(fields))
{
}


const PtrVec<Value>& Record::fields() const
{
	return scope().values();
}


void Record::PrettyPrint(Bytestream& out, size_t indent) const
{
	const std::string outerTabs(indent, '\t');

	out
		<< Bytestream::Definition << "record\n"
		<< Bytestream::Operator << outerTabs << "{\n"
		;

	const std::string innerTabs(indent + 1, '\t');
	for (auto& i : scope().values())
	{
		i->PrettyPrint(out, indent + 1);
		out << "\n";
	}

	out
		<< Bytestream::Operator << outerTabs << "}"
		<< Bytestream::Reset
		;
}

void Record::Accept(Visitor& v) const
{
	if (v.Enter(*this))
		scope().Accept(v);

	v.Leave(*this);
}

dag::ValuePtr Record::evaluate(EvalContext& ctx) const
{
	auto instantiationScope(ctx.EnterScope("record"));

	dag::ValueMap fields;

	for (auto& field : scope().values())
		fields[field->name().name()] = field->evaluate(ctx);

	return ctx.builder().Record(fields, type(), source());
}
