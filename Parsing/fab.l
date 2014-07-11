%{

#include "AST/ast.h"

#include "Parsing/Lexer.h"
#include "Parsing/Token.h"

#include "Parsing/lex.h"
#include "Parsing/yacc.h"

#include <math.h>

void		yyerror(const char *message);
int		yylex(YYSTYPE *yylval);

/* keep track of column numbers */
static int	previousLine = 0;
size_t		yycolumn = 1;

/*
 * Before each token is processed, save its string data and
 * location in the source code.
 */
#define YY_USER_ACTION { \
	/*
	const SourceRange& src = currentToken.source(); \
	if (src.end.line > 1 or src.end.column > 1) \
		Bytestream::Debug("lex.prelex") \
			<< Bytestream::Action << "lexed: " \
			<< currentToken << "\n"; \
	*/ \
	currentToken_ = NextToken(); \
	\
	if (yylineno > previousLine) \
	{ \
		yycolumn = 1; \
		previousLine = yylineno; \
	} \
	else \
	{ \
		yycolumn += yyleng; \
	} \
}

%}

%option c++
%option yylineno

/* single- and double-quoted strings: */
%x SQSTR
%x DQSTR

/* raw filenames can only be used inside of 'files()': */
%x SAW_FILES
%x IN_FILES

comment		#.*\n
identifier	[_a-zA-Z][_a-zA-Z0-9]*
filename	[_a-zA-Z0-9\.\/\-]+
digit		[0-9]
tab		[\t]
newline		[\n]
whitespace	[ \r]+

%%

{comment}	SetComment(yylval, true);	/* comment includes a newline */

\'		{ BEGIN SQSTR; BeginString(); }	/* open a string literal */
\"		{ BEGIN DQSTR; BeginString(); }

<SQSTR>\\\'	{ AppendChar('\''); }		/* escapes within a string */
<DQSTR>\\\"	{ AppendChar('"'); }

<SQSTR>"'"	{				/* close a string literal */
	BEGIN 0;
	EndString(yylval);
	return STRING_LITERAL;
}

<DQSTR>\"	{
	BEGIN 0;
	EndString(yylval);
	return STRING_LITERAL;
}

<SQSTR,DQSTR>\n	{ printf("invalid string"); exit(1); }	// other characters
<SQSTR,DQSTR>.	{ AppendChar(*yytext); }

files			{
	BEGIN SAW_FILES;
	SetToken(yylval);
	return FILES;
}

<SAW_FILES>\(		{
	BEGIN IN_FILES;
	return '(';
}

<IN_FILES>{filename}	{
	SetToken(yylval);
	return FILENAME;
}

<IN_FILES>\n		{
	/* Treat newlines inside of 'files()' like other newlines: ignore. */
}

<IN_FILES>\)		{
	/* Closing parenthesis puts us back in normal mode. */
	BEGIN 0;
	SetToken(yylval);
	return ')';
}

<IN_FILES>\,		{
	/* A comma also puts us back (parse arguments, not raw filenames). */
	BEGIN 0;
	SetToken(yylval);
	return ',';
}

true							return TRUE;
false							return FALSE;
{digit}+		yylval->intVal = atoi(yytext);	return INT_LITERAL;
if			SetToken(yylval);		return IF;
else			SetToken(yylval);		return ELSE;
foreach			SetToken(yylval);		return FOREACH;
as			SetToken(yylval);		return AS;
action			SetToken(yylval);		return ACTION;
file			SetToken(yylval);		return FILE_TOKEN;
function		SetToken(yylval);		return FUNCTION;
import			SetToken(yylval);		return IMPORT;
nil			SetToken(yylval);		return NIL;
return			SetToken(yylval);		return RETURN;
some			SetToken(yylval);		return SOME;
struct			SetToken(yylval);		return STRUCT;
not			SetToken(yylval);		return NOT;
and			SetToken(yylval);		return AND;
or			SetToken(yylval);		return OR;
xor			SetToken(yylval);		return XOR;
==			SetToken(yylval);		return EQUAL;
!=			SetToken(yylval);		return NEQUAL;
"<-"			SetToken(yylval);		return INPUT;
"=>"			SetToken(yylval);		return PRODUCES;
"+"			SetToken(yylval);		return ADD;
"::"			SetToken(yylval);		return PREFIX;
".+"			SetToken(yylval);		return SCALAR_ADD;
"{"|"}"|"("|")"|"["|"]"	SetToken(yylval);		return *yytext;
":"|","|";"|"="|"."	SetToken(yylval);		return *yytext;
{identifier}		SetToken(yylval);		return IDENTIFIER;
{tab}			yycolumn += 7;			/* ignore */
{newline}						/* ignore */
{whitespace}						/* ignore */
