/** @file File.cc    Definition of @ref File. */
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

#include "File.h"
#include "ostream.h"

#include <set>

using std::string;
using std::vector;


File* File::Source(const string& name, ExprList *arguments)
{
	std::vector<const Argument*> args;
	if (arguments != NULL)
		for (auto *a : *arguments)
			args.push_back(dynamic_cast<const Argument*>(a));

	delete arguments;
	return new File(name, args);
}

File* File::Source(const File *orig, ExprList *arguments)
{
	//! This is where (all concatenated) arguments go.
	std::vector<const Argument*> args(orig->args);

	std::set<string> argNames;
	for (auto *a : args)
		if (a->hasName())
			argNames.insert(a->getName().name());

	for (auto *a : *arguments)
	{
		auto *arg = dynamic_cast<const Argument*>(a);
		assert(arg->hasName());
		if (argNames.find(arg->getName().name()) == argNames.end())
			args.push_back(arg);
	}

	return new File(orig->name, args);
}


File* File::Target(const string& name, ExprList *arguments)
{
	std::vector<const Argument*> args;
	if (arguments != NULL)
		for (auto *a : *arguments)
			args.push_back(dynamic_cast<const Argument*>(a));

	delete arguments;
	return new File(name, args);
}


bool File::isStatic() const
{
	for (auto *a : args)
		if (!a->isStatic())
			return false;

	return true;
}

void File::PrettyPrint(std::ostream& out, int indent) const
{
	bool haveArgs = (args.size() > 0);

	if (haveArgs)
		out << Red << "file" << Yellow << "(";

	out << Magenta << name << ResetAll;

	for (auto *a : args)
		out << Yellow << ", " << ResetAll << *a;

	if (haveArgs)
		out << Yellow << ")" << ResetAll;
}
