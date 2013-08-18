/** @file File.h    Declaration of @ref File. */
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

#ifndef FILE_H
#define FILE_H

#include "Argument.h"
#include "Expression.h"

#include <vector>


/**
 * A reference to a file on disk (source or target).
 */
class File : public Expression
{
public:
	/**
	 * Create a source file, which is expected to be present on disk when
	 * fab is run.
	 */
	static File* Source(
		const std::string& name, ExprList *arguments = NULL);

	static File* Source(const File *orig, ExprList *arguments = NULL);

	/**
	 * Create a target file, which only exists at build time as a result
	 * of a build action.
	 */
	static File* Target(
		const std::string& name, ExprList *arguments = NULL);

	static File* Target(const File* orig, ExprList *arguments = NULL);

	~File() { for (auto *arg : args) delete arg; }

	virtual bool isStatic() const;
	virtual void PrettyPrint(std::ostream&, int indent = 0) const;

private:
	File(const std::string& name, std::vector<const Argument*>& args)
		: name(name), args(args)
	{
	}

	const std::string name;
	const std::vector<const Argument*> args;
};

#endif
