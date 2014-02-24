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


Build* Build::Create(shared_ptr<Rule>& rule, shared_ptr<Value> in,
                     shared_ptr<Value> out, SharedPtrVec<Value> dependencies,
                     SharedPtrVec<Value> extraOutputs,
                     const ValueMap& arguments, const SourceRange src)
{
	SharedPtrVec<File> inputs;
	appendFiles(in, inputs);

	SharedPtrVec<File> outputs;
	appendFiles(out, outputs);

	SharedPtrVec<File> depends;
	for (shared_ptr<Value>& dep : dependencies)
	{
		shared_ptr<File> f = dynamic_pointer_cast<File>(dep);
		if (not f)
			throw WrongTypeException("file", dep->type(),
			                         dep->source());

		depends.push_back(f);
	}

	SharedPtrVec<File> extraOut;
	for (shared_ptr<Value>& out : extraOutputs)
	{
		shared_ptr<File> f = dynamic_pointer_cast<File>(out);
		if (not f)
			throw WrongTypeException("file", out->type(),
			                         out->source());

		extraOut.push_back(f);
	}

	const Type& type =
		(out->type().isFile() and not extraOut.empty())
			? Type::ListOf(out->type())
			: out->type();

	return new Build(rule, inputs, outputs, depends, extraOut,
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
			if (j.first == "in" or j.first == "out")
				continue;

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


void Build::appendFiles(shared_ptr<Value>& in, vector<shared_ptr<File>>& out)
{
	assert(in);

	if (shared_ptr<Build> build = dynamic_pointer_cast<Build>(in))
		//
		// Not sure why std::copy() doesn't work here, but
		// it doesn't (segfault).
		//
		for (shared_ptr<File> i : build->out)
			out.push_back(i);

	else if (shared_ptr<File> file = dynamic_pointer_cast<File>(in))
		out.push_back(file);

	else if (shared_ptr<List> list = dynamic_pointer_cast<List>(in))
		for (shared_ptr<Value> value : *list)
			appendFiles(value, out);

	else if (shared_ptr<Target> t = dynamic_pointer_cast<Target>(in))
		for (shared_ptr<Value> f : t->files())
			out.push_back(dynamic_pointer_cast<File>(f));

	else throw WrongTypeException("file|list[file]",
	                              in->type(), in->source());
}
