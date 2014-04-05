/** @file DAG/DAG.h    Declaration of @ref fabrique::dag::DAG. */
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

#ifndef DAG_H
#define DAG_H

#include "ADT/StringMap.h"
#include "ADT/UniqPtr.h"
#include "DAG/Value.h"
#include "Support/Printable.h"

#include <string>


namespace fabrique {

class FabContext;

namespace ast {
	class Expression;
	class Identifier;
	class Scope;
}

namespace dag {

class Build;
class File;
class Rule;
class Target;
class Value;


/**
 * A directed acyclic graph of build actions.
 */
class DAG : public Printable
{
public:
	static UniqPtr<DAG> Flatten(const ast::Scope&, FabContext&);

	virtual const std::string& buildroot() const = 0;
	virtual const std::string& srcroot() const = 0;

	virtual const SharedPtrVec<File>& files() const = 0;
	virtual const SharedPtrVec<Build>& builds() const = 0;
	virtual const SharedPtrMap<Rule>& rules() const = 0;
	virtual const SharedPtrMap<Value>& variables() const = 0;
	virtual const SharedPtrMap<Target>& targets() const = 0;

	virtual void PrettyPrint(Bytestream&, size_t indent = 0) const override;
};

} // namespace dag
} // namespace fabrique

#endif
