/** @file FileList.cc    Definition of @ref FileList. */
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

#include "FileList.h"
#include "ostream.h"

using std::vector;


FileList* FileList::Take(vector<const File*> *files,
	vector<const Argument*> *args)
{
	std::auto_ptr<std::vector<const File*> > f(files);
	std::auto_ptr<std::vector<const Argument*> > a(args);
	if (a.get() == NULL)
		a.reset(new std::vector<const Argument*>);

	return new FileList(*f, *a);
}


bool FileList::isStatic() const
{
	for (auto *file : files)
		if (!file->isStatic())
			return false;

	for (auto *arg : args)
		if (!arg->isStatic())
			return false;

	return true;
}


void FileList::PrettyPrint(std::ostream& out, int indent) const
{
	out << Yellow << "[" << ResetAll;

	for (auto *file : files)
		out << " " << *file;

	for (auto *arg : args)
		out << ", " << *arg;

	out << Yellow << " ]" << ResetAll;
}
