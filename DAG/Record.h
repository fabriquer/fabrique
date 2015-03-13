/** @file DAG/Record.h    Declaration of @ref fabrique::dag::Record. */
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

#ifndef DAG_STRUCTURE_H
#define DAG_STRUCTURE_H

#include "ADT/StringMap.h"
#include "DAG/Value.h"

#include <memory>


namespace fabrique {
namespace dag {

/**
 * A reference to a file on disk (source or target).
 */
class Record : public Value
{
public:
	// TODO: don't promise anything about ordering in the layout
	typedef std::pair<std::string,ValuePtr> NamedValue;

	//! Create a record from an (optionally empty) vector of values.
	static Record* Create(const std::vector<NamedValue>&, const Type&, SourceRange);

	//! Create a record from a non-empty vector of values.
	static Record* Create(const std::vector<NamedValue>&, SourceRange);

	virtual ~Record();

	virtual bool hasFields() const override { return true; }
	virtual ValuePtr field(const std::string& name) const override;
	ValuePtr operator[] (const std::string& name) const
	{
		return field(name);
	}

	virtual void PrettyPrint(Bytestream&, size_t indent = 0) const override;
	void Accept(Visitor&) const override;

private:
	Record(const std::vector<NamedValue>&, const Type&, SourceRange);

	const std::vector<NamedValue> values_;
};

} // namespace dag
} // namespace fabrique

#endif