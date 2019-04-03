//! @file  builtins.hh    Declaration of builtin functions
/*
 * Copyright (c) 2018-2019 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed at Memorial University of Newfoundland
 * under the NSERC Discovery program (RGPIN-2015-06048).
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

#ifndef FAB_BUILTINS_H_
#define FAB_BUILTINS_H_

#include <fabrique/dag/DAGBuilder.hh>

#include <string>

namespace fabrique {

namespace ast {
class EvalContext;
}

namespace parsing {
class Parser;
}

namespace plugin {
class Loader;
}

namespace builtins {

/**
 * Create implementation of Fabrique `file()` function
 */
dag::ValuePtr OpenFile(dag::DAGBuilder&);

/**
 * Get `fields()` builtin function.
 */
dag::ValuePtr Fields(dag::DAGBuilder&);

/**
 * Create `import()` builtin function.
 *
 * @param     parser       parser to use when importing Fabrique files
 * @param     loader       object that can load plugins that have not been loaded yet
 *                         (lifetime must exceed the value returned by this function)
 * @param     srcroot      root directory containing all source files (absolute path)
 */
dag::ValuePtr Import(parsing::Parser &parser, plugin::Loader &loader, std::string srcroot,
                     ast::EvalContext&);

/**
 * Create implementation of Fabrique `print()` function
 */
dag::ValuePtr Print(dag::DAGBuilder&);


/**
 * Create implementation of Fabrique `type()` function
 */
dag::ValuePtr Type(dag::DAGBuilder&);


} // namespace builtins
} // namespace fabrique

#endif  // FAB_BUILTINS_H_
