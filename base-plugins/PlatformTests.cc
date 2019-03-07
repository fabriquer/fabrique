/**
 * @file base-plugins/PlatformTests.cc
 * Definition of @ref fabrique::plugins::PlatformTests.
 */
/*
 * Copyright (c) 2014, 2018-2019 Jonathan Anderson
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

/**
 * Platform detection: defines the bare minimum of constants required to
 * implement platform-specific functionality.
 *
 */
class PlatformTests : public plugin::Plugin
{
	public:
	virtual string name() const override { return "platform"; }
	virtual shared_ptr<dag::Record>
		Create(dag::DAGBuilder&, const ValueMap& args) const override;
};

struct Platform
{
	string name;

	// Non-mutually-exclusive flags:
	bool bsd;
	bool darwin;
	bool linux;
	bool posix;
	bool windows;

	static Platform get();
};

} // anonymous namespace

static string Architecture();


shared_ptr<Record>
PlatformTests::Create(DAGBuilder& builder, const ValueMap& args) const
{
	SourceRange src = SourceRange::Over(args);
	SemaCheck(args.empty(), src, "platform plugin does not take arguments");

	ValueMap fields;
	fields["architecture"] = builder.String(Architecture(), src);

	Platform p = Platform::get();
	fields["osname"] = builder.String(p.name);

	fields["bsd"] = builder.Bool(p.bsd, src);
	fields["darwin"] = builder.Bool(p.darwin, src);
	fields["posix"] = builder.Bool(p.posix, src);
	fields["windows"] = builder.Bool(p.windows, src);

	return builder.Record(fields);
}

static plugin::Registry::Initializer init(new PlatformTests());


static string Architecture()
{
#if defined(__amd64__) || defined(__x86_64__) || defined(_M_AMD64)
	return "amd64";

#elif defined(__arm__) || defined(_M_ARM)
	return "arm";

#elif defined(__aarch64__)
	return "arm64";

#elif defined(__i386__) || defined(_M_IX86)
	return "i386";

#elif defined(__ia64__) || defined(_M_IA64)
	return "ia64";

#elif defined(__m68k__)
	return "m68k";

#elif defined(__mips__)
	return "mips";

#elif defined(__powerpc__) || defined(_M_PPC)
	return "powerpc";

#elif defined(__sparc__) || defined(__sparc)
	return "sparc";

#else
	#error Unknown architecture
#endif
}


Platform Platform::get()
{
	Platform p;
	p.name =
#if defined(__APPLE__)
		"macos"

#elif defined(__FreeBSD__)
		"freebsd"

#elif defined(__NetBSD__)
		"netbsd"

#elif defined(__OpenBSD__)
		"openbsd"

#elif defined(__linux)
		"linux"

#elif defined(_WIN32) || defined(_WIN64)
		"windows"

#else
	#error Unsupported platform
#endif
		;

#if defined(__Apple__)
	p.darwin = true;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	p.bsd = true;
#elif defined(__linux)
	p.linux = true;
#elif defined(_WIN32) || defined(_WIN64)
	p.windows = true;
#endif

	p.posix = p.bsd or p.darwin or p.linux;

	return p;
}
