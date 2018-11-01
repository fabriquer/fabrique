/** @file Plugin/Loader.cc    Definition of @ref fabrique::plugin::Loader. */
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

#include <fabrique/platform/SharedLibrary.hh>
#include <fabrique/platform/files.hh>
#include <fabrique/plugin/Loader.hh>
#include <fabrique/plugin/Registry.hh>
#include "Support/Bytestream.h"
#include "Support/Join.h"

using namespace fabrique;
using namespace fabrique::platform;
using namespace fabrique::plugin;
using std::string;
using std::vector;


Loader::Loader(const vector<string>& paths)
	: paths_(paths)
{
}


std::weak_ptr<Plugin::Descriptor> Loader::Load(string name)
{
	const string libname = LibraryFilename(name);

	Bytestream& dbg = Bytestream::Debug("plugin.loader");
	dbg
		<< Bytestream::Action << "searching"
		<< Bytestream::Reset << " for "
		<< Bytestream::Type << "plugin "
		<< Bytestream::Literal << name
		<< Bytestream::Operator << " ("
		<< Bytestream::Literal << libname
		<< Bytestream::Operator << ")"
		<< Bytestream::Reset << " in paths "
		<< Bytestream::Operator << "["
		<< Bytestream::Reset << " "
		<< Bytestream::Literal << join(paths_, " ")
		<< Bytestream::Operator << " ]"
		<< Bytestream::Reset << "\n"
		;

	const string filename = FindFile(libname, paths_, FileIsSharedLibrary,
	                                 DefaultFilename(""));

	dbg
		<< Bytestream::Reset << "found "
		<< Bytestream::Operator << "'"
		<< Bytestream::Literal << filename
		<< Bytestream::Operator << "'"
		<< Bytestream::Reset << "\n"
		;
	if (filename.empty())
		return std::weak_ptr<Plugin::Descriptor>();

	libraries_.emplace_back(SharedLibrary::Load(filename));
	return Registry::get().lookup(name);
}
