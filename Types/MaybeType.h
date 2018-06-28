/** @file Types/MaybeType.h    Declaration of @ref fabrique::MaybeType. */
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

#ifndef MAYBE_TYPE_H
#define MAYBE_TYPE_H

#include "Types/Type.h"

namespace fabrique {


/**
 * A type that represents an ordered sequence.
 */
class MaybeType : public Type
{
public:
	virtual ~MaybeType() override;
	const Type& elementType() const { return elementType_; }

	virtual TypeMap fields() const override;

	virtual bool hasFields() const override { return true; }
	virtual bool isOptional() const override { return true; }
	virtual bool isSubtype(const Type&) const override;

protected:
	MaybeType(const Type& elementTy);

private:
	const Type& elementType_;
	friend class TypeContext;
	friend class RawMaybeType;
};


/**
 * An unparameterised sequence (e.g., `maybe`):
 * used to generate parameterised sequences (e.g., `maybe[foo]`).
 */
class RawMaybeType : public Type
{
public:
	virtual Type* Parameterise(
		const PtrVec<Type>&, const SourceRange&) const override;

protected:
	RawMaybeType(TypeContext&);
	friend class TypeContext;
};

} // namespace fabrique

#endif
