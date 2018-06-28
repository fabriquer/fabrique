/** @file DAG/Build.h    Declaration of @ref fabrique::dag::Build. */
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

#ifndef DAG_BUILD_H
#define DAG_BUILD_H

#include "DAG/DAG.h"
#include "DAG/File.h"
#include "DAG/Value.h"

#include <memory>
#include <vector>


namespace fabrique {
namespace dag {

class Rule;


/**
 * An application of a @ref fabrique::dag::Rule to transform @ref File objects.
 */
class Build : public Value
{
public:
	static Build* Create(std::shared_ptr<Rule>&, SharedPtrMap<Value>& args,
	                     const SourceRange&);

	virtual ~Build() override {}

	const Rule& buildRule() const { return *rule_; }

	const FileVec inputs() const { return in_; }
	const FileVec outputs() const { return out_; }

	const ValueMap& arguments() const { return args_; }

	virtual bool hasFields() const override;
	virtual ValuePtr field(const std::string& name) const override;

	virtual ValuePtr Negate(const SourceRange& loc) const override;
	virtual ValuePtr Add(ValuePtr&) const override;
	virtual ValuePtr PrefixWith(ValuePtr&) const override;
	virtual ValuePtr ScalarAdd(ValuePtr&) const override;
	virtual ValuePtr And(ValuePtr&) const override;
	virtual ValuePtr Or(ValuePtr&) const override;
	virtual ValuePtr Xor(ValuePtr&) const override;
	virtual ValuePtr Equals(ValuePtr&) const override;

	virtual void PrettyPrint(Bytestream&, unsigned int indent = 0) const override;
	void Accept(Visitor& v) const override;

private:
	Build(std::shared_ptr<Rule>&,
	      SharedPtrVec<File>& inputs,
	      SharedPtrVec<File>& outputs,
	      const ValueMap& arguments,
	      const Type&,
	      SourceRange);

	static void AppendFiles(const ValuePtr& in, SharedPtrVec<File>& out,
	                        bool generated = false);

	ValuePtr outputValue() const;

	std::shared_ptr<Rule> rule_;
	SharedPtrVec<File> in_;
	SharedPtrVec<File> out_;
	ValueMap args_;
};

} // namespace dag
} // namespace fabrique

#endif
