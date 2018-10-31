/** @file Backend/Make.h    Declaration of fabrique::backend::MakeBackend. */
/*
 * Copyright (c) 2014 Jonathan Anderson
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

#ifndef MAKE_BACKEND_H
#define MAKE_BACKEND_H

#include <fabrique/backend/Backend.hh>

#include <string>


namespace fabrique {

class Bytestream;

namespace backend {

/**
 * A backend that produces POSIX make files (no BSD or GNU extensions).
 *
 * @sa http://pubs.opengroup.org/onlinepubs/009695399/utilities/make.html
 */
class MakeBackend : public Backend
{
public:
	enum class Flavour
	{
		POSIX,
		BSD,
		GNU,
	};

	static MakeBackend* Create(Flavour);

	Flavour flavour() const { return flavour_; }

	std::string DefaultFilename() const;
	void Process(const dag::DAG&, Bytestream&, ErrorReport::Report);

private:
	MakeBackend(Flavour);

	const Flavour flavour_;
	const std::string indent_;
};

} // namespace backend
} // namespace fabrique

#endif
