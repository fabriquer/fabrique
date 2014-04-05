/** @file Backend/Dot.cc    Definition of fabrique::backend::DotBackend. */
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
#include "DAG/Formatter.h"
#include "DAG/List.h"
#include "DAG/Primitive.h"
#include "DAG/Rule.h"
#include "DAG/Target.h"
#include "DAG/Value.h"
#include "Support/Bytestream.h"
#include "Support/Join.h"

#include <cassert>

using namespace fabrique::dag;
using fabrique::backend::DotBackend;
using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;


class DotFormatter : public Formatter
{
public:
	using Formatter::Format;

	string Format(const Boolean&);
	string Format(const Build&);
	string Format(const File&);
	string Format(const Integer&);
	string Format(const List&);
	string Format(const Rule&);
	string Format(const String&);
	string Format(const Target&);
};


DotBackend* DotBackend::Create()
{
	return new DotBackend;
}


DotBackend::DotBackend()
	: indent_("\t")
{
}


void DotBackend::Process(const DAG& dag, Bytestream& out)
{
	DotFormatter formatter;

	out
		<< Bytestream::Comment << "#\n"
		<< "# .dot graph generated by Fabrique\n"
		<< "#\n"
		<< Bytestream::Definition << "digraph"
		<< Bytestream::Operator << " {\n"
		<< indent_
		<< Bytestream::Definition << "rankdir"
		<< Bytestream::Operator << " = "
		<< Bytestream::Literal << "\"LR\""
		<< Bytestream::Operator << ";"
		<< Bytestream::Reset << "\n\n"
		;


	out << Bytestream::Comment << "# Files:\n" ;
	for (auto& f : dag.files())
	{
		out
			<< indent_
			<< Bytestream::Definition
			<< "\"" << formatter.Format(*f) << "\""
			<< Bytestream::Operator << " [ "
			<< Bytestream::Definition << "shape"
			<< Bytestream::Operator << " = "
			<< Bytestream::Literal
			<< (f->generated() ? "octagon" : "ellipse")
			<< Bytestream::Operator << " ];\n"
			;
	}
	out << "\n";

	for (auto& i : dag.builds())
	{
		const dag::Build& build = *i;
		const std::string name = formatter.Format(build);

		out
			<< indent_
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
				<< indent_
				<< Bytestream::Operator << "\""
				<< formatter.Format(*f)
				<< Bytestream::Operator << "\" -> "
				<< Bytestream::Literal << "\"" << name << "\""
				<< Bytestream::Operator << ";\n"
				;

		for (const shared_ptr<File>& f : build.outputs())
			out
				<< indent_
				<< Bytestream::Literal << "\"" << name << "\""
				<< Bytestream::Operator << " -> \""
				<< Bytestream::Literal << formatter.Format(*f)
				<< Bytestream::Operator << "\";\n"
				;

		for (const shared_ptr<File>& f : build.sideEffectOutputs())
			out
				<< indent_
				<< Bytestream::Literal << "\"" << name << "\""
				<< Bytestream::Operator << " -> \""
				<< Bytestream::Literal << formatter.Format(*f)
				<< Bytestream::Operator << "\";\n"
				;
	}

	out
		<< Bytestream::Operator << "}\n"
		<< Bytestream::Reset
		;
}



string DotFormatter::Format(const Boolean& b)
{
	return b.value() ? "true" : "false";
}

string DotFormatter::Format(const Build& build)
{
	std::vector<string> substrings;

	substrings.push_back(build.buildRule().name());
	substrings.push_back("{");

	for (auto& i : build.explicitInputs())
		substrings.push_back(Format(*i));

	if (not build.dependencies().empty())
		substrings.push_back("+");

	for (auto& i : build.dependencies())
		substrings.push_back(Format(*i));

	substrings.push_back("=>");

	for (auto& i : build.outputs())
		substrings.push_back(Format(*i));

	if (not build.sideEffectOutputs().empty())
		substrings.push_back("+");

	for (auto& i : build.sideEffectOutputs())
		substrings.push_back(Format(*i));

	substrings.push_back("}");

	if (not build.arguments().empty())
	{
		substrings.push_back("(");

		for (auto& i : build.arguments())
		{
			substrings.push_back(i.first);
			substrings.push_back("=");
			substrings.push_back(Format(*i.second));
		}

		substrings.push_back(")");
	}

	return fabrique::join(substrings, " ");
}

string DotFormatter::Format(const File& f)
{
	return f.filename();
}

string DotFormatter::Format(const Integer& i)
{
	return std::to_string(i.value());
}

string DotFormatter::Format(const List& l)
{
	std::vector<string> substrings;

	for (const shared_ptr<Value>& element : l)
		substrings.push_back(Format(*element));

	return fabrique::join(substrings, " ");
}

string DotFormatter::Format(const Rule& rule)
{
	return rule.command();
}

string DotFormatter::Format(const String& s)
{
	return "'" + s.value() + "'";
}

string DotFormatter::Format(const Target& t)
{
	return Format(*t.files());
}
