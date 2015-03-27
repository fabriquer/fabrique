/** @file DAG/Target.cc    Definition of @ref fabrique::dag::Target. */
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
#include "DAG/Record.h"
#include "DAG/Target.h"
#include "DAG/Visitor.h"
#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Support/exceptions.h"
#include "Types/Type.h"
#include "Types/TypeError.h"

#include <cassert>

using namespace fabrique::dag;
using fabrique::SharedPtrVec;
using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;


namespace fabrique
{
	static void Append(const SharedPtrVec<File>&, SharedPtrVec<File>&);
	static SharedPtrVec<File> Files(const ValuePtr& v);
}


Target* Target::Create(const string& name, const shared_ptr<Build>& build)
{
	assert(build);

	shared_ptr<List> l(List::of(Files(build), build->source(),
	                            build->type().context()));
	return new Target(name, l, build->type());
}

Target* Target::Create(const string& name, const shared_ptr<File>& file)
{
	assert(file);

	SharedPtrVec<Value> files;
	files.push_back(file);

	shared_ptr<List> l(List::of(files, file->source(),
	                            file->type().context()));
	return new Target(name, l, file->type());
}

Target* Target::Create(const string& name, const shared_ptr<List>& list)
{
	shared_ptr<List> l(List::of(Files(list), list->source(),
	                            list->type().context()));
	return new Target(name, l, list->type());
}


Target::Target(const string& name, const shared_ptr<List>& files, const Type& t)
	: Value(t, files->source()), name_(name), files_(files)
{
#ifndef NDEBUG
	for (auto& f : *files)
		assert(dynamic_pointer_cast<File>(f));
#endif
}


bool Target::hasFields() const
{
	// If this is a single-file target, pass through to the file.
	if (type().isFile())
	{
		assert(files_->size() == 1);
		const ValuePtr& file = *files_->begin();
		return file->hasFields();
	}

	// Otherwise, pass through to the list.
	return files_->hasFields();
}


ValuePtr Target::field(const std::string& name) const
{
	if (type().isFile())
	{
		assert(files_->size() == 1);
		const ValuePtr& file = *files_->begin();
		return file->field(name);
	}

	return files_->field(name);
}


// Just pass operations through to the underlying List.
ValuePtr Target::Add(ValuePtr& rhs) const
{
	return underlyingFiles()->Add(rhs);
}

ValuePtr Target::PrefixWith(ValuePtr& rhs) const
{
	return underlyingFiles()->PrefixWith(rhs);
}

ValuePtr Target::ScalarAdd(ValuePtr& rhs) const
{
	return underlyingFiles()->ScalarAdd(rhs);
}

ValuePtr Target::And(ValuePtr& rhs) const
{
	return underlyingFiles()->And(rhs);
}

ValuePtr Target::Or(ValuePtr& rhs) const
{
	return underlyingFiles()->Or(rhs);
}

ValuePtr Target::Xor(ValuePtr& rhs) const
{
	return underlyingFiles()->Xor(rhs);
}

ValuePtr Target::Equals(ValuePtr& rhs) const
{
	return underlyingFiles()->Equals(rhs);
}


void Target::PrettyPrint(Bytestream& out, size_t indent) const
{
	underlyingFiles()->PrettyPrint(out, indent);
}


void Target::Accept(Visitor& v) const
{
	if (v.Visit(*this))
	{
		underlyingFiles()->Accept(v);
	}
}


const ValuePtr Target::underlyingFiles() const
{
	if (type().isOrdered())
		return files_;

	assert(files_->size() == 1);
	return *files_->begin();
}


static void fabrique::Append(const SharedPtrVec<File>& source, SharedPtrVec<File>& dest)
{
	dest.insert(dest.end(), source.begin(), source.end());
}

static SharedPtrVec<File> fabrique::Files(const ValuePtr& v)
{
	SharedPtrVec<File> files;
	assert(v);

	if (auto file = dynamic_pointer_cast<File>(v))
	{
		files.push_back(file);
	}
	else if (auto build = dynamic_pointer_cast<Build>(v))
	{
		assert(not build->outputs().empty());
		if (build->type().isFile())
			assert(build->outputs().size() == 1);

		Append(build->outputs(), files);
	}
	else if (auto list = dynamic_pointer_cast<List>(v))
	{
		for (const ValuePtr& element : *list)
			Append(Files(element), files);
	}
	else if (auto record = dynamic_pointer_cast<Record>(v))
	{
		for (auto field : record->fields())
		{
			ValuePtr value = field.second;
			if (value->type().isFile())
				Append(Files(value), files);
		}
	}
	else if (auto target = dynamic_pointer_cast<Target>(v))
	{
		Append(Files(target->files()), files);
	}
	else
	{
		assert(false && "unhandled value type");
	}

	return files;
}
