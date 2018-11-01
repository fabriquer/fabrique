/** @file Support/PosixSharedLibrary.cc Definition of @ref fabrique::PosixSharedLibrary. */
/*
 * Copyright (c) 2014 Jonathan Anderson
 * All rights reserved.
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

#include "Support/exceptions.h"

#include "PosixOnly.h"
#include "PosixSharedLibrary.hh"

#include <string>

#include <dlfcn.h>

using namespace fabrique::platform;


PosixSharedLibrary::PosixSharedLibrary(void *handle)
	: libHandle_(handle)
{
}


PosixSharedLibrary::~PosixSharedLibrary()
{
	dlclose(libHandle_);
}


std::shared_ptr<SharedLibrary> SharedLibrary::Load(std::string path)
{
	void *handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
	if (not handle)
		throw OSError("unable to dlopen '" + path + "'", dlerror());

	return std::make_shared<PosixSharedLibrary>(handle);
}
