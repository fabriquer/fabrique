/** @file Join.h    Declaration of the @ref Join ostream helper. */
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

#ifndef JOIN_H
#define JOIN_H

#include "ADT/PtrVec.h"

#include <sstream>
#include <string>


namespace fabrique {

class Bytestream;
class Printable;


template<class T>
class Join
{
public:
	static Join csv(const PtrVec<T>& p) { return Join(", ", p); }
	static Join ssv(const PtrVec<T>& p) { return Join(" ", p); }

	Join(std::string j, const PtrVec<T>& p)
		: joinStr(j), objects(p)
	{
	}

	void Print(Bytestream& out) const
	{
		for (size_t i = 0; i < objects.size(); )
		{
			out << *objects[i];
			if (++i < objects.size())
				out << joinStr;
		}
	}


private:
	const std::string joinStr;
	const PtrVec<T>& objects;
};

template<class T>
Bytestream& operator<< (Bytestream& out, const Join<T>& j)
{
	j.Print(out);
	return out;
}


template<class T>
std::string join(const T& c, const std::string& delim = ",")
{
	if (c.empty())
		return "";

	std::stringstream buffer;
	std::move(c.begin(), --c.end(),
	          std::ostream_iterator<std::string>(buffer, delim.c_str()));
	buffer << *(--c.end());

	return buffer.str();
}

std::string join(const std::string&, const std::string&,
                 const std::string& delim = ", ");

} // namespace fabrique

#endif
