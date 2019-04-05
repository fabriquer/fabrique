//! @file platform/OSError.hh    Declaration of platform::OSError
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

#ifndef FAB_PLATFORM_OSERROR_H_
#define FAB_PLATFORM_OSERROR_H_

#include <fabrique/Printable.hh>

#include <exception>

namespace fabrique {
namespace platform {

//! An error that has an OS-specific description.
class OSError : public std::exception, public Printable
{
public:
	OSError(const std::string& message, const std::string& description);
	OSError(const OSError&);
	virtual ~OSError() override;

	virtual const std::string& message() const { return message_; }
	virtual const std::string& description() const { return description_; }

	const char* what() const noexcept override { return completeMessage_.c_str(); }
	virtual void PrettyPrint(Bytestream&, unsigned int indent = 0) const override;

private:
	const std::string message_;
	const std::string description_;
	const std::string completeMessage_;
};

} // namespace platform
} // namespace fabrique

#endif // FAB_PLATFORM_OSERROR_H_
