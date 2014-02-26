/** @file Build.cc    Definition of @ref Build. */
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
#include "DAG/Rule.h"
#include "DAG/Target.h"

#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Support/exceptions.h"

#include "Types/FunctionType.h"
#include "Types/TypeError.h"

#include <cassert>

using namespace fabrique::dag;
using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::vector;


Build* Build::Create(shared_ptr<Rule>& rule, SharedPtrMap<Value>& args,
                     ConstPtrMap<Type>& paramTypes, const SourceRange& src)
{
	SharedPtrVec<File> inputs, outputs, dependencies, extraOutputs;
	ValueMap arguments;

	for (auto& i : args)
	{
		const std::string& name = i.first;
		shared_ptr<Value>& arg = i.second;
		const Type& type = i.second->type();

		if (name == "in")
			appendFiles(arg, inputs);

		else if (name == "out")
			appendFiles(arg, outputs);

		else if (type.isFile())
		{
			const Type& type = *paramTypes[name];

			if (type.typeParamCount() == 0)
				throw SemanticException(
					"file missing [in] or [out] tag", src);

			if (type[0].name() == "in")
			{
				appendFiles(arg, dependencies);
			}
			else if (type[0].name() == "out")
			{
				appendFiles(arg, extraOutputs);
				arguments[name] = arg;
			}
			else
				throw WrongTypeException("file[in|out]", type,
					arg->source());
		}

		else
			arguments[name] = arg;
	}

	// TODO: allow input-less actions (e.g. 'cat uname -a > file')?
	assert(not inputs.empty());
	assert(not outputs.empty());

	const File& out = *outputs.front();
	const Type& type =
		(outputs.size() == 1 and extraOutputs.empty()
		 and out.type().isFile())
			? out.type()
			: Type::ListOf(out.type());

	return new Build(rule, inputs, outputs, dependencies, extraOutputs,
	                 arguments, type, src);
}


Build::Build(shared_ptr<Rule>& rule,
             SharedPtrVec<File>& inputs,
             SharedPtrVec<File>& outputs,
             SharedPtrVec<File>& dependencies,
             SharedPtrVec<File>& extraOut,
             const ValueMap& arguments,
             const Type& t,
             SourceRange location)
	: Value(t, location), rule(rule),
	  in(inputs), out(outputs), deps(dependencies), extraOut(extraOut),
	  args(arguments)
{
	for (auto& f : in)
		assert(f);

	for (auto& f : out)
		assert(f);

	for (auto& f : deps)
		assert(f);

	for (auto& f : extraOut)
		assert(f);
}


const Build::FileVec Build::allInputs() const
{
	FileVec everything;

	for (shared_ptr<File> f : in)
		everything.push_back(f);

	for (shared_ptr<File> f : deps)
		everything.push_back(f);

	return everything;
}


const Build::FileVec Build::allOutputs() const
{
	FileVec everything;

	for (shared_ptr<File> f : out)
		everything.push_back(f);

	for (shared_ptr<File> f : extraOut)
		everything.push_back(f);

	return everything;
}


void Build::PrettyPrint(Bytestream& ostream, int indent) const
{
	ostream
		<< Bytestream::Reference << rule->name() << " "
		<< Bytestream::Operator << "{"
		;

	for (const shared_ptr<File>& f : in)
		ostream << " " << *f;

	ostream << Bytestream::Operator << " => ";

	for (const shared_ptr<File>& f : out)
		ostream << *f << " ";

	if (extraOut.size() > 0)
	{
		ostream << " + ";
		for (shared_ptr<File> f : extraOut)
			ostream << *f << " ";
	}

	ostream << Bytestream::Operator << "}";

	if (args.size() > 0)
	{
		ostream << Bytestream::Operator << "( ";

		for (auto& j : args)
		{
			ostream
				<< Bytestream::Definition << j.first
				<< Bytestream::Operator << " = "
				<< *j.second
				<< " "
				;
		}

		ostream << Bytestream::Operator << ")";
	}

	ostream << Bytestream::Reset;
}


void Build::appendFiles(const shared_ptr<Value>& in,
                        vector<shared_ptr<File>>& out)
{
	assert(in);

	if (shared_ptr<Build> build = dynamic_pointer_cast<Build>(in))
		//
		// Not sure why std::copy() doesn't work here, but
		// it doesn't (segfault).
		//
		for (shared_ptr<File> i : build->out)
			out.push_back(i);

	else if (const shared_ptr<File>& file = dynamic_pointer_cast<File>(in))
		out.push_back(file);

	else if (const shared_ptr<List>& list = dynamic_pointer_cast<List>(in))
		for (const shared_ptr<Value>& value : *list)
			appendFiles(value, out);

	else if (const shared_ptr<Target>& t = dynamic_pointer_cast<Target>(in))
		for (const shared_ptr<Value>& f : t->files())
			appendFiles(f, out);

	else throw WrongTypeException("file|list[file]",
	                              in->type(), in->source());

	for (auto& f : out)
		assert(f);
}
