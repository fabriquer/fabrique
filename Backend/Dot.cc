/** @file Dot.cc    Definition of @ref DotBackend. */
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

#include "Backend/Dot.h"
#include "DAG/Build.h"
#include "DAG/File.h"
#include "DAG/Rule.h"
#include "DAG/Value.h"
#include "Support/Bytestream.h"

#include <cassert>

using namespace fabrique::dag;
using fabrique::backend::DotBackend;
using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;


DotBackend* DotBackend::Create()
{
	return new DotBackend;
}


DotBackend::DotBackend()
	: indent("\t")
{
}


void DotBackend::Process(const DAG& dag, Bytestream& out)
{
	//
	// Extract the files and builds from the DAG (ignore variables, etc.).
	//
	typedef std::pair<string,shared_ptr<Value>> NamedValue;

	StringMap<shared_ptr<File>> files;
	StringMap<shared_ptr<Build>> builds;

	for (auto& i : dag)
	{
		string name = i.first;
		const shared_ptr<Value>& v = i.second;
		assert(v);

		if (auto file = dynamic_pointer_cast<File>(v))
			files[name] = file;

		else if (auto build = dynamic_pointer_cast<Build>(v))
			builds[name] = build;
	}


	out
		<< Bytestream::Comment << "#\n"
		<< "# .dot graph generated by Fabrique\n"
		<< "#\n"
		<< Bytestream::Definition << "digraph"
		<< Bytestream::Operator << " {\n"
		<< indent
		<< Bytestream::Definition << "rankdir"
		<< Bytestream::Operator << " = "
		<< Bytestream::Literal << "\"LR\""
		<< Bytestream::Operator << ";"
		<< Bytestream::Reset << "\n\n"
		;

	for (auto& i : builds)
	{
		const dag::Build& build = *i.second;
		const std::string name = build.str();

		out
			<< indent
			<< Bytestream::Literal << "\"" << name << "\""
			<< Bytestream::Operator << " [ "
			<< Bytestream::Definition << "shape"
			<< Bytestream::Operator << " = "
			<< Bytestream::Literal << "rectangle"
			<< Bytestream::Operator << ", "
			<< Bytestream::Definition << "label"
			<< Bytestream::Operator << " = "
			<< Bytestream::Literal
			<< "\"" << build.buildRule().name() << "\""
			<< Bytestream::Operator << " ];\n";

		for (const shared_ptr<File>& f : build.allInputs())
			out
				<< indent
				<< Bytestream::Operator << "\""
				<< *f
				<< Bytestream::Operator << "\" -> "
				<< Bytestream::Literal << "\"" << name << "\""
				<< Bytestream::Operator << ";\n"
				;

		for (const shared_ptr<File>& f : build.outputs())
			out
				<< indent
				<< Bytestream::Literal << "\"" << name << "\""
				<< Bytestream::Operator << " -> \""
				<< Bytestream::Literal << *f
				<< Bytestream::Operator << "\";\n"
				;

		for (const shared_ptr<File>& f : build.sideEffectOutputs())
			out
				<< indent
				<< Bytestream::Literal << "\"" << name << "\""
				<< Bytestream::Operator << " -> \""
				<< Bytestream::Literal << *f
				<< Bytestream::Operator << "\";\n"
				;
	}

	out
		<< Bytestream::Operator << "}\n"
		<< Bytestream::Reset
		;
}
