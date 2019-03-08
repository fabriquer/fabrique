/**
 * @file  fabrique/SourceLocation.h
 * @brief Declaration of @ref fabrique::SourceLocation
 */
/*
 * Copyright (c) 2013, 2016, 2018-2019 Jonathan Anderson
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

#ifndef FAB_SOURCE_LOCATION_H_
#define FAB_SOURCE_LOCATION_H_

#include <fabrique/Printable.hh>
#include <fabrique/UniqPtr.h>

#include <string>

namespace fabrique {

//! A location in the original source code.
class SourceLocation : public Printable
{
public:
	SourceLocation(const std::string& filename = "",
	               size_t line = 0, size_t column = 0);

	operator bool() const;
	bool operator < (const SourceLocation&) const;
	bool operator > (const SourceLocation&) const;
	bool operator == (const SourceLocation&) const;
	bool operator != (const SourceLocation&) const;

	virtual void PrettyPrint(Bytestream&, unsigned int indent = 0) const override;

	std::string filename;
	size_t line;
	size_t column;
};

} // class fabrique

#endif  // FAB_SOURCE_LOCATION_H_
