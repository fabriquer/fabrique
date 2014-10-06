/** @file Support/os-posix.cc    POSIX definitions of OS abstractions. */
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

#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Support/exceptions.h"
#include "Support/os.h"

#include <fstream>

#include <sys/stat.h>

#include <libgen.h>
#include <stdlib.h>

using std::string;


namespace {

//! An OS error that has an errno or equivalent output.
class PosixError : public fabrique::OSError
{
public:
	explicit PosixError(string message)
		: OSError(message, strerror(errno))
	{
	}

	PosixError(PosixError&& e)
		: OSError(e)
	{
	}

	virtual ~PosixError() {}

private:
	PosixError(const PosixError&) = delete;
};


bool FileExists(const string& filename, bool directory = false)
{
	struct stat s;
	if (stat(filename.c_str(), &s) == 0)
		return directory ? S_ISDIR(s.st_mode) : S_ISREG(s.st_mode);

	if (errno == ENOENT)
		return false;

	throw PosixError("error examining " + filename);
}


#if defined(__DARWIN_UNIX03)
/**
 * A Darwin-specific wrapper around dirname(3) that accepts a const char*
 * argument (rather than the plain char* accepted by dirname(3) under
 * __DARWIN_UNIX03).
 */
static const char* dirname(const char *filename)
{
	return ::dirname(const_cast<char*>(filename));
}
#endif

}


bool fabrique::PathIsAbsolute(const string& path)
{
	return (not path.empty() and path[0] == '/');
}


string fabrique::AbsoluteDirectory(string name, bool createIfMissing)
{
	const char *cname = name.c_str();

	struct stat s;
	if (stat(cname, &s) != 0)
	{
		if (errno == ENOENT and createIfMissing)
		{
			if (mkdir(cname, 0777) != 0)
				throw PosixError("creating directory " + name);
		}
		else
			throw PosixError("reading directory " + name);
	}

	return AbsolutePath(cname);
}


string fabrique::AbsolutePath(string name)
{
	char *absolutePath = realpath(name.c_str(), nullptr);
	if (not absolutePath)
		throw PosixError("error in realpath('" + name + "')");

	string path(absolutePath);
	free(absolutePath);

	if (path == ".")
		return "";

	return path;
}


string fabrique::BaseName(string path)
{
	const string filename = FilenameComponent(path);
	return filename.substr(0, filename.rfind('.'));
}


string fabrique::CreateDirCommand(string dir)
{
	return "if [ ! -e \"" + dir + "\" ]; then mkdir -p \"" + dir + "\"; fi";
}


string fabrique::DirectoryOf(string filename, bool absolute)
{
	const char *dir = dirname(filename.c_str());

	if (not dir)
		throw PosixError("error looking for parent of " + filename);

	const string relative(dir);
	if (not absolute)
		return (relative == ".") ? "" : relative;

	const string absoluteDir(AbsoluteDirectory(dir));

	struct stat s;
	if (stat(absoluteDir.c_str(), &s) != 0)
		throw PosixError("error querying " + absoluteDir);

	if (not S_ISDIR(s.st_mode))
		throw PosixError(filename + " is not a directory");

	return absoluteDir;
}


string fabrique::FileExtension(string path)
{
	const string filename = FilenameComponent(path);

	size_t i = filename.rfind('.');
	if (i == string::npos)
		return "";

	else
		return filename.substr(i + 1);
}


string fabrique::FilenameComponent(string pathIncludingDirectory)
{
	if (pathIncludingDirectory.empty())
		return "";

	const char *cname = pathIncludingDirectory.c_str();
	return basename(const_cast<char*>(cname));
}


std::string fabrique::FindModule(string srcroot, string subdir, string name)
{
	const string relativeName = JoinPath(subdir, name);
	if (PathIsAbsolute(relativeName) and FileExists(relativeName))
		return relativeName;

	if (FileExists(JoinPath(srcroot, relativeName)))
		return relativeName;

	const string searchPaths[] = {
		"/usr/local/share/fabrique",
	};

	for (const string& path : searchPaths)
	{
		const string fullPath = JoinPath(path, relativeName);
		if (FileExists(fullPath))
			return fullPath;
	}

	const string dirname = JoinPath(srcroot, relativeName);
	if (FileExists(dirname, true))
	{
		const string fabfile = JoinPath(dirname, "fabfile");
		if (FileExists(fabfile))
			return JoinPath(relativeName, "fabfile");
	}

	throw UserError("unable to find module '" + name + "'");
}


string fabrique::JoinPath(const string& x, const string& y)
{
	if (x.empty() or x == ".")
		return y;

	if (y.empty() or y == ".")
		return x;

	return x + "/" + y;
}

string fabrique::JoinPath(const std::vector<string>& components)
{
	return join(components, "/");
}


bool fabrique::PathIsDirectory(string path)
{
	return FileExists(path, true);
}

bool fabrique::PathIsFile(string path)
{
	return FileExists(path, false);
}
