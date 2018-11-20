//! @file  names.hh    Names defined or reserved by the Fabrique language
/*
 * Copyright (c) 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed at Memorial University of Newfoundland
 * under the NSERC Discovery program (RGPIN-2015-06048).
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

#ifndef FAB_NAMES_H_
#define FAB_NAMES_H_

#include <string>

namespace fabrique {
namespace names {

//
// Reserved names: it is not permissible to define values with these names.
//
static const char Action[] = "action";
static const char And[] = "and";
static const char Arguments[] = "args";
static const char Bool[] = "bool";
static const char BuildDirectory[] = "builddir";
static const char BuildRoot[] = "buildroot";
static const char False[] = "false";
static const char File[] = "file";
static const char Files[] = "files";
static const char Function[] = "function";
static const char Import[] = "import";
static const char In[] = "in";
static const char Int[] = "int";
static const char List[] = "list";
static const char Nil[] = "nil";
static const char Not[] = "not";
static const char Or[] = "or";
static const char Out[] = "out";
static const char Record[] = "record";
static const char SourceRoot[] = "srcroot";
static const char String[] = "string";
static const char True[] = "true";
static const char Type[] = "type";
static const char XOr[] = "xor";

//
// Other names defined to have specific meanings in some contexts
// but still usable as user-defined names.
//
static const char Basename[] = "basename";
static const char Extension[] = "extension";
static const char Generated[] = "generated";
static const char FileName[] = "filename";
static const char FullName[] = "fullname";
static const char Name[] = "name";
static const char Subdirectory[] = "subdir";

bool reservedName(const std::string&);


} // namespace names
} // namespace fabrique

#endif  // FAB_NAMES_H_
