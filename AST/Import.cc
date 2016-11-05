/** @file AST/Import.cc    Definition of @ref fabrique::ast::Import. */
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

#include "AST/Argument.h"
#include "AST/Builtins.h"
#include "AST/EvalContext.h"
#include "AST/Import.h"
#include "AST/Scope.h"
#include "AST/Value.h"
#include "AST/Visitor.h"
#include "DAG/File.h"
#include "DAG/Record.h"
#include "Plugin/Plugin.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/TypeContext.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;
using fabrique::plugin::Plugin;


Import::Import(UniqPtr<StringLiteral>& name, UniqPtrVec<Argument>& arguments,
               const std::string& subdir, UniqPtr<Scope>& scope, const Type& ty,
               SourceRange src)
	: Expression(ty, src), HasScope(std::move(scope)),
	  name_(std::move(name)), arguments_(std::move(arguments)), subdirectory_(subdir)
{
}


Import::Import(UniqPtr<StringLiteral>& name, UniqPtrVec<Argument>& arguments,
               UniqPtr<Plugin>& plugin, SourceRange src)
	: Expression(plugin->type(), src), HasScope(nullptr),
	  name_(std::move(name)), arguments_(std::move(arguments)),
	  plugin_(std::move(plugin))
{
}


void Import::PrettyPrint(Bytestream& out, size_t indent) const
{
	out
		<< Bytestream::Action << "import"
		<< Bytestream::Operator << "("
		<< *name_
		;

	if (not arguments_.empty())
	{
		for (const UniqPtr<Argument>& a : arguments_)
		{
			out << Bytestream::Operator << ", ";
			a->PrettyPrint(out, indent);
		}
	}

	out
		<< Bytestream::Operator << ")"
		;
}


void Import::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		name_->Accept(v);
	}

	v.Leave(*this);
}

dag::ValuePtr Import::evaluate(EvalContext& ctx) const
{
	auto scope(ctx.EnterScope("import()"));

	dag::ValuePtr builddir(
		ctx.builder().File(subdirectory(), dag::ValueMap(),
	                           type().context().fileType(), source(), true));

	dag::ValuePtr subdir(
		ctx.builder().File(subdirectory(), dag::ValueMap(),
	                           type().context().fileType(), source()));

	scope.set(ast::BuildDirectory, builddir);
	scope.set(ast::Subdirectory, subdir);

	// Gather arguments into an 'args' record.
	dag::ValueMap args;
	for (const UniqPtr<ast::Argument>& a : arguments())
	{
		assert(a->hasName());

		const std::string& argName = a->getName().name();
		dag::ValuePtr value = a->evaluate(ctx);

		args[argName] = value;
	}

	dag::DAGBuilder builder(ctx.builder());
	if (plugin_)
		return plugin_->Create(builder, args);

	/*
	dag::ValuePtr argRecord(
		builder.Record(args, this->scope().arguments(), source()));

	scope.set(ast::Arguments, argRecord);
	*/

	for (const auto& v : this->scope().values())
		v->evaluate(ctx);

	dag::ValueMap fields;
	Type::NamedTypeVec types;
	for (auto& i : scope.leave())
	{
		fields[i.first] = i.second;
		types.emplace_back(i.first, i.second->type());
	}

	return builder.Record(fields, source());
}
