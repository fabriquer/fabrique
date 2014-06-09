/** @file DAG/Build.cc    Definition of @ref fabrique::dag::Build. */
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

#include "DAG/Build.h"
#include "DAG/File.h"
#include "DAG/List.h"
#include "DAG/Parameter.h"
#include "DAG/Rule.h"
#include "DAG/Target.h"
#include "DAG/Visitor.h"

#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Support/exceptions.h"

#include "Types/FileType.h"
#include "Types/FunctionType.h"
#include "Types/TypeError.h"

#include <cassert>

using namespace fabrique::dag;
using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::vector;


Build* Build::Create(shared_ptr<Rule>& rule, SharedPtrMap<Value>& arguments,
                     ConstPtrMap<Type>& paramTypes, const SourceRange& src)
{
	SharedPtrVec<File> inputs, outputs;

	for (auto& i : arguments)
	{
		const std::string& name = i.first;
		ValuePtr& arg = i.second;
		const Type& argType = i.second->type();
		const Type& paramType = *paramTypes[name];

		if (not argType.isSubtype(paramType))
			throw WrongTypeException(paramType, argType, arg->source());

		if (FileType::isInput(paramType))
			AppendFiles(arg, inputs);

		else if (FileType::isOutput(paramType))
			AppendFiles(arg, outputs, true);
	}

	assert(not outputs.empty());

	// Also include unspecified default parameters.
	for (const shared_ptr<Parameter>& p : rule->parameters())
	{
		auto i = arguments.find(p->name());
		if (i == arguments.end())
			arguments[p->name()] = p->defaultValue();
	}

	const File& out = *outputs.front();
	const Type& type =
		(outputs.size() == 1)
			? outputs.front()->type()
			: Type::ListOf(out.type(), out.source());

	return new Build(rule, inputs, outputs, arguments, type, src);
}


Build::Build(shared_ptr<Rule>& rule,
             SharedPtrVec<File>& inputs,
             SharedPtrVec<File>& outputs,
             const ValueMap& arguments,
             const Type& t,
             SourceRange location)
	: Value(t, location), rule_(rule), in_(inputs), out_(outputs),
	  args_(arguments)
{
	for (auto& f : in_)
		assert(f);

	for (auto& f : out_)
		assert(f);
}


void Build::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out
		<< Bytestream::Reference << rule_->name() << " "
		<< Bytestream::Operator << "{"
		;

	for (const shared_ptr<File>& f : in_)
		out << " " << *f;

	out << Bytestream::Operator << " => ";

	for (const shared_ptr<File>& f : out_)
		out << *f << " ";

	out << Bytestream::Operator << "}";

	if (args_.size() > 0)
	{
		out << Bytestream::Operator << "( ";

		for (auto& j : args_)
		{
			if (FileType::isFileOrFiles(j.second->type()))
				continue;

			out
				<< Bytestream::Definition << j.first
				<< Bytestream::Operator << " = "
				<< *j.second
				<< " "
				;
		}

		out << Bytestream::Operator << ")";
	}

	out << Bytestream::Reset;
}


void Build::Accept(Visitor& v) const
{
	if (v.Visit(*this))
	{
		for (auto a : args_)
			a.second->Accept(v);

		for (auto f : inputs())
			f->Accept(v);

		for (auto f : outputs())
			f->Accept(v);
	}
}


void Build::AppendFiles(const ValuePtr& in,
                        vector<shared_ptr<File>>& out, bool generated)
{
	assert(in);

	if (const shared_ptr<File>& file = dynamic_pointer_cast<File>(in))
	{
		out.push_back(file);
		if (generated and not file->generated())
			file->setGenerated(true);
	}
	else if (shared_ptr<Build> build = dynamic_pointer_cast<Build>(in))
		for (shared_ptr<File> i : build->out_)
			AppendFiles(i, out, generated);

	else if (const shared_ptr<List>& list = dynamic_pointer_cast<List>(in))
		for (const ValuePtr& value : *list)
			AppendFiles(value, out, generated);

	else if (const shared_ptr<Target>& t = dynamic_pointer_cast<Target>(in))
		for (const ValuePtr& f : *t->files())
			AppendFiles(f, out, generated);

	else throw WrongTypeException("file|list[file]",
	                              in->type(), in->source());

	for (auto& f : out)
		assert(f);
}
