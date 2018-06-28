/** @file DAG/Parameter.h    Declaration of @ref fabrique::dag::Parameter. */
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

#ifndef DAG_PARAMETER_H
#define DAG_PARAMETER_H

#include "Support/Printable.h"
#include "Support/SourceLocation.h"
#include "Support/Uncopyable.h"
#include "Types/Typed.h"

#include <memory>
#include <string>

namespace fabrique {
namespace dag {

class Value;


//! The result of evaluating an expression.
class Parameter : public HasSource, public Printable, public Typed,
                  public Uncopyable
{
public:
	Parameter(std::string name, const Type& type, std::shared_ptr<Value> defaultValue,
	          const SourceRange& = SourceRange::None());

	virtual ~Parameter() override;

	const std::string& name() const { return name_; }
	const std::shared_ptr<Value>& defaultValue() const
	{
		return defaultValue_;
	}

	virtual void PrettyPrint(Bytestream&, unsigned int indent) const override;

private:
	const std::string name_;
	const std::shared_ptr<Value> defaultValue_;
};

} // namespace dag
} // namespace fabrique

#endif // !DAG_PARAMETER_H
