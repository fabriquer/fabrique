/** @file Types/SequenceType.h    Declaration of @ref fabrique::SequenceType. */
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

#ifndef SEQUENCE_TYPE_H
#define SEQUENCE_TYPE_H

#include "Types/Type.h"

namespace fabrique {


/**
 * A type that represents an ordered sequence.
 */
class SequenceType : public Type
{
public:
	virtual ~SequenceType() override;
	const Type& elementType() const { return elementType_; }

	virtual bool isSubtype(const Type&) const override;

	virtual bool hasFiles() const override;
	virtual bool hasOutput() const override;
	virtual bool isOrdered() const override { return true; }

	virtual const Type& onAddTo(const Type&) const override;
	virtual const Type& onPrefixWith(const Type&) const override;

protected:
	SequenceType(const Type& elementTy);

private:
	const Type& elementType_;
	friend class TypeContext;
	friend class RawSequenceType;
};


/**
 * An unparameterised sequence (e.g., `list`):
 * used to generate parameterised sequences (e.g., `list[foo]`).
 */
class RawSequenceType : public Type
{
public:
	virtual Type* Parameterise(
		const PtrVec<Type>&, const SourceRange&) const override;

protected:
	RawSequenceType(TypeContext&);
	friend class TypeContext;
};

} // namespace fabrique

#endif
