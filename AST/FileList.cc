/** @file AST/FileList.cc    Definition of @ref fabrique::ast::FileList. */
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

#include "AST/Builtins.h"
#include "AST/FileList.h"
#include "AST/Visitor.h"
#include "DAG/EvalContext.h"
#include "DAG/File.h"
#include "DAG/List.h"
#include "DAG/Primitive.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Support/os.h"
#include "Types/TypeContext.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;
using std::string;


void FileList::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out << Bytestream::Operator << "[" << Bytestream::Reset;

	for (auto& file : files_)
		out << " " << *file;

	for (auto& arg : args_)
		out << ", " << *arg;

	out << Bytestream::Operator << " ]" << Bytestream::Reset;
}


void FileList::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		for (auto& f : files_)
			f->Accept(v);

		for (auto& a : args_)
			a->Accept(v);
	}

	v.Leave(*this);
}

dag::ValuePtr FileList::evaluate(dag::EvalContext& ctx) const
{
	assert(ctx.Lookup(ast::Subdirectory));
	const string subdir = ctx.Lookup(ast::Subdirectory)->str();

	auto scope(ctx.EnterScope("files"));
	SharedPtrVec<dag::Value> files;

	for (const UniqPtr<Argument>& arg : arguments())
	{
		const string name = arg->getName().name();
		if (name == ast::Subdirectory)
		{
			const string subsubdir =
				arg->getValue().evaluate(ctx)->str();

			const string completeSubdir = JoinPath(subdir, subsubdir);
			const SourceRange& src = arg->getValue().source();

			scope.set(name, ctx.String(completeSubdir, src));
		}

		else
			throw SemanticException("unexpected argument",
			                        arg->source());
	}

	for (const UniqPtr<Filename>& file : files_)
		files.push_back(
			std::dynamic_pointer_cast<dag::File>(
				file->evaluate(ctx)));

	scope.leave();

	return dag::ValuePtr(dag::List::of(files, source(), type().context()));
}
