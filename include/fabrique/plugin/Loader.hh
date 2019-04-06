//! @file plugin/Loader.hh    Declaration of @ref fabrique::plugin::Loader
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

#ifndef FAB_PLUGIN_LOADER_H_
#define FAB_PLUGIN_LOADER_H_

#include <fabrique/plugin/Plugin.hh>

#include <string>
#include <vector>


namespace fabrique {

namespace platform {
class SharedLibrary;
}

namespace plugin {

//! Support for loading plugins from shared libraries.
class Loader
{
	public:
	Loader(const std::vector<std::string>& paths);

	/**
	 * Load a shared library with a given (library) name.
	 *
	 * @param   name     the library's name, cross-platform and excluding filename
	 *                   details (e.g., "foo" rather than "libfoo.so")
	 */
	std::weak_ptr<Plugin> Load(std::string name);

	private:
	std::vector<std::string> paths_;
	std::vector<std::shared_ptr<platform::SharedLibrary>> libraries_;
};

} // namespace plugin
} // namespace fabrique

#endif
