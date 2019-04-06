/** @file driver.cc    Driver for the fabrique compiler. */
/*
 * Copyright (c) 2013-2014, 2018 Jonathan Anderson
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

#include "CLIArguments.h"

#include <fabrique/builtins.hh>
#include <fabrique/Bytestream.hh>
#include <fabrique/FabBuilder.hh>
#include <fabrique/UserError.hh>

#include <fabrique/ast/ASTDump.hh>
#include <fabrique/ast/EvalContext.hh>

#include <fabrique/backend/Backend.hh>

#include <fabrique/dag/DAG.hh>

#include <fabrique/parsing/Parser.hh>

#include <fabrique/platform/OSError.hh>
#include <fabrique/platform/files.hh>

#include <fabrique/plugin/Loader.hh>

#include <fabrique/types/TypeContext.hh>

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>

using namespace fabrique;
using namespace fabrique::platform;
using namespace std;
using fabrique::backend::Backend;


int main(int argc, char *argv[]) {
	//
	// Parse command-line arguments.
	//
	CLIArguments args = CLIArguments::Parse(argc, argv);
	if (not args or args.help)
	{
		CLIArguments::PrintUsage(cerr);
		return (args ? 0 : 1);
	}

	//
	// Set up debug streams.
	//
	Bytestream::SetDebugPattern(args.debugPattern);
	Bytestream::SetDebugStream(Bytestream::Stdout());

	Bytestream& argDebug = Bytestream::Debug("cli.args");
	args.Print(argDebug);
	argDebug << Bytestream::Reset << "\n";

	Bytestream& err = Bytestream::Stderr();

	try
	{
		//
		// Translate command-line arguments into values for the
		// Fabrique instance using a FabBuilder:
		//
		Fabrique fab = FabBuilder()
			.parseOnly(args.parseOnly)
			.printASTs(args.printAST)
			.printDAG(args.printDAG)
			.dumpASTs(args.dumpAST)
			.backends(args.outputFormats)
			.outputDirectory(args.output)
			.pluginPaths(PluginSearchPaths(args.executable))
			.printToStdout(args.printOutput)
			.regenerationCommand(args.executable + args.str())
			.build()
			;

		fab.AddArguments(args.definitions);
		fab.Process(args.input);

		return 0;
	}
	catch (const UserError& e)
	{
		err
			<< Bytestream::Error << "Error"
			<< Bytestream::Reset << ": " << e
			;
	}
	catch (const OSError& e)
	{
		err
			<< Bytestream::Error << e.message()
			<< Bytestream::Reset << ": "
			<< Bytestream::ErrorMessage << e.description()
			;
	}
	catch (const SourceCodeException& e)
	{
		err << e;
	}
	catch (const std::exception& e)
	{
		err
			<< Bytestream::Error << "Uncaught exception"
			<< Bytestream::Reset << ": "
			<< Bytestream::ErrorMessage << e.what()
			;
	}

	err << Bytestream::Reset << "\n";
	return 1;
}
