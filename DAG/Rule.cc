/** @file DAG/Rule.cc    Definition of @ref fabrique::dag::Rule. */
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

#include "DAG/File.h"
#include "DAG/Parameter.h"
#include "DAG/Rule.h"
#include "DAG/Visitor.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"

using namespace fabrique::dag;
using std::string;


Rule* Rule::Create(string name, string command, const ValueMap& arguments,
                   const SharedPtrVec<Parameter>& parameters,
                   const Type& t, const SourceRange& location)
{
	ValueMap args(arguments);

	string description;

	// If no description has been specified, use the command string.
	auto d = args.find("description");
	if (d != args.end())
	{
		description = d->second->str();
		args.erase(d);
	}
	else
	{
		description = command;
	}

	return new Rule(name, command, description, args, parameters,
	                t, location);
}


Rule::Rule(const string& name, const string& command, const string& description,
	   const ValueMap& args, const SharedPtrVec<Parameter>& parameters,
	   const Type& t, SourceRange location)
	: Callable(parameters), Value(t, location), ruleName_(name),
	  command_(command), description_(description), arguments_(args)
{
}

void Rule::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out
		<< Bytestream::Action << command_
		<< Bytestream::Operator << " {"
		<< Bytestream::Literal << " '" << description_ << "'"
		;

	for (auto& i : arguments_)
	{
		out
			<< Bytestream::Operator << ", "
			<< Bytestream::Definition << i.first
			<< Bytestream::Operator << " = "
			<< Bytestream::Literal << "'" << *i.second << "'"
			;
	}

	if (not parameters().empty())
	{
		out << Bytestream::Operator << " <-";
		for (auto& i : parameters())
			out << " " << *i << Bytestream::Operator << ",";
	}

	out
		<< Bytestream::Operator << " }"
		<< Bytestream::Reset
		;
}


void Rule::Accept(Visitor& v) const
{
	if (v.Visit(*this))
	{
		for (auto a : arguments_)
			a.second->Accept(v);
	}
}
