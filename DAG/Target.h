/** @file Target.h    Declaration of @ref Target. */
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

#ifndef DAG_TARGET_H
#define DAG_TARGET_H

#include "ADT/PtrVec.h"
#include "DAG/Value.h"

#include <string>


namespace fabrique {
namespace dag {

class Build;
class File;
class List;


//! A named build target that references a set of files.
class Target : public Value
{
public:
	static Target* Create(const std::string& name,
	                      const std::shared_ptr<Build>&);

	static Target* Create(const std::string& name,
	                      const std::shared_ptr<File>&);

	static Target* Create(const std::string& name,
	                      const std::shared_ptr<List>&);

	const std::string& name() const { return name_; }
	const SharedPtrVec<File>& files() const { return files_; }

	void PrettyPrint(Bytestream&, int indent = 0) const;

private:
	Target(const std::string& name, const SharedPtrVec<File>& files,
	       const SourceRange&, const Type&);

	const std::string name_;
	const SharedPtrVec<File> files_;
};

} // namespace dag
} // namespace fabrique

#endif
