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

#include "DAG/Build.h"
#include "DAG/DAG.h"
#include "DAG/File.h"
#include "DAG/List.h"
#include "DAG/Primitive.h"
#include "DAG/Rule.h"
#include "DAG/Target.h"

#include "Support/Bytestream.h"
#include "Support/Join.h"

#include <cassert>

using namespace fabrique::backend;
using namespace fabrique::dag;

using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;
using std::vector;


NinjaBackend* NinjaBackend::Create()
{
	return new NinjaBackend;
}


NinjaBackend::NinjaBackend()
	: indent("    ")
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
	else if (auto target = dynamic_pointer_cast<Target>(v))
	{
		vector<string> files;
		for (auto& f : target->files())
			files.push_back(stringify(f));

		return fabrique::join(files, " ");
	}

	return v->str();
}

void NinjaBackend::Process(const dag::DAG& dag, Bytestream& out)
{
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

	for (auto& i : dag.variables())
	{
		out
			<< Bytestream::Definition << i.first
			<< Bytestream::Operator << " = "
			<< Bytestream::Literal << stringify(i.second)
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

	for (auto& i : dag.rules())
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

		for (auto& a : rule.arguments())
			out
				<< Bytestream::Definition << "  " << a.first
				<< Bytestream::Operator << " = "
				<< Bytestream::Literal << stringify(a.second)
				<< Bytestream::Reset << "\n"
				;

		out << "\n";
	}


	// Psuedo-targets:
	for (auto& i : dag.targets())
		out
			<< Bytestream::Type << "build "
			<< Bytestream::Definition << i.first
			<< Bytestream::Operator << " : "
			<< Bytestream::Action << "phony "
			<< Bytestream::Literal << stringify(i.second)
			<< "\n"
			;

	out << "\n";


	// Build steps:
	out
		<< "\n"
		<< Bytestream::Comment
		<< "#\n"
		<< "# Build steps:\n"
		<< "#\n"
		<< Bytestream::Reset
		;

	for (auto& i : dag.builds())
	{
		const dag::Build& build = *i;

		out << Bytestream::Type << "build";
		for (const shared_ptr<File>& f : build.outputs())
			out << " " << *f;

		for (const shared_ptr<File>& f : build.sideEffectOutputs())
			out << " " << *f;

		out
			<< Bytestream::Operator << ": "
			<< Bytestream::Reference << build.buildRule().name()
			;

		for (const shared_ptr<File>& f : build.explicitInputs())
			out << " " << *f;

		if (build.dependencies().size() > 0)
		{
			out << " |";
			for (const shared_ptr<File>& f : build.explicitInputs())
				out << " " << *f;
		}

		out << "\n";

		for (auto& a : build.arguments())
			out
				<< Bytestream::Definition << "  " << a.first
				<< Bytestream::Operator << " = "
				<< Bytestream::Literal << stringify(a.second)
				<< Bytestream::Reset << "\n"
				;
		out << "\n";
	}
}
