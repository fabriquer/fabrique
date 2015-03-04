/** @file DAG/DAG.cc    Definition of @ref fabrique::dag::DAG. */
/*
 * Copyright (c) 2013-2014 Jonathan Anderson
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
#include "DAG/DAG.h"
#include "DAG/File.h"
#include "DAG/Function.h"
#include "DAG/Rule.h"
#include "DAG/Target.h"

#include "Support/Bytestream.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::dag;

using std::shared_ptr;
using std::string;


void DAG::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	SharedPtrMap<Value> namedValues;
	for (auto& i : rules()) namedValues.emplace(i);
	for (auto& i : targets()) namedValues.emplace(i);
	for (auto& i : variables()) namedValues.emplace(i);

	for (auto& i : namedValues)
	{
		const string& name = i.first;
		const ValuePtr& v = i.second;

		assert(v);

		out
			<< Bytestream::Definition << name
			<< Bytestream::Operator << ":"
			<< Bytestream::Type << v->type()
			<< Bytestream::Operator << " = "
			<< *v
			<< Bytestream::Reset << "\n"
			;
	}

	for (const shared_ptr<File>& f : files())
	{
		out
			<< Bytestream::Type << f->type()
			<< Bytestream::Operator << ": "
			<< *f
			<< Bytestream::Reset << "\n"
			;
	}

	for (const shared_ptr<Build>& b : builds())
	{
		out
			<< Bytestream::Type << "build"
			<< Bytestream::Operator << ": "
			<< *b
			<< Bytestream::Reset << "\n"
			;
	}
}
