/** @file DAG/Build.cc    Definition of @ref fabrique::dag::Build. */
/*
 * Copyright (c) 2014, 2018 Jonathan Anderson
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

#include <fabrique/dag/Build.hh>
#include <fabrique/dag/File.hh>
#include <fabrique/dag/List.hh>
#include <fabrique/dag/Parameter.hh>
#include <fabrique/dag/Rule.hh>
#include <fabrique/dag/Visitor.hh>

#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Support/exceptions.h"

#include <fabrique/types/FileType.hh>
#include <fabrique/types/FunctionType.hh>
#include <fabrique/types/TypeError.hh>

#include <cassert>

using namespace fabrique::dag;
using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::vector;


Build* Build::Create(shared_ptr<Rule>& rule, SharedPtrMap<Value>& arguments,
                     const SourceRange& src)
{
	SemaCheck(rule, src, "Build::Create() passed null Rule");

	// Builds need the parameter types, not just the argument types.
	ConstPtrMap<Type> paramTypes;
	for (auto& p : rule->parameters())
		paramTypes[p->name()] = &p->type();


	SharedPtrVec<File> inputs, outputs;

	for (auto& i : arguments)
	{
		const std::string& name = i.first;
		ValuePtr& arg = i.second;
		SemaCheck(arg, src, "argument '" + name + "' has no value");

		const Type& argType = arg->type();
		const Type& paramType = *paramTypes[name];

		argType.CheckSubtype(paramType, arg->source());

		if (FileType::isInput(paramType))
			AppendFiles(arg, inputs);

		else if (FileType::isOutput(paramType))
			AppendFiles(arg, outputs, true);
	}

	SemaCheck(not outputs.empty(), src, "build does not produce any output files");

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


bool Build::hasFields() const
{
	// We only pass field requests through to underlying files, not lists
	// of files (they're not currently stored that way).
	return type().isFile();
}

ValuePtr Build::field(const std::string& name) const
{
	FAB_ASSERT(hasFields(), "accessing field of field-less Build");
	FAB_ASSERT(out_.size() == 1,
		"Build::Field only works with single-output builds (have"
		+ std::to_string(out_.size()) + " output files)");
	FAB_ASSERT(out_[0]->hasFields(), "output file has no fields");

	return out_[0]->field(name);
}

ValuePtr Build::Add(ValuePtr& rhs) const
{
	return outputValue()->Add(rhs);
}

ValuePtr Build::PrefixWith(ValuePtr& rhs) const
{
	return outputValue()->PrefixWith(rhs);
}

ValuePtr Build::And(ValuePtr& rhs) const
{
	return outputValue()->And(rhs);
}

ValuePtr Build::Or(ValuePtr& rhs) const
{
	return outputValue()->Or(rhs);
}

ValuePtr Build::Xor(ValuePtr& rhs) const
{
	return outputValue()->Xor(rhs);
}

ValuePtr Build::Equals(ValuePtr& rhs) const
{
	return outputValue()->Equals(rhs);
}

ValuePtr Build::outputValue() const
{
	if (type().isFile())
	{
		SemaCheck(out_.size() == 1, source(),
			"build of type " + type().str() + " should have one output, not "
			+ std::to_string(out_.size()));

		return out_[0];
	}

	SourceRange src(*out_.front(), **(out_.end() - 1));
	return ValuePtr { List::of(out_, src, type().context()) };
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
#ifndef NDEBUG
	for (auto& f : in_)
	{
		SemaCheck(f, location, "null input fule");
	}

	for (auto& f : out_)
	{
		SemaCheck(f, location, "null output file");
	}
#endif

	SemaCheck(t.hasFiles(), location, "no files in type " + t.str());
}


void Build::PrettyPrint(Bytestream& out, unsigned int /*indent*/) const
{
	out
		<< Bytestream::Definition << rule_->name()
		<< Bytestream::Operator << " {"
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
	FAB_ASSERT(in, "called Build::AppendFiles with null input");

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

	else throw WrongTypeException("file|list[file]",
	                              in->type(), in->source());

#ifndef NDEBUG
	for (auto& f : out)
	{
		FAB_ASSERT(f, "null output file after Build::AppendFiles()");
	}
#endif
}
