/** @file Plugin/Registry.h    Declaration of @ref fabrique::plugin::Registry. */
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

#ifndef PLUGIN_REGISTRY_H
#define PLUGIN_REGISTRY_H

#include <fabrique/UniqPtr.h>
#include <fabrique/plugin/Plugin.hh>


namespace fabrique {
namespace plugin {

//! A registry for naming Fabrique @ref Plugin objects.
class Registry
{
	public:
	/**
	 * A RAII type for owning @ref Plugin::Descriptor objects
	 * and registering them in the @ref Plugin::Registry.
	 */
	class Initializer
	{
		public:
		Initializer(Plugin*);
		~Initializer();

		private:
		Registry& registry_;
		std::shared_ptr<Plugin> plugin_;
	};

	static Registry& get();

	Registry& Register(std::weak_ptr<Plugin>);
	void Deregister(std::string pluginName);

	std::weak_ptr<Plugin> lookup(std::string) const;

	private:
	Registry() {}

	StringMap<std::weak_ptr<Plugin>> plugins_;
};

} // namespace plugin
} // namespace fabrique

#endif
