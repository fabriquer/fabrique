/**
 * @file plugins/PlatformTests.cc
 * Definition of @ref fabrique::plugins::PlatformTests.
 */
/*
 * Copyright (c) 2014, 2018 Jonathan Anderson
 * All rights reserved.
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

#include <fabrique/dag/DAGBuilder.hh>
#include <fabrique/plugin/Registry.hh>
#include "Support/exceptions.h"

#if defined(__APPLE__)
#include "TargetConditionals.h"
#endif

using namespace fabrique;
using namespace fabrique::dag;
using fabrique::plugin::Plugin;
using std::shared_ptr;
using std::string;


namespace {

// Apple:
static const char iOS[]		= "ios";
static const char MacOSX[]	= "macosx";

// BSD:
static const char FreeBSD[]	= "freebsd";
static const char NetBSD[]	= "netbsd";
static const char OpenBSD[]	= "openbsd";

// Linux (I hope we don't need distro differentiation):
static const char Linux[]	= "linux";

// Windows:
static const char Win32[]	= "win32";
static const char Win64[]	= "win64";

static const char* Platforms[] = {
	// Apple:
	iOS,
	MacOSX,

	// BSD:
	FreeBSD,
	NetBSD,
	OpenBSD,

	// Linux:
	Linux,

	// Windows:
	Win32,
	Win64,
};


/**
 * The name of the platform that Fabrique was compiled for.
 *
 * This doesn't provide any logic such as "BSD, Linux and Mac are all POSIX":
 * that is done in Fabrique files rather than compiled C++ files.
 */
static const char *Name =
// Apple currently means Mac:
#if defined(__APPLE__)
	MacOSX

// BSD:
#elif defined(__FreeBSD__)
	FreeBSD

#elif defined(__NetBSD__)
	NetBSD

#elif defined(__OpenBSD__)
	OpenBSD

// Linux:
#elif defined(__linux)
	Linux

// Windows:
#elif defined(_WIN32)
	#if defined(_WIN64)
		Win64
	#else
		Win32
	#endif

#else
	#error Unsupported platform
#endif
	;

/**
 * Platform detection: defines the bare minimum of constants required to
 * implement platform-specific functionality.
 *
 */
class PlatformTests : public plugin::Plugin
{
	public:
	virtual string name() const override { return "platform-tests"; }
	virtual shared_ptr<dag::Record>
		Create(dag::DAGBuilder&, const ValueMap& args) const override;
};

} // anonymous namespace


shared_ptr<Record>
PlatformTests::Create(DAGBuilder& builder, const ValueMap& args) const
{
	SourceRange src = SourceRange::Over(args);
	SemaCheck(args.empty(), src, "platform plugin does not take arguments");

	const ValueMap scope;
	ValueMap fields;

	for (const char *platform : Platforms)
	{
		const bool isThisPlatform = (Name == platform);
		fields[platform] = builder.Bool(isThisPlatform, src);
	}

	return builder.Record(fields);
}


static plugin::Registry::Initializer init(new PlatformTests());
