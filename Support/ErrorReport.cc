/** @file Support/ErrorReport.cc    Definition of @ref fabrique::ErrorReport. */
/*
 * Copyright (c) 2013, 2016, 2018 Jonathan Anderson
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
#include "Support/ErrorReport.h"

#include <cassert>
#include <fstream>
#include <list>
#include <map>

using namespace fabrique;
using std::string;


ErrorReport::ErrorReport(string message, SourceRange range, Severity severity,
                         string detail, SourceLocation loc, unsigned int lines)
	: HasSource(range), severity_(severity), message_(message),
	  detail_(detail), caret_(loc), contextLines_(lines)
{
}


void ErrorReport::PrettyPrint(Bytestream& out, unsigned int /*indent*/) const
{
	out << "\n" << source() << ": ";

	switch (severity_)
	{
		case Severity::Error:
			out << Bytestream::Error << "error";
			break;

		case Severity::Warning:
			out << Bytestream::Warning << "warning";
			break;

		case Severity::Message:
			out << Bytestream::Info << "info";
			break;
	}

	out
		<< Bytestream::Reset << ": "
		<< Bytestream::ErrorMessage << message_
		<< Bytestream::Reset << "\n"
		;

	source().PrintSource(out, caret_, contextLines_);

	out << Bytestream::Reset;

	if (not detail_.empty())
	{
		out
			<< Bytestream::Info << "\t" << detail_
			<< Bytestream::Reset << "\n";
	}
}
