/** @file Rule.h    Declaration of @ref Rule. */
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

#ifndef DAG_ACTION_H
#define DAG_ACTION_H

#include "ADT/StringMap.h"
#include "DAG/DAG.h"
#include "DAG/Value.h"
#include "Support/Printable.h"

#include <string>


namespace fabrique {
namespace dag {

class File;


/**
 * An action that transforms files into other files.
 */
class Rule : public Value
{
public:
	static Rule* Create(std::string name, std::string command,
	                    const ValueMap& otherParameters,
	                    const SourceRange from = SourceRange::None());

	virtual ~Rule() {}

	const std::string& name() const { return ruleName; }
	const std::string& command() const { return cmd; }
	const std::string& description() const { return descrip; }
	const ValueMap& parameters() const { return params; }

	std::string type() const { return "rule"; }
	std::string str() const { return cmd; }

	void PrettyPrint(Bytestream&, int indent = 0) const;

private:
	Rule(const std::string& name, const std::string& command,
	     const std::string& description, const ValueMap& params,
	     SourceRange location);

	const std::string ruleName;
	const std::string cmd;
	const std::string descrip;
	const ValueMap params;
};

} // namespace dag
} // namespace fabrique

#endif
