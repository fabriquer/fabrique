/** @file AST/NameReference.cc    Definition of @ref fabrique::ast::NameReference. */
/*
 * Copyright (c) 2013, 2016 Jonathan Anderson
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
#include "AST/NameReference.h"
#include "AST/Scope.h"
#include "AST/Visitor.h"
#include "DAG/Record.h"
#include "DAG/UndefinedValueException.h"
#include "Parsing/ErrorReporter.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/Type.h"

using namespace fabrique;
using namespace fabrique::ast;
using std::string;


NameReference::Parser::~Parser()
{
}


NameReference*
NameReference::Parser::Build(const Scope& scope, TypeContext& types, Err& err)
{
	if (not name_)
		return nullptr;

	UniqPtr<Identifier> name(name_->Build(scope, types, err));

	const Type& targetType = scope.Lookup(*name);
	if (not targetType)
	{
		err.ReportError("reference to undefined value", *name);
		return nullptr;
	}

	return new NameReference(std::move(name), targetType);
}


NameReference::NameReference(UniqPtr<Identifier> name, const Type& targetType)
	: Expression(targetType, name->source()), name_(std::move(name))
{
}


void NameReference::PrettyPrint(Bytestream& out, size_t indent) const
{
	name_->PrettyPrint(out, indent);
}


void NameReference::Accept(Visitor& v) const
{
	if (v.Enter(*this))
		name_->Accept(v);

	v.Leave(*this);
}

dag::ValuePtr NameReference::evaluate(EvalContext& ctx) const
{
	static Bytestream& debug = Bytestream::Debug("eval.lookup");
	const string& name = Type::UntypedPart(name_->str());

	std::shared_ptr<dag::Record> base;
	dag::ValuePtr value;

	//
	// A symbol reference can have multiple dot-separated components:
	//
	// foo = bar.baz.wibble;
	//
	// In this case, 'bar' and 'bar.baz' must both be records
	// (things that can contain named things), but 'wibble' can
	// be any kind of Value.
	//
	// Iterate over each name component.
	//
	for (size_t begin = 0, end; begin < name.length(); begin = end + 1)
	{
		end = name.find('.', begin + 1);
		const string component = name.substr(begin, end - begin);

		debug
			<< Bytestream::Action << "lookup component "
			<< Bytestream::Operator << "'"
			<< Bytestream::Literal << component
			<< Bytestream::Operator << "'"
			<< Bytestream::Reset << "\n"
			;

		value = base
			? base->field(component)
			: ctx.Lookup(component);

		if (not value)
			throw dag::UndefinedValueException(
				name.substr(0, end), source());

		// Is this the last component?
		if (end == string::npos)
			break;

		// Not the last component: must be a record!
		base = std::dynamic_pointer_cast<dag::Record>(value);
		if (not base)
			throw SemanticException(
				name.substr(0, end)
				+ " (" + value->type().str()
				+ ") is not a record",
				source());
	}

	if (not value)
		throw dag::UndefinedValueException(name, source());

	return value;
}
