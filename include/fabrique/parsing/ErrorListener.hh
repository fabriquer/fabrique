/**
 * @file Parsing/ParseErrorStrategy.h
 * Declaration of @ref fabrique::ast::ParseErrorStrategy.
 */
/*
 * Copyright (c) 2018 Jonathan Anderson
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

#ifndef PARSING_ERROR_LISTENER_H_
#define PARSING_ERROR_LISTENER_H_

#include <antlr-cxx-runtime/BaseErrorListener.h>

namespace fabrique {
namespace parsing {

class ErrorListener : public antlr4::BaseErrorListener
{
public:
	/**
	 * Constructor.
	 *
	 * @param   filename      name of the file being parsed (will be used to find
	 *                        source files in which errors are being reported)
	 */
	ErrorListener(std::string filename);

	//
	// ANTLRErrorListener implementation:
	//
	virtual void syntaxError(
		antlr4::Recognizer *, antlr4::Token *, size_t line, size_t column,
		const std::string &msg, std::exception_ptr e) override;

private:
	const std::string filename_;
};

} // namespace parsing
} // namespace fabrique

#endif /* PARSING_ERROR_LISTENER_H_ */
