//! @file  builtins.cc    Definitions of builtin functions
/*
 * Copyright (c) 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed at Memorial University of Newfoundland
 * under the NSERC Discovery program (RGPIN-2015-06048).
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

#include <fabrique/builtins.hh>
#include <fabrique/names.hh>
#include <fabrique/dag/DAGBuilder.hh>
#include <fabrique/dag/Parameter.hh>
#include <fabrique/types/FileType.hh>
#include <fabrique/types/TypeContext.hh>

#include "Support/exceptions.h"

using namespace fabrique;
using namespace fabrique::builtins;
using namespace fabrique::dag;


static ValuePtr OpenFileImpl(ValueMap arguments, DAGBuilder &b, SourceRange src)
{
	auto filename = arguments["name"];
	SemaCheck(filename, src, "missing filename");

	// TODO: look up subdirectory in the call context?
	std::string subdir;
	if (auto s = arguments[builtins::Subdirectory])
	{
		subdir = s->str();
	}

	return b.File(subdir, filename->str(), arguments, src);
}


fabrique::dag::ValuePtr fabrique::builtins::OpenFile(fabrique::dag::DAGBuilder &b)
{
	TypeContext &types = b.typeContext();

	SharedPtrVec<dag::Parameter> params;
	params.emplace_back(new Parameter("name", types.stringType()));

	return b.Function(OpenFileImpl, types.fileType(), params,
	                  SourceRange::None(), true);
}
