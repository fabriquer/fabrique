/** @file Plugin/Registry.cc    Definition of @ref fabrique::plugin::Registry. */
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

#include "Plugin/Plugin.h"
#include "Plugin/Registry.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::plugin;


Registry::Initializer::Initializer(Plugin::Descriptor *descriptor)
	: registry_(Registry::get()), plugin_(descriptor)
{
	assert(plugin_);
	registry_.Register(plugin_);
}


Registry::Initializer::~Initializer()
{
	registry_.Deregister(plugin_->name());
}


Registry& Registry::get()
{
	static Registry& instance = *new Registry();
	return instance;
}


Registry& Registry::Register(std::weak_ptr<Plugin::Descriptor> plugin)
{
	const std::string name = plugin.lock()->name();
	assert(plugins_.find(name) == plugins_.end());

	plugins_.emplace(name, plugin);
	return *this;
}


void Registry::Deregister(std::string name)
{
	assert(plugins_.find(name) != plugins_.end());
	plugins_.erase(name);
}


std::weak_ptr<Plugin::Descriptor> Registry::lookup(std::string name) const
{
	auto i = plugins_.find(name);
	if (i == plugins_.end())
		return std::weak_ptr<Plugin::Descriptor>();

	return i->second;
}
