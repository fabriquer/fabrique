/** @file cstr.h    C string helpers. */
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

#ifndef CSTR_H
#define CSTR_H

#include <cassert>
#include <string>

namespace fabrique {

/**
 * A reference to a C string, which may not be long-lived.
 *
 * Very much like llvm::StringRef, but without having to include LLVM headers.
 */
class CStringRef
{
public:
	const char* begin() const { return cstr; }
	const char* end() const { return cstr + len; }

	size_t length() const { return len; }
	bool operator == (const std::string& s)
	{
		assert(cstr != NULL);

		return (len == s.length())
			and (strncmp(cstr, s.c_str(), len) == 0);
	}

	operator std::string() const { return str(); }
	std::string str() const
	{
		assert(cstr != NULL);
		return std::string(cstr, 0, len);
	}

	void set(char *s, size_t l) { cstr = s; len = l; }
	void set(char *b, char* e) { cstr = b; len = (e - b); }

	char *cstr;
	size_t len;
};

} // namespace fabrique

#endif
