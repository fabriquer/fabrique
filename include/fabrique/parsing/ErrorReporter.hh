//! @file parsing/ErrorReporter.hh    Declaration of @ref fabrique::parser::ErrorReporter
/*
 * Copyright (c) 2016, 2018 Jonathan Anderson
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

#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include <fabrique/ErrorReport.hh>


namespace fabrique {
namespace parser {

/**
 * Sink for error reports related to parsing.
 */
class ErrorReporter
{
	public:
	ErrorReporter(std::vector<ErrorReport>&);

	bool hasErrors() const;

	ErrorReport& ReportError(std::string, SourceRange,
		ErrorReport::Severity = ErrorReport::Severity::Error,
		std::string detail = "");
	ErrorReport& ReportError(std::string, const HasSource&,
		ErrorReport::Severity = ErrorReport::Severity::Error,
		std::string detail = "");

	private:
	std::vector<ErrorReport>& errors_;
};

} // namespace parser
} // namespace fabrique

#endif
