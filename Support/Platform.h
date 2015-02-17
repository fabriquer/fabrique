/** @file Support/Platform.h      Platform detection. */
/*
 * Copyright (c) 2015 Jonathan Anderson
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

#ifndef PLATFORM_H
#define PLATFORM_H

#if defined(__APPLE__)
#include "TargetConditionals.h"
#endif

namespace fabrique {
namespace platform {

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


/**
 * The name of the platform that Fabrique was compiled for.
 *
 * This doesn't provide any logic such as "BSD, Linux and Mac are all POSIX":
 * that is done in Fabrique files rather than compiled C++ files.
 */
static const char *Name =
// Apple:
#if defined(__APPLE__)
	#if defined(TARGET_OS_IPHONE)
		iOS
	#elif defined(TARGET_OS_MAC)
		MacOS
	#else
		#error Unsupported Apple platform
	#endif

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


} // namespace platform
} // namespace fabrique

#endif
