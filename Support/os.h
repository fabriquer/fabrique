/** @file Support/os.h      Declarations of OS-abstraction functions. */
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

#ifndef POSIX_H
#define POSIX_H

#include <string>
#include <vector>

namespace fabrique {

//
// File- and path-related predicates:
//

//! Check whether a file is executable on this platform.
bool FileIsExecutable(std::string path);

//! The named path is absolute, whether or not the file actually exists.
bool PathIsAbsolute(const std::string&);

//! Does the named path exist, and is it a directory?
bool PathIsDirectory(std::string);

//! Does the named path exist, and is it a regular file?
bool PathIsFile(std::string);


//
// Filename and path manipulation:
//

//! Find the absolute version of a directory, optionally creating it.
std::string AbsoluteDirectory(std::string name, bool createIfMissing = true);

//! Find the absolute version of a path (file or directory).
std::string AbsolutePath(std::string path);

//! Get the basename of a path: 'foo/bar.c' -> 'bar'.
std::string BaseName(std::string path);

//! The command required to create a directory (if it doesn't already exist).
std::string CreateDirCommand(std::string directory);

//! Find the directory containing a file, optionally returning an absolute path.
std::string DirectoryOf(std::string filename, bool absolute = false);

//! Get the extension of a path: 'foo/bar.c' -> 'c'.
std::string FileExtension(std::string path);

//! Find the non-directory component of a path.
std::string FilenameComponent(std::string pathIncludingDirectory);

/**
 * Find a file named @a filename within a set of @a directories.
 *
 * @param   test      A test to invoke on each file (e.g., PathIsFile, FileIsExecutable)
 *                    in order to confirm the applicability of a file.
 */
std::string FindFile(std::string filename, const std::vector<std::string>& directories,
                     std::function<bool (const std::string&)> test = PathIsFile);

//! Join two path components (a directory and a filename).
std::string JoinPath(const std::string&, const std::string&);

//! Join an arbitrary number of path components (directories and maybe a filename).
std::string JoinPath(const std::vector<std::string>&);


//
// Fabrique modules:
//

//! Find the name of a Fabrique module within the @a srcroot and platform search paths.
std::string FindModule(std::string srcroot, std::string subdir,
                       std::string filename);

}

#endif
