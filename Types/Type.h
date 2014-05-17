/** @file Types/Type.h    Declaration of @ref fabrique::Type. */
/*
 * Copyright (c) 2013 Jonathan Anderson
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

#ifndef TYPE_H
#define TYPE_H

#include <functional>

#include "ADT/PtrVec.h"
#include "Support/Printable.h"
#include "Support/Uncopyable.h"

#include <string>

namespace fabrique {

class FabContext;
class SourceRange;


/**
 * The name of a value, function, parameter or argument.
 */
class Type : public Printable, public Uncopyable
{
public:
	static const Type& GetSupertype(const Type&, const Type&);
	static const Type& ListOf(const Type&, const SourceRange&);

	static std::string UntypedPart(std::string typedName);

	Type(std::weak_ptr<Type> parent) = delete;
	virtual ~Type() {}

	FabContext& context() const { return parent_; }

	virtual const std::string name() const;
	virtual void PrettyPrint(Bytestream&, size_t indent = 0) const override;

	bool operator == (const Type&) const;
	bool operator != (const Type& t) const { return !(*this == t); }

	const PtrVec<Type>& typeParameters() const { return parameters_; }
	size_t typeParamCount() const { return parameters_.size(); }
	const Type& operator [] (size_t i) const;

	typedef std::function<PtrVec<Type> (const PtrVec<Type>&)> TypesMapper;
	const Type& Map(TypesMapper, const SourceRange&) const;

	virtual bool isSubtype(const Type&) const;
	virtual bool isSupertype(const Type&) const;

	operator bool() const;

	virtual bool valid() const { return true; }
	virtual bool isFile() const { return false; }
	virtual bool isFunction() const { return false; }
	virtual bool isString() const { return false; }
	virtual bool isOrdered() const { return false; }
	virtual bool canBeNegated() const { return false; }

	virtual const Type& onAddTo(const Type&) const;
	virtual const Type& onPrefixWith(const Type&) const;


protected:
	Type(const std::string&, const PtrVec<Type>& params, FabContext&);
	virtual Type* Parameterise(const PtrVec<Type>&, const SourceRange&) const;

private:
	static Type* Create(const std::string&, const PtrVec<Type>& params,
	                    FabContext&);

	FabContext& parent_;
	const std::string typeName_;
	const PtrVec<Type> parameters_;

	friend class FabContext;
};

} // namespace fabrique

#endif
