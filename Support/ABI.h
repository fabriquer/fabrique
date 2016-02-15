/** @file Parsing/ABI.h    Declaration of ABI helper functions. */
/*
 * Copyright (c) 2015 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed at Memorial University of Newfoundland under
 * the NSERC Discovery program (RGPIN-2015-06048).
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

#ifndef SUPPORT_ABI_H
#define SUPPORT_ABI_H

#include <string>
#include <typeinfo>

namespace fabrique {

/**
 * Demangle a C++ name according the current compiler's ABI.
 *
 * Thanks to http://stackoverflow.com/a/4541470/611594.
 */
std::string DemangleABIName(const std::string&);

/**
 * Demangle the name inside a C++ type_info object.
 */
std::string Demangle(const std::type_info&);

template<class T>
std::string TypeName(const T& value)
{
	return Demangle(typeid(value));
}

} // namespace fabrique

#endif