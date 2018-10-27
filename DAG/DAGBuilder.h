/** @file DAG/DAGBuilder.h    Declaration of @ref fabrique::dag::DAGBuilder. */
/*
 * Copyright (c) 2014 Jonathan Anderson
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

#ifndef DAG_BUILDER_H
#define DAG_BUILDER_H

#include <string>

#include "DAG/Function.h"
#include "DAG/Record.h"
#include "DAG/Value.h"


namespace fabrique {

class CLIArguments;
class FileType;
class FunctionType;
class TypeContext;

namespace dag {

class Build;
class DAG;
class File;
class Parameter;
class Rule;


//! A object that builds @ref DAG nodes in a @ref DAGBuilder::Context.
class DAGBuilder
{
public:
	//! An object that can supply a @ref DAGBuilder with names and types.
	class Context
	{
		public:
		virtual ~Context();

		virtual std::string currentValueName() const = 0;
		virtual TypeContext& types() const = 0;
	};


	DAGBuilder(Context&);


	//! Construct a @ref DAG from the current @ref DAGBuilder state.
	UniqPtr<DAG> dag(std::vector<std::string> topLevelTargets) const;

	TypeContext& typeContext() { return ctx_.types(); }


	//! Define a variable with a name and a value.
	void Define(std::string name, ValuePtr);

	/**
	 * Add the build steps required to regenerate the @ref DAG if
	 * Fabrique input files change.
	 */
	ValuePtr AddRegeneration(const CLIArguments& commandLineArguments,
                                 const std::vector<std::string>& inputFiles,
                                 const std::vector<std::string>& outputFiles);

	//! Create a @ref dag::Boolean.
	ValuePtr Bool(bool, SourceRange);

	//! Construct a @ref dag::Build from a @ref dag::Rule and parameters.
	std::shared_ptr<class Build>
	Build(std::shared_ptr<class Rule>, ValueMap, SourceRange);

	//! Create a @ref dag::File from a path.
	ValuePtr File(std::string fullPath, const ValueMap& attributes,
	              const FileType&, const SourceRange& src = SourceRange::None(),
	              bool generated = false);

	//! Create a @ref dag::File from a subdirectory and a filename.
	ValuePtr File(std::string subdir, std::string filename,
	              const ValueMap& attributes, const FileType&,
	              const SourceRange& src = SourceRange::None(),
	              bool generated = false);

	//! Define a @ref dag::Function.
	ValuePtr Function(const Function::Evaluator, const SharedPtrVec<Parameter>&,
	                  const FunctionType&, SourceRange = SourceRange::None());

	//! Create a @ref dag::Integer.
	ValuePtr Integer(int, SourceRange);

	//! Create a @ref dag::Rule.
	ValuePtr Rule(std::string command, const ValueMap& arguments,
	              const SharedPtrVec<Parameter>& parameters, const Type&,
	              const SourceRange& from = SourceRange::None());

	//! Create a @ref dag::String.
	ValuePtr String(const std::string&, SourceRange = SourceRange::None());

	//! Create a @ref dag::Record.
	std::shared_ptr<Record> Record(const ValueMap&, SourceRange = SourceRange::None());


protected:
	std::shared_ptr<class Rule> MakeDirectory() const;

	ValuePtr Rule(std::string name, std::string command,
	              const ValueMap& arguments,
	              const SharedPtrVec<Parameter>& parameters, const Type&,
	              const SourceRange& from = SourceRange::None());

	Context& ctx_;

	// Values we've created:
	SharedPtrVec<class File> files_;
	SharedPtrVec<class Build> builds_;
	SharedPtrMap<class Rule> rules_;
	SharedPtrMap<class Value> variables_;
	SharedPtrMap<class Value> targets_;

};

} // namespace dag
} // namespace fabrique

#endif
