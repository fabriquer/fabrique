/** @file plugins/SysctlPlugin.cc   Definition of @ref fabrique::plugins::SysctlPlugin. */
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

#include "DAG/DAGBuilder.h"
#include "DAG/Parameter.h"
#include "Plugin/Registry.h"
#include "Types/FunctionType.h"
#include "Types/StructureType.h"
#include "Types/TypeContext.h"
#include "Support/PosixError.h"
#include "Support/exceptions.h"

#include <cassert>

#include <sys/sysctl.h>
#include <errno.h>

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
class SysctlPlugin : public plugin::Plugin
{
	public:
	virtual shared_ptr<dag::Structure> Create(dag::DAGBuilder&) const override;

	class Factory : public Plugin::Descriptor
	{
		public:
		virtual string name() const override { return "sysctl"; }
		virtual UniqPtr<Plugin> Instantiate(TypeContext&) const override;

	};

	private:
	SysctlPlugin(const Factory& factory, const StructureType& type,
	             const Type& stringType,
	             const FunctionType& stringSysctlType)
		: Plugin(type, factory), stringType_(stringType),
		  stringSysctlType_(stringSysctlType)
	{
	}

	const Type& stringType_;

	const FunctionType& stringSysctlType_;
};

} // anonymous namespace


UniqPtr<Plugin> SysctlPlugin::Factory::Instantiate(TypeContext& ctx) const
{
	const Type& stringType = ctx.stringType();
	//const Type& intType = ctx.integerType();

	const FunctionType& string = ctx.functionType(stringType, stringType);

	const StructureType& type = ctx.structureType({
		{ "string", string }
	});

	return UniqPtr<Plugin>(new SysctlPlugin(*this, type, stringType, string));
}


static ValuePtr StringSysctl(const ValueMap& /*scope*/, const ValueMap& args,
                             DAGBuilder& builder, SourceRange src)
{
	// If the user didn't pass a 'name' argument of type string,
	// dag::Callable ought to have caught it.
	assert(args.size() == 1);
	assert(args.find("name") != args.end());

	ValuePtr name = args.find("name")->second;
	assert(name->type().isSubtype(builder.typeContext().stringType()));
	const char *rawName = name->str().c_str();

	size_t len;
	if (sysctlbyname(rawName, NULL, &len, NULL, 0))
		throw PosixError(
			"error querying size of '" + name->str() + "' sysctl");

	UniqPtr<char> buffer(new char[len]);
	if (sysctlbyname(rawName, buffer.get(), &len, NULL, 0))
		throw PosixError(
			"error retrieving '" + name->str() + "' via sysctlbyname()");

	return builder.String(buffer.get(), src);
}


shared_ptr<Structure> SysctlPlugin::Create(DAGBuilder& builder) const
{
	std::vector<Structure::NamedValue> fields;

	SharedPtrVec<Parameter> params;
	params.emplace_back(new Parameter("name", stringType_, ValuePtr()));

	fields.emplace_back("string",
		std::shared_ptr<Function>(Function::Create(StringSysctl, ValueMap(), params, stringSysctlType_)));

	auto result = std::dynamic_pointer_cast<Structure>(
		builder.Struct(fields, type(), SourceRange::None()));

	assert(result);
	return result;
}


static auto& registry = plugin::Registry::Default().Register(*new SysctlPlugin::Factory());
