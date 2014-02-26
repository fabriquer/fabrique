/** @file Target.cc    Definition of @ref Target. */
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
#include "DAG/Target.h"
#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Support/exceptions.h"
#include "Types/Type.h"
#include "Types/TypeError.h"

#include <cassert>

using namespace fabrique::dag;
using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;


Target* Target::Create(const string& name, const shared_ptr<Build>& build)
{
	assert(build);

	SharedPtrVec<File> files(build->allOutputs());
	assert(not files.empty());

	if (build->type().isFile())
		assert(files.size() == 1);

	shared_ptr<List> l(List::of(build->allOutputs(), build->source()));
	return new Target(name, l, build->type());
}

Target* Target::Create(const string& name, const shared_ptr<File>& file)
{
	assert(file);

	SharedPtrVec<Value> files;
	files.push_back(file);

	shared_ptr<List> l(List::of(files, file->source()));
	return new Target(name, l, file->type());
}

Target* Target::Create(const string& name, const shared_ptr<List>& list)
{
	for (auto& f : *list)
		assert(f);

	return new Target(name, list, list->type());
}


Target::Target(const string& name, const shared_ptr<List>& files, const Type& t)
	: Value(t, files->source()), name_(name), files_(files)
{
}


// Just pass operations through to the underlying List.
shared_ptr<Value> Target::Add(shared_ptr<Value>& rhs)
{
	return files_->Add(rhs);
}

shared_ptr<Value> Target::PrefixWith(shared_ptr<Value>& rhs)
{
	return files_->PrefixWith(rhs);
}

shared_ptr<Value> Target::ScalarAdd(shared_ptr<Value>& rhs)
{
	return files_->ScalarAdd(rhs);
}

shared_ptr<Value> Target::And(shared_ptr<Value>& rhs)
{
	return files_->And(rhs);
}

shared_ptr<Value> Target::Or(shared_ptr<Value>& rhs)
{
	return files_->Or(rhs);
}

shared_ptr<Value> Target::Xor(shared_ptr<Value>& rhs)
{
	return files_->Xor(rhs);
}


void Target::PrettyPrint(Bytestream& out, int indent) const
{
	if (type().isFile())
		out << **files_->begin();

	else
		out << *files_;
}
