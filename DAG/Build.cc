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

#include "Support/Bytestream.h"
#include "Support/exceptions.h"

using namespace fabrique::dag;
using std::shared_ptr;
using std::vector;


namespace fabrique {
namespace dag {
static void listify(shared_ptr<Value>& in, vector<shared_ptr<File>>& out);
}
}


Build* Build::Create(shared_ptr<Rule>& rule,
                     shared_ptr<Value> in, shared_ptr<Value> out,
                     const ValueMap& arguments, const SourceRange src)
{
	vector<shared_ptr<File>> inputs;
	listify(in, inputs);

	vector<shared_ptr<File>> outputs;
	listify(out, outputs);

	return new Build(rule, inputs, outputs, arguments, src);
}


Build::Build(shared_ptr<Rule>& rule,
             vector<shared_ptr<File>>& inputs,
             vector<shared_ptr<File>>& outputs,
             const ValueMap& arguments,
             SourceRange location)
	: Value(location), rule(rule),
	  inputs(inputs), outputs(outputs), arguments(arguments)
{
}


std::string Build::str() const
{
	return "<unimplemented>";
}


void Build::PrettyPrint(Bytestream& out, int indent) const
{
	out << Bytestream::Operator << "{";

	for (const shared_ptr<File>& f : inputs)
		out << " " << *f;

	out << Bytestream::Operator << " => ";

	for (const shared_ptr<File>& f : outputs)
		out << *f << " ";

	out << Bytestream::Operator << "}";

	if (arguments.size() > 0)
	{
		out << Bytestream::Operator << "( ";

		for (auto& i : arguments)
			out
				<< Bytestream::Definition << i.first
				<< Bytestream::Operator << " = "
				<< *i.second
				<< " "
				;

		out << Bytestream::Operator << ")";
	}

	out << Bytestream::Reset;
}


static void fabrique::dag::listify(shared_ptr<Value>& in,
                                   vector<shared_ptr<File>>& out)
{
	if (shared_ptr<File> file = std::dynamic_pointer_cast<File>(in))
		out.push_back(file);

	else if (shared_ptr<List> list = std::dynamic_pointer_cast<List>(in))
		for (shared_ptr<Value> value : *list)
		{
		}

	else throw SemanticException(
		"expected file or list of files", in->getSource());
}
