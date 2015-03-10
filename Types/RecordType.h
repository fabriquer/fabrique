/** @file Types/RecordType.h    Declaration of @ref fabrique::RecordType. */
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

#ifndef STRUCTURE_TYPE_H
#define STRUCTURE_TYPE_H

#include "ADT/StringMap.h"
#include "Types/Type.h"
#include <vector>

namespace fabrique {

/**
 * The type of a record, which contains named, typed, immutable fields.
 */
class RecordType : public Type
{
public:
	static RecordType* Create(const NamedTypeVec&, TypeContext&);

	virtual ~RecordType();
	TypeMap fields() const override { return fieldTypes_; }

	virtual bool hasFields() const override { return true; }
	virtual bool isSubtype(const Type&) const override;
	virtual const Type& supertype(const Type&) const override;

	virtual void PrettyPrint(Bytestream&, size_t indent) const override;

private:
	RecordType(const StringMap<const Type&>& fields,
	           const std::vector<std::string>& fieldNames, TypeContext&);

	//! The types of fields within the record.
	StringMap<const Type&> fieldTypes_;

	/**
	 * Ordered sequence of field names.
	 *
	 * This isn't semantically relevant, but it's nice to output field names
	 * in the same order as their definition.
	 */
	std::vector<std::string> fieldNames_;

	friend class TypeContext;
};

} // namespace fabrique

#endif
