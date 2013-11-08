/** @file Rule.cc    Definition of @ref Rule. */
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
#include "DAG/Rule.h"
#include "Support/Bytestream.h"

using namespace fabrique::dag;
using std::string;


Rule* Rule::Create(const string command, const StringMap<string>& parameters)
{
	StringMap<string> params(parameters);

	string description;

	auto d = params.find("description");
	if (d == params.end())
	{
		description = command;
	}
	else
	{
		description = d->second;
		params.erase(d);
	}

	return new Rule(command, description, params);
}


Rule::Rule(const string& command, const string& description,
	   const StringMap<string>& params)
	: command(command), description(description), parameters(params)
{
}

void Rule::PrettyPrint(Bytestream& out, int indent) const
{
	out
		<< Bytestream::Action << command
		<< Bytestream::Operator << " {"
		<< Bytestream::Literal << " '" << description << "'"
		;

	for (auto& i : parameters)
	{
		out
			<< Bytestream::Operator << ", "
			<< Bytestream::Definition << i.first
			<< Bytestream::Operator << " = "
			<< Bytestream::Literal << "'" << i.second << "'"
			;
	}

	out
		<< Bytestream::Operator << " }"
		<< Bytestream::Reset
		;
}
