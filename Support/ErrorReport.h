/** @file Support/ErrorReport.h    Declaration of @ref fabrique::ErrorReport. */
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

#ifndef ERROR_REPORT_H
#define ERROR_REPORT_H

#include "Support/Printable.h"
#include "Support/SourceLocation.h"

#include <functional>
#include <ostream>
#include <string>

namespace fabrique {

//! A non-exceptional representation of a problem in source code.
class ErrorReport : public HasSource, public Printable
{
public:
	enum class Severity
	{
		Error,
		Warning,
		Message,
	};

	typedef std::function<void (std::string, SourceRange, Severity)> Report;

	static ErrorReport*
	Create(const std::string& message,
	       const SourceRange& loc = SourceRange::None(),
	       Severity severity = Severity::Error,
	       unsigned int contextLines = 3);

	virtual ~ErrorReport() {}

	const std::string& getMessage() const { return message_; }
	void PrettyPrint(Bytestream& out, unsigned int indent = 0) const override;

private:
	ErrorReport(const std::string& message, const SourceRange& range,
	            const SourceLocation& loc, Severity severity, unsigned int lines)
		: HasSource(range), severity_(severity), message_(message),
		  caret_(loc), contextLines_(lines)
	{
	}

	const Severity severity_;
	const std::string message_;
	const SourceLocation caret_;
	const unsigned int contextLines_;
};

} // namespace fabrique

#endif
