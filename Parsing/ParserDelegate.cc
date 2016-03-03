/** @file Parsing/ParserDelegate.cc Definition of @ref fabrique::parser::ParserDelegate. */
/*
 * Copyright (c) 2015-2016 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed at Memorial University of Newfoundland under
 * the NSERC Discovery program (RGPIN-2015-06048).
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

#include "AST/ast.h"
#include "Parsing/Grammar.h"
#include "Parsing/ParserDelegate.h"
#include "Support/ABI.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/BooleanType.h"
#include "Types/FunctionType.h"
#include "Types/IntegerType.h"
#include "Types/RecordType.h"
#include "Types/StringType.h"
#include "Types/TypeContext.h"
#include "Types/TypeError.h"

using namespace fabrique;
using namespace fabrique::parser;

using pegmatite::ASTStack;
using pegmatite::InputRange;
using pegmatite::ParserPosition;

using namespace std::placeholders;
using std::string;
using std::unique_ptr;
using std::vector;


ParserDelegate::ParserDelegate(const Grammar& g, TypeContext& t,
                               UniqPtrVec<ErrorReport>& e)
	: grammar_(g), types_(t), errors_(e)
{
	BindType<ast::Identifier>(g.Identifier);
	BindType<ast::TypeReference>(g.Type);
	BindParser<ast::TypeReference::FieldTypeParser>(g.FieldType);

	BindType<ast::BoolLiteral>(g.BoolLiteral);
	BindType<ast::IntLiteral>(g.IntLiteral);
	BindType<ast::StringLiteral>(g.StringLiteral);

	BindType<ast::CompoundExpression>(g.CompoundExpression);
	BindType<ast::Conditional>(g.Conditional);
	BindType<ast::FieldAccess>(g.FieldReference);
	BindType<ast::File>(g.File);
	BindType<ast::FileList>(g.FileList);
	BindType<ast::List>(g.List);
	BindType<ast::NameReference>(g.NameReference);
	BindType<ast::Record>(g.Record);
	BindType<ast::TypeDeclaration>(g.TypeDeclaration);

	BindParser<ast::UnaryOperation::Negative>(g.NegativeOperation);
	BindParser<ast::UnaryOperation::Not>(g.NotOperation);
	BindParser<ast::UnaryOperation::Positive>(g.PositiveOperation);

	BindParser<ast::BinaryOperation::And>(g.AndOperation);
	BindParser<ast::BinaryOperation::Or>(g.OrOperation);
	BindParser<ast::BinaryOperation::XOr>(g.XOrOperation);

	BindParser<ast::BinaryOperation::LessThan>(g.LessThanOperation);
	BindParser<ast::BinaryOperation::GreaterThan>(g.GreaterThanOperation);
	BindParser<ast::BinaryOperation::Equals>(g.EqualsOperation);
	BindParser<ast::BinaryOperation::NotEqual>(g.NotEqualOperation);

	BindParser<ast::BinaryOperation::Add>(g.AddOperation);
	BindParser<ast::BinaryOperation::Prefix>(g.PrefixOperation);
	BindParser<ast::BinaryOperation::ScalarAdd>(g.ScalarAddOperation);

	BindType<ast::Value>(g.Value);
	BindType<ast::Scope>(g.Values);
}

ParserDelegate::~ParserDelegate()
{
}


pegmatite::ErrorReporter ParserDelegate::pegErr()
{
	return [this](const InputRange& src, std::string message)
	{
		errors_.ReportError(message, SourceRange(src));
	};
}


UniqPtr<ast::Scope>
ParserDelegate::Parse(pegmatite::Input& input, const ast::Scope& containingScope)
{
	unique_ptr<ast::Scope::Parser> parseTree;
	if (not parse(input, grammar_.Values, grammar_.Ignored, pegErr(), parseTree))
		return nullptr;

	unique_ptr<ast::Scope> scope(parseTree->Build(containingScope, types_, errors_));

	return scope;
}

UniqPtr<ast::Value>
ParserDelegate::ParseValue(pegmatite::Input& input, const ast::Scope& containingScope)
{
	unique_ptr<ast::Value::Parser> value;
	if (not parse(input, grammar_.Value, grammar_.Ignored, pegErr(), value))
		return nullptr;

	return UniqPtr<ast::Value>(value->Build(containingScope, types_, errors_));
}


#if 0
//
// AST scopes:
//
ast::Scope& ParserDelegate::CurrentScope()
{
	// We must always have at least a top-level scope on the stack.
	assert(scopes_.size() > 0);
	return *scopes_.top();
}


ast::Scope& ParserDelegate::EnterScope(const string& name, const Type& /*args*/, SourceRange /*src*/)
{
	Bytestream::Debug("parser.scope")
		<< string(scopes_.size(), ' ')
		<< Bytestream::Operator << " >> "
		<< Bytestream::Type << "scope"
		<< Bytestream::Literal << " '" << name << "'"
		<< Bytestream::Reset << "\n"
		;

	const bool FirstScope = scopes_.empty();

	const ast::Scope *parent = FirstScope ? nullptr : &CurrentScope();
	scopes_.emplace(new ast::Scope(parent/*, name, args*/));

#if 0
	if (definitions_)
	{
		Builtin(Arguments, definitions_, src);
		definitions_.release();
	}
#endif

	return *scopes_.top();
}

ast::Scope& ParserDelegate::EnterScope(const string& name)
{
	return EnterScope(name, types_.nilType());
}

ast::Scope& ParserDelegate::EnterScope(ast::Scope&& s)
{
	Bytestream::Debug("parser.scope")
		<< string(scopes_.size(), ' ')
		<< Bytestream::Operator << " >> "
		<< Bytestream::Type << "scope"
		<< Bytestream::Literal << " '" << s.name() << "'"
		<< Bytestream::Reset << "\n"
		;

	scopes_.emplace(new ast::Scope(std::move(s)));

	return *scopes_.top();
}

unique_ptr<ast::Scope> ParserDelegate::ExitScope()
{
	unique_ptr<ast::Scope> scope = std::move(scopes_.top());
	assert(scope and not scopes_.top());
	scopes_.pop();

	Bytestream& dbg = Bytestream::Debug("parser.scope");
	dbg
		<< string(scopes_.size(), ' ')
		<< Bytestream::Operator << " << "
		<< Bytestream::Type << "scope"
		<< Bytestream::Literal << " '" << scope->name() << "'"
		<< Bytestream::Operator << ":"
		;

	for (auto& v : scope->symbols())
		dbg << " " << v.first;

	dbg
		<< Bytestream::Reset << "\n"
		;

	return std::move(scope);
}
#endif


/*
bool ParserDelegate::Builtin(string name, UniqPtr<ast::Expression>& e, SourceRange src)
{
	UniqPtr<Token> token(new Token(name, src));
	UniqPtr<Identifier> id(Id(std::move(token)));

	return DefineValue(id, e, true);
}


bool ParserDelegate::Builtin(string name, int value, SourceRange src)
{
	UniqPtr<Expression> val(ParseInt(value));

	return Builtin(name, val, src);
}


bool ParserDelegate::Builtin(string name, string value, SourceRange src)
{
	UniqPtr<Token> token(new Token(value, src));
	UniqPtr<Expression> val(ParseString(std::move(token)));

	return Builtin(name, val, src);
}


bool ParserDelegate::Builtin(string name, UniqPtr<Scope>& scope, SourceRange src)
{
	EnterScope(std::move(*scope));
	UniqPtr<Expression> value(Record(src));

	return Builtin(name, value, src);
}
*/


#if 0
ParserDelegate::NodePtr ParserDelegate::File(const InputRange& input, ASTStack *stack)
{
	auto arguments = nodes<ast::Argument>(input, stack, StopOnWrongType);
	auto filename = pop<ast::Expression>(stack);

	if (not filename->type().isSubtype(StringType::get(types_)))
	{
		ReportError("filename should be of type 'string', not '"
		             + filename->type().str() + "'", *filename);
		return NodePtr();
	}

	const FileType& type = types_.fileType();

	return NodePtr {
		ast::Filename::Create(filename, arguments, type, input)
	};
}

ParserDelegate::NodePtr ParserDelegate::Files(const InputRange& input, ASTStack *stack)
{
	auto files = nodes<ast::Filename>(input, stack);
	const Type& type = types_.fileListType();

	return NodePtr {
		new ast::FileList(files, UniqPtrVec<ast::Argument>(), type, input)
	};
}

ParserDelegate::NodePtr
ParserDelegate::FilesWithArgs(const InputRange& input, ASTStack *stack)
{
	auto args = nodes<ast::Argument>(input, stack, StopOnWrongType);
	auto files = nodes<ast::Filename>(input, stack);

	return NodePtr {
		new ast::FileList(files, std::move(args), types_.fileListType(), input)
	};
}

ParserDelegate::NodePtr
ParserDelegate::Filename(const InputRange& input, ASTStack*)
{
	unique_ptr<ast::Expression> filename {
		new ast::StringLiteral(input.str(), StringType::get(types_), input)
	};

	UniqPtrVec<ast::Argument> args;
	const FileType& type = types_.fileType();

	return NodePtr { ast::Filename::Create(filename, args, type, input) };
}

ParserDelegate::NodePtr
ParserDelegate::NamedArgument(const InputRange&, ASTStack *stack)
{
	auto value = pop<ast::Expression>(stack);
	auto name = pop<ast::Identifier>(stack);

	return NodePtr { new ast::Argument(name, value) };
}

ParserDelegate::NodePtr
ParserDelegate::UnnamedArgument(const InputRange&, ASTStack *stack)
{
	unique_ptr<ast::Identifier> name;
	auto value = pop<ast::Expression>(stack);

	return NodePtr { new ast::Argument(name, value) };
}

ParserDelegate::NodePtr ParserDelegate::List(const InputRange& input, ASTStack *stack)
{
	auto elements = nodes<ast::Expression>(input, stack);

	const Type& elementType = types_.supertype(elements.begin(), elements.end());
	const Type& type = types_.listOf(elementType, input);

	return NodePtr { new ast::List(elements, type, input) };
}

ParserDelegate::NodePtr ParserDelegate::ScopedValues(const Input& input, Stack* stack)
{
	auto scope = ExitScope();
	scope->UpdateSource(input);

	for (auto& value : nodes<ast::Value>(input, stack))
		scope->Take(value);

	return NodePtr { scope.release() };
}


StringLiteral* Parser::ParseString(UniqPtr<fabrique::Token>&& t)
{
	if (not t)
		return nullptr;

	return new StringLiteral(*t, StringType::get(ctx_), t->source());
}
#endif



#if 0
const Type& Parser::getType(const string& name,
                            const SourceRange& begin, const SourceRange& end,
                            const PtrVec<Type>& params)
{
	const SourceRange src(begin, end);

	const Type& t = ctx_.find(name, src, params);
	if (not t)
	{
		ReportError("unknown type", src);
		return t;
	}

	return t;
}

const Type& Parser::getType(UniqPtr<Identifier>&& name,
                            UniqPtr<const PtrVec<Type>>&& params)
{
	static const PtrVec<Type>& empty = *new PtrVec<Type>;
	if (not name)
		return ctx_.nilType();

	const Type& userType = CurrentScope().Lookup(*name);
	if (userType)
		return userType;

	const SourceRange src = name->source();
	return getType(name->name(), src, src, params ? *params : empty);
}


const FunctionType& Parser::FnType(const PtrVec<Type>& inputs,
                                   const Type& output, SourceRange)
{
	return ctx_.functionType(inputs, output);
}


const RecordType* Parser::CreateRecordType(UniqPtr<UniqPtrVec<Identifier>>& f,
                                           SourceRange /*src*/)
{
	if (not f)
		return nullptr;

	Type::NamedTypeVec fields;
	for (UniqPtr<Identifier>& id : *f)
	{
		if (not id->isTyped())
		{
			ReportError("record fields must have a name and a type",
			            id->source());
			return nullptr;
		}

		fields.emplace_back(id->name(), id->type());
	}

	return &ctx_.recordType(fields);
}


Action* Parser::DefineAction(UniqPtr<UniqPtrVec<Argument>>& args,
                             const SourceRange& src,
                             UniqPtr<UniqPtrVec<Parameter>>&& params)
{
	if (not args)
		return nullptr;

	if (not params)
	{
		ReportError("action has no parameters", src);
		return nullptr;
	}

	ExitScope();

	bool hasOutput = false;
	for (const auto& p : *params)
	{
		if (p->type().hasOutput())
		{
			hasOutput = true;
			break;
		}
	}

	if (not hasOutput)
	{
		ReportError("action does not produce any output files", src);
		return nullptr;
	}

	return Action::Create(*args, params, src, ctx_);
}


Argument* Parser::Arg(UniqPtr<Expression>& value, UniqPtr<Identifier>&& name)
{
	if (not value)
		return nullptr;

	return new Argument(name, value);
}


BinaryOperation* Parser::BinaryOp(BinaryOperation::Operator op,
                                  UniqPtr<Expression>& lhs,
                                  UniqPtr<Expression>& rhs)
{
	if (not lhs or not rhs)
		return nullptr;

	return BinaryOperation::Create(std::move(lhs), op, std::move(rhs));
}


Call* Parser::CreateCall(UniqPtr<Expression>& targetExpr,
                         UniqPtr<UniqPtrVec<Argument>>& args,
                         const SourceRange& end)
{
	if (not targetExpr or not args)
		return nullptr;

	SourceRange src(targetExpr->source(), end);

	if (not targetExpr->type().isFunction())
	{
		ReportError("calling an uncallable expression", src);
		return nullptr;
	}

	auto& fnType = dynamic_cast<const FunctionType&>(targetExpr->type());

	// Do some sanity checking on the arguments (the kind that doesn't
	// require deep knowledge of the callee).
	bool seenKeywordArgument = false;
	for (auto& a : *args)
	{
		if (a->hasName())
			seenKeywordArgument = true;

		else if (seenKeywordArgument)
		{
			ReportError(
				"positional argument after keyword argument",
				a->source());
			return nullptr;
		}
	}

	return new Call(targetExpr, *args, fnType.returnType(), src);
}


CompoundExpression* Parser::CompoundExpr(UniqPtr<Expression>& result,
                                         SourceRange begin, SourceRange end)
{
	if (not result)
		return nullptr;

	SourceRange src = result->source();
	if (begin != SourceRange::None())
	{
		assert(end != SourceRange::None());
		src = SourceRange(begin, end);
	}

	return new CompoundExpression(ExitScope(), result, src);
}



FieldAccess* Parser::FieldAccess(UniqPtr<Expression>& record,
                                 UniqPtr<Identifier>& field)
{
	if (not record or not field)
		return nullptr;

	const string& name = field->name();

	const Type& recordType = record->type();
	if (not recordType.hasFields())
		throw SemanticException(
			"value of type '" + recordType.str() + "' does not have fields",
			record->source());

	Type::TypeMap fieldTypes { recordType.fields() };
	auto i = fieldTypes.find(name);
	if (i == fieldTypes.end())
		throw SemanticException("no such field", field->source());

	const Type& fieldType = i->second;

	if (field->isTyped())
		field->type().CheckSubtype(fieldType, field->source());
	else
		field.reset(Id(std::move(field), &fieldType));

	return new class FieldAccess(record, field);
}


FieldQuery* Parser::FieldQuery(UniqPtr<Expression>& record,
                               UniqPtr<Identifier>& field, UniqPtr<Expression>& def,
                               SourceRange src)
{
	if (not record or not field)
		return nullptr;

	const Type& recordType = record->type();
	if (not recordType.hasFields())
		throw SemanticException(
			"value of type '" + recordType.str() + "' does not have fields",
			record->source());

	const Type& defaultType = def->type();

	Type::TypeMap recordFields = recordType.fields();
	auto i = recordFields.find(field->name());
	if (i != recordFields.end())
	{
		const Type& fieldType = i->second;
		fieldType.CheckSubtype(defaultType, src);
	}

	const Type& t =
		i == recordFields.end()
			? defaultType
			: defaultType.supertype(i->second)
			;

	return new class FieldQuery(record, field, def, t, src);
}


Filename* Parser::File(UniqPtr<Expression>& name, const SourceRange& src,
                       UniqPtr<UniqPtrVec<Argument>>&& args)
{
	if (not name)
		return nullptr;

	static UniqPtrVec<Argument>& empty = *new UniqPtrVec<Argument>;

	if (not name->type().isSubtype(StringType::get(ctx_)))
	{
		ReportError("filename should be of type 'string', not '"
		             + name->type().str() + "'", *name);
		return nullptr;
	}

	return Filename::Create(name, args ? *args : empty, ctx_.fileType(), src);
}


ForeachExpr* Parser::Foreach(UniqPtr<Mapping>& mapping,
                             UniqPtr<Expression>& body,
                             const SourceRange& begin)
{
	if (not mapping or not body)
		return nullptr;

	SourceRange src(begin, body->source());
	ExitScope();

	const Type& resultTy = ctx_.listOf(body->type(), src);
	return new ForeachExpr(mapping, body, resultTy, src);
}


TypeDeclaration* Parser::DeclareType(const RecordType& t, SourceRange src)
{
	return new TypeDeclaration(ctx_.userType(t), src);
}


Function* Parser::DefineFunction(const SourceRange& begin,
                                 UniqPtr<UniqPtrVec<Parameter>>& params,
                                 UniqPtr<Expression>& body,
                                 const Type *resultType)
{
	if (not params or not body)
		return nullptr;

	if (resultType and not body->type().isSubtype(*resultType))
	{
		ReportError(
			"wrong return type ("
			+ body->type().str() + " not a subtype of "
			+ resultType->str()
			+ ")", *body);
		return nullptr;
	}

	SourceRange loc(begin, *body);

	PtrVec<Type> parameterTypes;
	for (auto& p : *params)
		parameterTypes.push_back(&p->type());

	ExitScope();

	const Type& retTy = resultType ? *resultType : body->type();
	const FunctionType& ty = ctx_.functionType(parameterTypes, retTy);
	return new Function(*params, ty, body, loc);
}





Import* Parser::ImportModule(UniqPtr<StringLiteral>& name, UniqPtrVec<Argument>& args,
                             SourceRange src)
{
	if (not name)
		return nullptr;

	Bytestream& dbg = Bytestream::Debug("parser.import");
	dbg
		<< Bytestream::Action << "importing"
		<< Bytestream::Type << " module"
		<< Bytestream::Operator << " '"
		<< Bytestream::Literal << name->str()
		<< Bytestream::Operator << "'"
		<< Bytestream::Reset << " with "
		;

	if (not args.empty())
	{
		dbg << "args ";
		for (const UniqPtr<Argument>& a : args)
			dbg << " " << *a;

		dbg << Bytestream::Reset << " and ";
	}

	dbg
		<< Bytestream::Reference << "srcroot "
		<< Bytestream::Filename << srcroot_
		<< Bytestream::Reset << "\n"
		;

	if (auto plugin = pluginRegistry_.lookup(name->str()).lock())
	{
		UniqPtr<plugin::Plugin> instance(plugin->Instantiate(ctx_));

		dbg
			<< Bytestream::Action << "found"
			<< Bytestream::Type << " plugin "
			<< Bytestream::Definition << plugin->name()
			<< Bytestream::Reset << " with type "
			<< instance->type()
			<< "\n"
			;

		return new Import(name, args, instance, src);
	}

	if (auto plugin = pluginLoader_.Load(name->str()).lock())
	{
		UniqPtr<plugin::Plugin> instance(plugin->Instantiate(ctx_));

		dbg
			<< Bytestream::Action << "loaded"
			<< Bytestream::Type << " plugin "
			<< Bytestream::Definition << plugin->name()
			<< Bytestream::Reset << " with type "
			<< instance->type()
			<< "\n"
			;

		return new Import(name, args, instance, src);
	}

	const string subdir(currentSubdirectory_.top());
	string filename;
	try { filename = FindModule(srcroot_, subdir, name->str()); }
	catch (const UserError& e)
	{
		ReportError(e.message(), src);
		return nullptr;
	}

	const string directory = DirectoryOf(filename);

	currentSubdirectory_.push(directory);

	const string absolute =
		PathIsAbsolute(filename) ? filename : JoinPath(srcroot_, filename);

	dbg
		<< Bytestream::Action << "found"
		<< Bytestream::Type << " module "
		<< Bytestream::Operator << "'"
		<< Bytestream::Literal << filename
		<< Bytestream::Operator << "'"
		<< Bytestream::Reset << " at "
		<< Bytestream::Operator << "'"
		<< Bytestream::Literal << absolute
		<< Bytestream::Operator << "'"
		<< "\n"
		;

	std::ifstream input(absolute);
	if (not input.good())
		throw UserError("Can't open '" + filename + "'");

	Type::NamedTypeVec argTypes;
	for (UniqPtr<Argument>& a : args)
	{
		if (not a->hasName())
		{
			ReportError("import argument must be named",
			            a->source());
			return nullptr;
		}

		argTypes.emplace_back(a->getName().name(), a->type());
	}

	const RecordType& argsType = ctx_.recordType(argTypes);

	UniqPtr<Scope> module = ParseFile(input, argsType, absolute);
	if (not module)
		return nullptr;

	currentSubdirectory_.pop();

	Type::NamedTypeVec fields;
	for (const UniqPtr<Value>& value : module->values())
	{
		const Identifier& id { value->name() };
		if (not id.reservedName())
			fields.emplace_back(id.name(), value->type());
	}
	const RecordType& ty { ctx_.recordType(fields) };

	return new Import(name, args, directory, module, ty, src);
}


Conditional* Parser::IfElse(const SourceRange& ifLocation,
                            UniqPtr<Expression>& condition,
                            UniqPtr<Expression>& thenResult,
                            UniqPtr<Expression>& elseResult)
{
	if (not condition or not thenResult or not elseResult)
		return nullptr;

	const Type& type = thenResult->type().supertype(elseResult->type());
	if (not type)
	{
		ReportError("incompatible types for conditional clauses: "
		            + thenResult->type().str()
			    + " vs " + elseResult->type().str(),
		            SourceRange::Over(thenResult, elseResult));
		return nullptr;
	}

	return new Conditional(ifLocation, condition, thenResult, elseResult, type);
}


List* Parser::ListOf(UniqPtrVec<Expression>& elements,
                     const SourceRange& src)
{
	const Type& elementType = ctx_.supertype(elements.begin(), elements.end());
	const Type& ty = ctx_.listOf(elementType, src);
	return new List(elements, ty, src);
}


Mapping* Parser::Map(UniqPtr<Expression>& source, UniqPtr<Identifier>& target)
{
	if (not source or not target)
		return nullptr;

	UniqPtr<Identifier> id(std::move(target));

	Bytestream& dbg = Bytestream::Debug("parser.map");
	dbg
		<< "mapping: " << *id
		<< Bytestream::Operator << " <- "
		<< *source
		<< Bytestream::Operator << ":"
		<< source->type()
		<< "\n"
		;

	assert(source->type());
	if (source->type().typeParamCount() != 1)
	{
		ReportError("cannot iterate over " + source->type().str(),
		            source->source());
		return nullptr;
	}

	const Type& elementType = source->type()[0];

	if (id->isTyped())
	{
		if (not id->type().isSupertype(elementType))
		{
			ReportError("incompatible types for map: "
			            + id->type().str() + " vs "
			            + elementType.str(),
			            SourceRange::Over(source, target));
			return nullptr;
		}
	}
	else
		id.reset(Id(std::move(id), &elementType));


	UniqPtr<Parameter> parameter(Param(std::move(id)));

	SourceRange src(*parameter, *source);
	return new Mapping(parameter, source, src);
}

SomeValue* Parser::Some(UniqPtr<Expression>& initializer, SourceRange src)
{
	if (not initializer)
		return nullptr;

	const Type& type = ctx_.maybe(initializer->type(), src);
	return new SomeValue(type, initializer, src);
}

Record* Parser::Record(SourceRange src)
{
	UniqPtr<Scope> scope(ExitScope());

	Type::NamedTypeVec fields;
	for (const UniqPtr<Value>& v : scope->values())
		fields.emplace_back(v->name().name(), v->type());

	const RecordType& t = ctx_.recordType(fields);

	return new Record(scope, t, src);
}

BoolLiteral* Parser::True()
{
	return new BoolLiteral(true, BooleanType::get(ctx_),
	                       lexer_.CurrentTokenRange());
}

BoolLiteral* Parser::False()
{
	return new BoolLiteral(false, BooleanType::get(ctx_),
	                       lexer_.CurrentTokenRange());
}

IntLiteral* Parser::ParseInt(int value)
{
	return new IntLiteral(value, IntegerType::get(ctx_),
	                      lexer_.CurrentTokenRange());
}

StringLiteral* Parser::ParseString(UniqPtr<fabrique::Token>&& t)
{
	if (not t)
		return nullptr;

	return new StringLiteral(*t, StringType::get(ctx_), t->source());
}


Parameter* Parser::Param(UniqPtr<Identifier>&& name,
                         UniqPtr<Expression>&& defaultValue)
{
	if (not name)
		return nullptr;

	if (not (name->isTyped() or (defaultValue and defaultValue->type())))
	{
		ReportError("expected type or default value", *name);
		return nullptr;
	}

	if (name->isTyped() and defaultValue
	    and not defaultValue->type().isSubtype(name->type()))
	{
		ReportError("expected type " + name->type().str()
		            + ", got " + defaultValue->type().str(),
		            *defaultValue);
		return nullptr;
	}

	const Type& resultType =
		name->isTyped() ? name->type() : defaultValue->type();
	assert(resultType);

	auto *p = new Parameter(name, resultType, std::move(defaultValue));
	CurrentScope().Register(p);

	return p;
}


SymbolReference* Parser::Reference(UniqPtr<Identifier>&& name)
{
	if (not name)
		return nullptr;

	const Type& t = CurrentScope().Lookup(*name);
	if (not t.valid())
	{
		ReportError("reference to undefined value", *name);
		return nullptr;
	}

	return new SymbolReference(std::move(name), t);
}


SymbolReference* Parser::Reference(UniqPtr<class FieldAccess>&& access)
{
	if (not access)
		return nullptr;

	const Type& fieldType = access->type();
	return new SymbolReference(std::move(access), fieldType);
}


DebugTracePoint* Parser::TracePoint(UniqPtr<Expression>& e, SourceRange src)
{
	return new DebugTracePoint(e, src);
}


UnaryOperation* Parser::UnaryOp(UnaryOperation::Operator op,
                                const SourceRange& opSrc,
                                UniqPtr<Expression>& e)
{
	if (not e)
		return nullptr;

	return UnaryOperation::Create(op, opSrc, e);
}


bool Parser::Builtin(string name, UniqPtr<Expression>& e, SourceRange src)
{
	UniqPtr<Token> token(new Token(name, src));
	UniqPtr<Identifier> id(Id(std::move(token)));

	return DefineValue(id, e, true);
}


bool Parser::Builtin(string name, int value, SourceRange src)
{
	UniqPtr<Expression> val(ParseInt(value));

	return Builtin(name, val, src);
}


bool Parser::Builtin(string name, string value, SourceRange src)
{
	UniqPtr<Token> token(new Token(value, src));
	UniqPtr<Expression> val(ParseString(std::move(token)));

	return Builtin(name, val, src);
}


bool Parser::Builtin(string name, UniqPtr<Scope>& scope, SourceRange src)
{
	EnterScope(std::move(*scope));
	UniqPtr<Expression> value(Record(src));

	return Builtin(name, value, src);
}


void Parser::AddToScope(const PtrVec<Argument>& args)
{
	auto& scope(CurrentScope());

	for (auto *arg : args)
		if (arg->hasName())
			scope.Register(arg);
}


fabrique::Token* Parser::ParseToken(YYSTYPE& yyunion)
{
	assert(yyunion.token);
	assert(dynamic_cast<fabrique::Token*>(yyunion.token));

	return yyunion.token;
}


bool Parser::Set(YYSTYPE& yyunion, Node *e)
{
	if (not e)
		return false;

	Bytestream& dbg = Bytestream::Debug("parser.node");
	dbg
		<< Bytestream::Action << "parsed "
		<< Bytestream::Type << "AST node"
		<< Bytestream::Reset
		;

	if (auto *typed = dynamic_cast<const Typed*>(e))
		dbg << " of type " << typed->type();

	else if (auto *ot = dynamic_cast<const OptionallyTyped*>(e))
	{
		if (ot->isTyped())
			dbg << " with type " << ot->type();
	}

	dbg
		<< Bytestream::Operator << ": "
		<< Bytestream::Reset << *e
		<< Bytestream::Operator << " @ " << e->source()
		<< "\n"
		;

	yyunion.node = e;
	return true;
}
#endif
