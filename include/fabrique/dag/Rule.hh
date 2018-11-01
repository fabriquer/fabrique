/** @file DAG/Rule.h    Declaration of @ref fabrique::dag::Rule. */
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

#ifndef DAG_ACTION_H
#define DAG_ACTION_H

#include <fabrique/StringMap.h>
#include <fabrique/dag/Callable.hh>
#include <fabrique/dag/DAG.hh>
#include <fabrique/dag/Value.hh>

#include <string>


namespace fabrique {
namespace dag {

class File;


/**
 * An action that transforms files into other files.
 */
class Rule : public Callable, public Value
{
public:
	static const std::string& RegenerationRuleName();

	static Rule* Create(std::string name, std::string command,
	                    const ValueMap& arguments,
	                    const SharedPtrVec<Parameter>& parameters,
	                    const Type&,
	                    const SourceRange& from = SourceRange::None());

	virtual ~Rule() override {}

	ValuePtr Call(ValueMap, DAGBuilder&, SourceRange) const override;

	const std::string& name() const { return ruleName_; }
	const std::string& command() const { return command_; }

	bool hasDescription() const { return not description_.empty(); }
	const std::string& description() const { return description_; }

	//! Arguments define the action (e.g., command = 'cc').
	const ValueMap& arguments() const { return arguments_; }

	std::string str() const override { return command_; }

	void setSelf(std::weak_ptr<Rule>);

	virtual void PrettyPrint(Bytestream&, unsigned int indent = 0) const override;
	void Accept(Visitor& v) const override;

private:
	Rule(const std::string& name, const std::string& command,
	     const std::string& description, const ValueMap& args,
	     const SharedPtrVec<Parameter>& parameters,
	     const Type&, SourceRange location);

	const std::string ruleName_;
	const std::string command_;
	const std::string description_;
	const ValueMap arguments_;

	//! We need something to pass to @ref Build constructors.
	std::weak_ptr<Rule> self_;
};

} // namespace dag
} // namespace fabrique

#endif
