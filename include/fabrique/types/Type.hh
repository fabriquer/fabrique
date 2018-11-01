/** @file Types/Type.h    Declaration of @ref fabrique::Type. */
/*
 * Copyright (c) 2013-2016, 2018 Jonathan Anderson
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

#include <fabrique/Printable.hh>
#include <fabrique/PtrVec.h>
#include <fabrique/StringMap.h>
#include <fabrique/Uncopyable.hh>

#include <string>

namespace fabrique {

class SourceRange;
class TypeContext;


/**
 * The name of a value, function, parameter or argument.
 */
class Type : public Printable, public Uncopyable
{
public:
	typedef std::pair<std::string, const Type&> NamedType;
	typedef std::vector<NamedType> NamedTypeVec;

	static const Type& ListOf(const Type&, const SourceRange&);

	static std::string UntypedPart(std::string typedName);

	Type(std::weak_ptr<Type> parent) = delete;
	virtual ~Type() override {}

	TypeContext& context() const { return parent_; }

	virtual const std::string name() const;

	typedef StringMap<const Type&> TypeMap;

	/**
	 * The fields that objects of this type have.
	 *
	 * This can be empty even if @b hasFields() is true: objects like this
	 * one might not happen to have fields while still being the kind of
	 * objects that, in general, do (e.g., a record like @b args with no members).
	 */
	virtual TypeMap fields() const { return TypeMap(); }

	virtual void PrettyPrint(Bytestream&, unsigned int indent = 0) const override;

	bool operator == (const Type&) const;
	bool operator != (const Type& t) const { return !(*this == t); }

	const PtrVec<Type>& typeParameters() const { return parameters_; }
	size_t typeParamCount() const { return parameters_.size(); }
	const Type& operator [] (size_t i) const;

	typedef std::function<PtrVec<Type> (const PtrVec<Type>&)> TypesMapper;
	const Type& Map(TypesMapper, const SourceRange&) const;

	virtual bool isSubtype(const Type&) const;
	virtual bool isSupertype(const Type&) const;

	/**
	 * Check to ensure that this type is a subtype of @b t.
	 * If not, throw a @ref WrongTypeException.
	 */
	void CheckSubtype(const Type& t, SourceRange) const;

	/**
	 * Find a common supertype for @b this type and @b other.
	 *
	 * The default implementation checks to see if A is a supertype of B or
	 * vice versa. Subtypes could override this method with logic for constructing
	 * lowest-common-denominator supertypes, e.g., including the fields that
	 * are common to two record types.
	 *
	 * @returns   the common supertype, or @b nil if none is found
	 */
	virtual const Type& supertype(const Type& other) const;

	operator bool() const;

	virtual bool valid() const { return true; }
	virtual bool hasFields() const { return false; }
	virtual bool hasFiles() const { return false; }
	virtual bool hasOutput() const { return false; }
	virtual bool isFile() const { return false; }
	virtual bool isFunction() const { return false; }
	virtual bool isNumeric() const { return false; }
	virtual bool isString() const { return false; }
	virtual bool isOptional() const { return false; }
	virtual bool isOrdered() const { return false; }
	virtual bool isType() const { return false; }
	virtual bool canBeNegated() const { return false; }

	virtual const Type& onAddTo(const Type&) const;
	virtual const Type& onMultiply(const Type&) const;
	virtual const Type& onPrefixWith(const Type&) const;


protected:
	Type(const std::string&, const PtrVec<Type>& params, TypeContext&);
	virtual Type* Parameterise(const PtrVec<Type>&, const SourceRange&) const;

private:
	static Type* Create(const std::string&, const PtrVec<Type>& params,
	                    TypeContext&);

	TypeContext& parent_;
	const std::string typeName_;
	const PtrVec<Type> parameters_;

	friend class TypeContext;
};

} // namespace fabrique

#endif
