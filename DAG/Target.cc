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


Target* Target::Create(const string& name, shared_ptr<Build>& build)
{
	assert(build);

	SharedPtrVec<File> files(build->allOutputs());
	assert(not files.empty());

	if (build->type().isFile())
		assert(files.size() == 1);

	return new Target(name, files, build->source(), build->type());
}

Target* Target::Create(const string& name, shared_ptr<File>& file)
{
	SharedPtrVec<File> files(1, file);
	return new Target(name, files, file->source(), file->type());
}

Target* Target::Create(const string& name, shared_ptr<List>& list)
{
	for (auto& f : *list)
		assert(f);

	SharedPtrVec<File> files;
	for (auto i : *list)
	{
		//
		// If we are referencing an existing target,
		// pass its files through to the new target.
		//
		if (auto t = dynamic_pointer_cast<Target>(i))
		{
			const SharedPtrVec<File>& f = t->files();
			files.insert(files.end(), f.begin(), f.end());
		}

		//
		// Otherwise, we had better have a file.
		//
		else if (auto f = dynamic_pointer_cast<File>(i))
			files.push_back(f);

		else
			assert(false && "not a Target or File");
	}

	return new Target(name, files, list->source(), list->type());
}


Target::Target(const string& name, const SharedPtrVec<File>& files,
               const SourceRange& src, const Type& ty)
	: Value(ty, src), name_(name), files_(files)
{
}


void Target::PrettyPrint(Bytestream& out, int indent) const
{
	if (type().isFile())
		out << *files_.front();

	else
	{
		out << Bytestream::Operator << "[";

		for (auto f : files_)
			out << " " << *f;

		out << Bytestream::Operator << " ]";
	}
}
