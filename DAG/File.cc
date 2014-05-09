/** @file DAG/File.cc    Definition of @ref fabrique::dag::File. */
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
#include "DAG/Visitor.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Support/os.h"

using namespace fabrique::dag;
using std::shared_ptr;
using std::string;


File* File::Create(string fullPath, const Type& t, SourceRange src)
{
	return new File(fullPath, PathIsAbsolute(fullPath), t, src);
}

File* File::Create(string directory, string filename, const Type& t,
                   SourceRange src)
{
	string fullName(JoinPath(directory, filename));
	return Create(fullName, t, src);
}


File::File(string filename, bool absolute, const Type& t, SourceRange source)
	: Value(t, source), filename_(filename),
	  absolute_(absolute), generated_(false)
{
}


string File::filename() const
{
	if (absolute_)
		return filename_;

	if (subdirectory_.empty())
		return filename_;

	return JoinPath(subdirectory_, filename_);
}


string File::fullName() const
{
	if (absolute_)
		return filename_;

	return JoinPath(generated() ? "${buildroot}" : "${srcroot}", filename());
}


void File::setGenerated(bool gen)
{
	if (absolute_ and gen)
		throw SemanticException(
			"cannot generate a file with absolute path", source());

	generated_ = gen;
}



shared_ptr<Value> File::Add(shared_ptr<Value>& suffix)
{
	shared_ptr<File> f(new File(filename_ + suffix->str(), absolute_, type(),
	                            SourceRange::Over(this, suffix)));

	f->setSubdirectory(subdirectory_);

	return f;
}


shared_ptr<Value> File::PrefixWith(shared_ptr<Value>& prefix)
{
	shared_ptr<File> f(new File(prefix->str() + filename_, absolute_, type(),
	                            SourceRange::Over(prefix, this)));

	f->setSubdirectory(subdirectory_);

	return f;
}


void File::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out << Bytestream::Literal << filename() << Bytestream::Reset;
}


void File::Accept(Visitor& v) const
{
	v.Visit(*this);
}
