/** @file plugins/SysctlPlugin.cc   Definition of @ref fabrique::plugins::SysctlPlugin. */
/*
 * Copyright (c) 2014, 2018 Jonathan Anderson
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
#include <sys/sysctl.h>

#include <fabrique/dag/DAGBuilder.hh>
#include <fabrique/dag/Parameter.hh>
#include <fabrique/plugin/Registry.hh>
#include <fabrique/types/FunctionType.hh>
#include <fabrique/types/RecordType.hh>
#include <fabrique/types/TypeContext.hh>
#include "Support/PosixError.h"
#include "Support/exceptions.h"

#include <cassert>

#include <errno.h>

using namespace fabrique;
using namespace fabrique::dag;
using fabrique::plugin::Plugin;
using std::shared_ptr;
using std::string;


namespace fabrique {
namespace plugins {

/**
 * Provides access to the sysctl(3) set of C library functions.
 *
 * Many useful properties of the system are represented (or controlled) with sysctl(3)
 * entries. For instance, Fabrique build descriptions might like to inspect the values
 * of kern.ostype, kern.osrelease, etc.
 */
class SysctlPlugin : public plugin::Plugin
{
	public:
	virtual shared_ptr<dag::Record>
		Create(dag::DAGBuilder&, const dag::ValueMap& args) const override;

	class Factory : public Plugin::Descriptor
	{
		public:
		virtual string name() const override { return "sysctl"; }
		virtual UniqPtr<Plugin> Instantiate(TypeContext&) const override;

	};

	private:
	SysctlPlugin(const Factory& factory, const RecordType& type,
	             const Type& stringType, const Type& intType)
		: Plugin(type, factory), intType_(intType), stringType_(stringType)
	{
	}

	const Type& intType_;
	const Type& stringType_;
};


static string SysctlName(ValueMap args);
static ValuePtr StringSysctl(ValueMap args, DAGBuilder& builder, SourceRange);
static ValuePtr IntegerSysctl(ValueMap args, DAGBuilder& builder, SourceRange);


UniqPtr<Plugin> SysctlPlugin::Factory::Instantiate(TypeContext& ctx) const
{
	const Type& stringType = ctx.stringType();
	const Type& intType = ctx.integerType();

	const FunctionType& string = ctx.functionType(stringType, stringType);
	const FunctionType& integer = ctx.functionType(stringType, intType);

	const RecordType& type = ctx.recordType({
		{ "string", string },
		{ "int", integer },
	});

	return UniqPtr<Plugin>(
		new SysctlPlugin(*this, type, stringType, integer));
}


shared_ptr<Record> SysctlPlugin::Create(DAGBuilder& builder, const ValueMap& args) const
{
	SemaCheck(args.empty(), SourceRange::Over(args),
		"sysctl plugin does not take arguments");

	const SharedPtrVec<Parameter> params = {
		std::make_shared<Parameter>("name", stringType_, ValuePtr()),
	};

	ValueMap fields = {
		{
			"string",
			builder.Function(StringSysctl, stringType_, params)
		},
		{
			"int",
			builder.Function(IntegerSysctl, intType_, params)
		},
	};

	return builder.Record(fields);
}


static string SysctlName(ValueMap args)
{
	// If the user didn't pass a 'name' argument of type string,
	// dag::Callable ought to have caught it.
	assert(args.size() == 1);
	assert(args.find("name") != args.end());

	ValuePtr name = args.find("name")->second;
	assert(name);

	return name->str();
}


static ValuePtr StringSysctl(ValueMap args, DAGBuilder& builder, SourceRange src)
{
#if !defined (OS_POSIX)
	throw UserError(name() + " functions are only useful on POSIX platforms");
#endif

	const string name = SysctlName(args);
	const char *rawName = name.c_str();

	size_t len = 0;
	if (sysctlbyname(rawName, NULL, &len, NULL, 0))
		throw PosixError(
			"error querying size of '" + name + "' sysctl");

	UniqPtr<char> buffer(new char[len]);
	if (sysctlbyname(rawName, buffer.get(), &len, NULL, 0))
		throw PosixError(
			"error retrieving '" + name + "' via sysctlbyname()");

	return builder.String(buffer.get(), src);
}


static ValuePtr IntegerSysctl(ValueMap args, DAGBuilder& builder, SourceRange src)
{
#if !defined (OS_POSIX)
	throw UserError(name() + " functions are only useful on POSIX platforms");
#endif

	const string name = SysctlName(args);
	const char *rawName = name.c_str();

	int value;
	size_t len = sizeof(value);
	if (sysctlbyname(rawName, &value, &len, NULL, 0))
		throw PosixError(
			"error retrieving '" + name + "' via sysctlbyname()");

	return builder.Integer(value, src);
}


static plugin::Registry::Initializer init(new SysctlPlugin::Factory());

} // namespace plugins
} // namespace fabrique
