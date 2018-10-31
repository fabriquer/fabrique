/** @file DAG/Visitor.h    Declaration of @ref fabrique::dag::Visitor. */
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

#ifndef DAG_FORMATTER_H
#define DAG_FORMATTER_H

#include <fabrique/dag/Visitor.hh>

#include <stack>
#include <string>

namespace fabrique {
namespace dag {

class Value;

//! An object that converts DAG nodes into strings.
class Formatter : public Visitor
{
public:
	std::string Format(const Value&);

	virtual std::string Format(const Boolean&) = 0;
	virtual std::string Format(const Build&) = 0;
	virtual std::string Format(const File&) = 0;
	virtual std::string Format(const Function&) = 0;
	virtual std::string Format(const Integer&) = 0;
	virtual std::string Format(const List&) = 0;
	virtual std::string Format(const Record&) = 0;
	virtual std::string Format(const Rule&) = 0;
	virtual std::string Format(const String&) = 0;
	virtual std::string Format(const TypeReference&) = 0;

	bool Visit(const Boolean&);
	bool Visit(const Build&);
	bool Visit(const File&);
	bool Visit(const Function&);
	bool Visit(const Integer&);
	bool Visit(const List&);
	bool Visit(const Record&);
	bool Visit(const Rule&);
	bool Visit(const String&);
	bool Visit(const TypeReference&);

private:
	std::stack<std::string> values_;
};

} // namespace dag
} // namespace fabrique

#endif
