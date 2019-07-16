//! @file strings.cc    Definition of string manipulation functions (join, split, etc.)
/*
 * Copyright (c) 2013, 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme and at Memorial University
 * of Newfoundland under the NSERC Discovery program (RGPIN-2015-06048).
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

#include <fabrique/Bytestream.hh>
#include <fabrique/strings.hh>

#include <cassert>
#include <iterator>

using namespace fabrique;
using std::string;


string fabrique::join(const string& x, const string& y, const string& delim)
{
	if (x.empty())
		return y;

	return x + delim + y;
}


std::vector<string> fabrique::Split(const std::string& s, const std::string delim)
{
	long long lastDelimiter = -1;
	std::vector<string> parts;

	while (true)
	{
		assert(lastDelimiter >= -1);
		size_t last = static_cast<size_t>(lastDelimiter) + delim.length();

		size_t next = s.find(delim, last);
		if (next == string::npos)
			break;

		const size_t len = next - last;
		parts.push_back(s.substr(last, len));

		assert(next >= 0);
		lastDelimiter = static_cast<long long>(next);
	}

	assert(lastDelimiter >= -1);
	parts.push_back(s.substr(static_cast<size_t>(lastDelimiter) + delim.length()));

	return parts;
}
