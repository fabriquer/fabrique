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

#ifndef DAG_VISITOR_H
#define DAG_VISITOR_H

namespace fabrique {
namespace dag {

class Boolean;
class Build;
class File;
class Function;
class Integer;
class List;
class Record;
class Rule;
class String;
class Target;


//! Interfact for objects that visit @ref DAG nodes and add functionality.
class Visitor
{
public:
	virtual ~Visitor();

	virtual bool Visit(const Boolean&) = 0;
	virtual bool Visit(const Build&) = 0;
	virtual bool Visit(const File&) = 0;
	virtual bool Visit(const Function&) = 0;
	virtual bool Visit(const Integer&) = 0;
	virtual bool Visit(const List&) = 0;
	virtual bool Visit(const Record&) = 0;
	virtual bool Visit(const Rule&) = 0;
	virtual bool Visit(const String&) = 0;
	virtual bool Visit(const Target&) = 0;
};

} // namespace dag
} // namespace fabrique

#endif
