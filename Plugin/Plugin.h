/** @file Plugin/Plugin.h    Declaration of @ref fabrique::plugin::Plugin. */
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

#ifndef PLUGIN_H
#define PLUGIN_H

#include "ADT/UniqPtr.h"
#include "DAG/Record.h"
#include "Types/Typed.h"


namespace fabrique {

class TypeContext;

namespace dag {
class DAGBuilder;
}

//! Code to support loading and using Fabrique plugins.
namespace plugin {

/**
 * A plugin that provides extra functionality to Fabrique build descriptions.
 *
 * Plugins are written in C++ to provide functionality that simple shell commands
 * don't express well. For instance, instead of parsing the output of sysctl(8)
 * (turning a typed value into a string and back into a value according to expected type),
 * a sysctl plugin can represent the underlying types correctly. The difference is:
 * if a Fabrique description expects the wrong type, it can receive a type error rather
 * than a syntactically-legal but logically-incorrect reinterpretation (e.g. "0").
 */
class Plugin : public Typed
{
	public:
	virtual ~Plugin();


	/**
	 * Static information about a plugin.
	 */
	class Descriptor
	{
		public:
		virtual ~Descriptor();

		virtual std::string name() const = 0;
		virtual UniqPtr<Plugin> Instantiate(TypeContext&) const = 0;
	};

	static Plugin::Descriptor& nullPlugin();

	const Descriptor& descriptor() const { return descriptor_; }

	virtual std::shared_ptr<dag::Record>
		Create(dag::DAGBuilder&, const dag::ValueMap& arguments) const = 0;


	protected:
	Plugin(const Type&, const Descriptor& descriptor);


	private:
	const Descriptor& descriptor_;
};

} // namespace plugin
} // namespace fabrique

#endif
