/** @file Backend/Dot.cc    Definition of fabrique::backend::DotBackend. */
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

#include <fabrique/backend/Dot.hh>

#include <fabrique/dag/Build.hh>
#include <fabrique/dag/File.hh>
#include <fabrique/dag/Formatter.hh>
#include <fabrique/dag/List.hh>
#include <fabrique/dag/Primitive.hh>
#include <fabrique/dag/Record.hh>
#include <fabrique/dag/Rule.hh>
#include <fabrique/dag/TypeReference.hh>
#include <fabrique/dag/Value.hh>

#include "Support/Bytestream.h"
#include "Support/Join.h"

#include <fabrique/types/FileType.hh>

#include <cassert>

using namespace fabrique::dag;
using namespace std;

using fabrique::backend::DotBackend;

namespace {

class DotFormatter : public Formatter
{
public:
	using Formatter::Format;

	string Format(const Boolean&);
	string Format(const Build&);
	string Format(const File&);
	string Format(const Function&);
	string Format(const Integer&);
	string Format(const List&);
	string Format(const Record&);
	string Format(const Rule&);
	string Format(const String&);
	string Format(const TypeReference&);
};

} // anonymous namespace


DotBackend* DotBackend::Create()
{
	return new DotBackend;
}


DotBackend::DotBackend()
	: indent_("\t")
{
}


void DotBackend::Process(const DAG& dag, Bytestream& out, ErrorReport::Report)
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
		const string name = formatter.Format(build);

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

		for (const shared_ptr<File>& f : build.inputs())
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

string DotFormatter::Format(const Build& b)
{
	vector<string> substrings;

	substrings.push_back(b.buildRule().name());
	substrings.push_back("{");

	for (auto& i : b.inputs())
		substrings.push_back(Format(*i));

	substrings.push_back("=>");

	for (auto& i : b.outputs())
		substrings.push_back(Format(*i));

	substrings.push_back("}");

	vector<pair<string,ValuePtr>> arguments;
	copy_if(b.arguments().begin(), b.arguments().end(), back_inserter(arguments),
		[](pair<string, ValuePtr> i)
		{
			return not fabrique::FileType::isFileOrFiles(i.second->type());
		});

	if (not arguments.empty())
	{
		substrings.push_back("(");

		for (auto& i : arguments)
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
	return f.relativeName();
}

string DotFormatter::Format(const Function&)
{
	return "";
}

string DotFormatter::Format(const Integer& i)
{
	return to_string(i.value());
}

string DotFormatter::Format(const List& l)
{
	vector<string> substrings;

	for (const shared_ptr<Value>& element : l)
		substrings.push_back(Format(*element));

	return fabrique::join(substrings, " ");
}

string DotFormatter::Format(const Record&)
{
	return "";
}

string DotFormatter::Format(const Rule& rule)
{
	return rule.command();
}

string DotFormatter::Format(const String& s)
{
	return "'" + s.value() + "'";
}

string DotFormatter::Format(const TypeReference& t)
{
	return t.referencedType().str();
}
