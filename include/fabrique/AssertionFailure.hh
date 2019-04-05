//! @file AssertionFailure.hh    Declaration of fabrique::AssertionFailure
/*
 * Copyright (c) 2013, 2018-2019 Jonathan Anderson
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

#ifndef FAB_ASSERTION_FAILURE_H_
#define FAB_ASSERTION_FAILURE_H_

#include <exception>
#include <string>


namespace fabrique {

//! Some code may choose to throw this exception rather than assert() out.
class AssertionFailure : public std::exception
{
public:
	AssertionFailure(const std::string& condition,
	                 const std::string& message = "");

	AssertionFailure(const AssertionFailure&);

	virtual ~AssertionFailure() override;

	const char* what() const noexcept override;
	const std::string& condition() const noexcept { return condition_; }
	const std::string& message() const noexcept { return message_; }

private:
	const std::string condition_;
	const std::string message_;
};

} // namespace fabrique


#define FAB_ASSERT(expr, detail) \
	do \
	{ \
		if (not (expr)) \
		{ \
			throw fabrique::AssertionFailure(#expr, detail); \
		} \
	} \
	while (false)

#endif // FAB_ASSERTION_FAILURE_H_
