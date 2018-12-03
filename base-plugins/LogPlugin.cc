//! @file plugins/LogPlugin.cc   Definition of @ref LogPlugin
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

#include <sys/types.h>

#include <fabrique/Bytestream.hh>
#include <fabrique/dag/DAGBuilder.hh>
#include <fabrique/dag/List.hh>
#include <fabrique/dag/Parameter.hh>
#include <fabrique/plugin/Registry.hh>
#include <fabrique/types/TypeContext.hh>
#include "Support/exceptions.h"

using namespace fabrique;
using namespace fabrique::dag;

using fabrique::plugin::Plugin;
using std::shared_ptr;
using std::string;


namespace {

/**
 * Provides access to the sysctl(3) set of C library functions.
 *
 * Many useful properties of the system are represented (or controlled) with sysctl(3)
 * entries. For instance, Fabrique build descriptions might like to inspect the values
 * of kern.ostype, kern.osrelease, etc.
 */
class LogPlugin : public plugin::Plugin
{
	public:
	virtual string name() const override { return "log"; }
	virtual shared_ptr<dag::Record>
		Create(dag::DAGBuilder&, const dag::ValueMap& args) const override;
};

} // anonymous namespace

static ValuePtr Print(ValueMap args, dag::DAGBuilder&, SourceRange);


shared_ptr<Record> LogPlugin::Create(DAGBuilder& builder, const ValueMap& args) const
{
	SemaCheck(args.empty(), SourceRange::Over(args),
		"log plugin does not take arguments");

	auto &types = builder.typeContext();
	const Type &nilList = types.listOf(types.nilType());

	const SharedPtrVec<Parameter> params = {
		std::make_shared<Parameter>("values", nilList),
	};

	ValueMap fields = {
		{
			"print",
			builder.Function(Print, types.booleanType(), params),
		},
	};

	return builder.Record(fields);
}


static ValuePtr Print(ValueMap args, dag::DAGBuilder &b, SourceRange src)
{
	auto i = args.find("values");
	SemaCheck(i != args.end(), src, "missing 'values' argument");

	auto values = i->second;
	SemaCheck(values, src, "null 'values' argument");

	Bytestream &out = Bytestream::Stdout();

	for (const auto &v : *values->asList())
	{
		v->PrettyPrint(out);
		out << " ";
	}
	out << "\n";

	return b.Bool(true, src);
}


static plugin::Registry::Initializer init(new LogPlugin());
