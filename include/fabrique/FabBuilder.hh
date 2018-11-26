//! @file FabBuilder.hh    Declaration of the FabBuilder class
/*
 * Copyright (c) 2018 Jonathan Anderson
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

#ifndef FAB_BUILDER_H_
#define FAB_BUILDER_H_

#include <fabrique/Fabrique.hh>
#include <fabrique/backend/Backend.hh>


namespace fabrique {

/**
 * Builder type for Fabrique instances.
 */
class FabBuilder
{
public:
	FabBuilder();
	Fabrique build();

	FabBuilder& parseOnly(bool p) { parseOnly_ = p; return *this; }
	FabBuilder& printASTs(bool p) { printASTs_ = p; return *this; }
	FabBuilder& printDAG(bool p) { printDAG_ = p; return *this; }
	FabBuilder& dumpASTs(bool p) { dumpASTs_ = p; return *this; }
	FabBuilder& printToStdout(bool p) { stdout_ = p; return *this; }

	FabBuilder& backends(std::vector<std::string> backendNames);
	FabBuilder& outputDirectory(std::string d);

	FabBuilder& pluginPaths(std::vector<std::string> paths)
	{
		pluginPaths_ = std::move(paths);
		return *this;
	}

	FabBuilder& regenerationCommand(std::string command)
	{
		regenCommand_ = std::move(command);
		return *this;
	}

private:
	bool parseOnly_;
	bool printASTs_;
	bool printDAG_;
	bool dumpASTs_;
	bool stdout_;

	UniqPtrVec<backend::Backend> backends_;
	Fabrique::ErrorReporter err_;
	std::string outputDir_;
	std::vector<std::string> pluginPaths_;
	std::string regenCommand_;
};

} // namespace fabrique

#endif  // FAB_BUILDER_H_
