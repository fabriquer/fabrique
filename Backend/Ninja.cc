/** @file Ninja.cc    Definition of @ref NinjaBackend. */
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

#include "Backend/Ninja.h"

#include "DAG/DAG.h"
#include "DAG/File.h"
#include "DAG/List.h"
#include "DAG/Primitive.h"
#include "DAG/Rule.h"

#include "Support/Bytestream.h"
#include "Support/Join.h"

using namespace fabrique::backend;
using namespace fabrique::dag;

using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;
using std::vector;


NinjaBackend* NinjaBackend::Create(Bytestream& out)
{
	return new NinjaBackend(out);
}


NinjaBackend::NinjaBackend(Bytestream& out)
	: out(out), indent("    ")
{
}


string stringify(const shared_ptr<Value>& v)
{
	assert(v);

	if (auto list = dynamic_pointer_cast<List>(v))
	{
		vector<string> substrings;
		for (auto& v : *list)
			substrings.push_back(stringify(v));

		return fabrique::join(substrings, " ");
	}

	return v->str();
}

void NinjaBackend::Process(const dag::DAG& d)
{
	//
	// Split values into files, rules and variables.
	//
	typedef std::pair<string,shared_ptr<Value>> NamedValue;

	StringMap<shared_ptr<File>> files;
	StringMap<shared_ptr<Rule>> rules;
	StringMap<string> variables;

	for (auto& i : d)
	{
		string name = i.first;
		const shared_ptr<Value>& v = i.second;
		assert(v);

		if (auto file = dynamic_pointer_cast<File>(v))
			files[name] = file;

		else if (auto rule = dynamic_pointer_cast<Rule>(v))
			rules[name] = rule;

		else
			variables[name] = stringify(v);
	}


	//
	// Now write out the file:
	//

	// Header comment:
	out
		<< Bytestream::Comment
		<< "#\n"
		<< "# Ninja file generated by Fabrique\n"
		<< "#\n"
		<< "\n"
		;


	// Variables:
	out
		<< "#\n"
		<< "# Varables:\n"
		<< "#\n"
		<< Bytestream::Reset
		;

	for (auto& i : variables)
	{
		out
			<< Bytestream::Definition << i.first
			<< Bytestream::Operator << " = "
			<< Bytestream::Literal << i.second
			<< Bytestream::Reset
			<< "\n"
			;
	}


	// Rules:
	out
		<< "\n"
		<< Bytestream::Comment
		<< "#\n"
		<< "# Rules:\n"
		<< "#\n"
		<< Bytestream::Reset
		;

	for (auto& i : rules)
	{
		const dag::Rule& rule = *i.second;

		out
			<< Bytestream::Type << "rule "
			<< Bytestream::Action << i.first
			<< Bytestream::Reset << "\n"

			<< Bytestream::Definition << "  command"
			<< Bytestream::Operator << " = "
			<< Bytestream::Literal << rule.command()
			<< Bytestream::Reset << "\n"

			<< Bytestream::Definition << "  description"
			<< Bytestream::Operator << " = "
			<< Bytestream::Literal << rule.description()
			<< Bytestream::Reset << "\n"
			;

		for (auto& p : rule.parameters())
			out
				<< Bytestream::Definition << "  " << p.first
				<< Bytestream::Operator << " = "
				<< Bytestream::Literal << stringify(p.second)
				<< Bytestream::Reset << "\n"
				;

		out << "\n";
	}
}
