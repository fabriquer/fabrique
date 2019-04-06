//! @file ast/ast.hh    Meta-include file for all AST node types
/*
 * Copyright (c) 2013, 2018 Jonathan Anderson
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

#ifndef AST_H
#define AST_H

namespace fabrique
{

//! Representation of the Abstract Syntax Tree for Fabrique source code.
namespace ast {}

}

#include <fabrique/ast/Action.hh>
#include <fabrique/ast/Argument.hh>
#include <fabrique/ast/Arguments.hh>
#include <fabrique/ast/BinaryOperation.hh>
#include <fabrique/ast/Call.hh>
#include <fabrique/ast/CompoundExpr.hh>
#include <fabrique/ast/Conditional.hh>
#include <fabrique/ast/FieldAccess.hh>
#include <fabrique/ast/FieldQuery.hh>
#include <fabrique/ast/FilenameLiteral.hh>
#include <fabrique/ast/FileList.hh>
#include <fabrique/ast/Foreach.hh>
#include <fabrique/ast/Function.hh>
#include <fabrique/ast/Identifier.hh>
#include <fabrique/ast/List.hh>
#include <fabrique/ast/NameReference.hh>
#include <fabrique/ast/Parameter.hh>
#include <fabrique/ast/Record.hh>
#include <fabrique/ast/TypeDeclaration.hh>
#include <fabrique/ast/UnaryOperation.hh>
#include <fabrique/ast/Value.hh>

#include <fabrique/ast/literals.hh>

#endif
