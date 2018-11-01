//! @file platform/posix/files.cc    POSIX definitions of OS file abstractions
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

#include <fabrique/platform/PosixError.hh>
#include <fabrique/platform/files.hh>

#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Support/String.h"
#include "Support/exceptions.h"

#include <fstream>

#include <sys/stat.h>

#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>

using std::string;
using std::vector;


namespace {

bool FileExists(const string& filename, bool directory = false)
{
	struct stat s;
	if (stat(filename.c_str(), &s) == 0)
		return directory ? S_ISDIR(s.st_mode) : S_ISREG(s.st_mode);

	if (errno == ENOENT)
		return false;

	throw fabrique::platform::PosixError("error examining " + filename);
}


static const char PathDelimiter = ':';

}


namespace fabrique {
namespace platform {


bool PathIsAbsolute(const string& path)
{
	return (not path.empty() and path[0] == '/');
}


string AbsoluteDirectory(string name, bool createIfMissing)
{
	const char *cname = name.c_str();

	struct stat s;
	if (stat(cname, &s) != 0)
	{
		if (errno == ENOENT and createIfMissing)
		{
			if (mkdir(cname, 0777) != 0)
				throw PosixError("creating directory '" + name + "'");
		}
		else
			throw PosixError("reading directory '" + name + "'");
	}

	return AbsolutePath(cname);
}


string AbsolutePath(string name)
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


string BaseName(string path)
{
	const string filename = FilenameComponent(path);
	return filename.substr(0, filename.rfind('.'));
}


string CreateDirCommand(string dir)
{
	return "if [ ! -e \"" + dir + "\" ]; then mkdir -p \"" + dir + "\"; fi";
}


MissingFileReporter
DefaultFilename(std::string name)
{
	return [name](string, const vector<string>&) { return name; };
}


string DirectoryOf(string filename, bool absolute)
{
	// Allocate space for an extra byte in case `filename` isn't null-terminated
	const size_t NameLength = filename.length() + 1;
	char nameCopy[NameLength];
	strlcpy(nameCopy, filename.c_str(), NameLength);
	const char *dir = dirname(nameCopy);

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


string FileExtension(string path)
{
	const string filename = FilenameComponent(path);

	size_t i = filename.rfind('.');
	if (i == string::npos)
		return "";

	else
		return filename.substr(i + 1);
}


bool FileIsExecutable(string path)
{
	struct stat s;
	if (stat(path.c_str(), &s) != 0)
		throw PosixError("error querying '" + path + "'");

	if (not S_ISREG(s.st_mode))
		return false;

	return (s.st_mode & S_IXUSR);
}


bool FileIsSharedLibrary(string path)
{
	//
	// For now, just check that a file exists and is executable.
	// We can refine this logic later.
	//
	struct stat s;
	if (stat(path.c_str(), &s) != 0)
		throw PosixError("error querying '" + path + "'");

	if (not S_ISREG(s.st_mode))
		return false;

	return (s.st_mode & S_IXUSR);
}


string FilenameComponent(string pathIncludingDirectory)
{
	if (pathIncludingDirectory.empty())
		return "";

	const char *cname = pathIncludingDirectory.c_str();
	return basename(const_cast<char*>(cname));
}


string FileNotFound(string name, const vector<string>& searchPaths)
{
	std::ostringstream oss;
	oss << "no file '" << name << "' in directories [";

	for (const string& directory : searchPaths)
		oss << " '" << directory << "'";

	oss << " ]";

	throw UserError(oss.str());
}


string FindExecutable(string name, vector<string> paths, MissingFileReporter report)
{
	const char *path = getenv("PATH");
	if (not path)
		throw PosixError("error in getenv('PATH')");

	vector<string> systemPath = Split(path, string(1, PathDelimiter));
	paths.insert(paths.end(), systemPath.begin(), systemPath.end());

	return FindFile(name, paths, FileIsExecutable, report);
}


string FindFile(string filename, const vector<string>& directories,
                std::function<bool (const string&)> test,
                MissingFileReporter fileNotFound)
{
	for (const string& directory : directories)
	{
		const string absolute = JoinPath(directory, filename);
		if (PathIsFile(absolute) and test(absolute))
			return absolute;
	}

	return fileNotFound(filename, directories);
}


string FindModule(string srcroot, string subdir, string name)
{
	const string relativeName = JoinPath(subdir, name);

	//
	// Have we been passed an absolute module path?
	//
	if (PathIsAbsolute(relativeName) and FileExists(relativeName))
		return relativeName;

	//
	// If we can find the module relative to the srcroot, we don't want to
	// return an absolute path: it will go into 'subdir' and try to generate
	// files by absolute name. That is not allowed: files must be generated
	// relative to the buildroot.
	//
	if (FileExists(JoinPath(srcroot, relativeName)))
		return relativeName;

	//
	// Look for the file within platform-specific search paths.
	//
	const vector<string> searchPaths = {
		"/usr/local/share/fabrique",
	};

	const string found = FindFile(relativeName, searchPaths,
	                              PathIsFile, DefaultFilename(""));
	if (not found.empty())
		return found;

	//
	// If we were passed a directory, look for 'fabfile' within it.
	//
	const string dirname = JoinPath(srcroot, relativeName);
	if (FileExists(dirname, true))
	{
		const string fabfile = JoinPath(dirname, "fabfile");
		if (FileExists(fabfile))
			return JoinPath(relativeName, "fabfile");
	}

	throw UserError("unable to find module '" + name + "'");
}


string JoinPath(const string& x, const string& y)
{
	if (x.empty() or x == ".")
		return y;

	if (y.empty() or y == ".")
		return x;

	return x + "/" + y;
}

string JoinPath(const vector<string>& components)
{
	return join(components, "/");
}


string LibraryFilename(string name)
{
	static constexpr char Extension[] =
#if defined(OS_DARWIN)
		"dylib"
#else
		"so"
#endif
		;

	return "lib" + name + "." + Extension;
}


vector<string> PluginSearchPaths(string binary)
{
	const string prefix = DirectoryOf(DirectoryOf(binary));
	return {
		prefix + "/lib/fabrique",
		"/usr/lib/fabrique",
		"/usr/local/lib/fabrique",
	};
}


bool PathIsDirectory(string path)
{
	return FileExists(path, true);
}

bool PathIsFile(string path)
{
	return FileExists(path, false);
}

} // namespace platform
} // namespace fabrique
