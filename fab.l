%{

#include "ast.h"
#include "Lexer.h"

#include "lex.h"
#include "yacc.h"

#include <math.h>

int		yyerror(const char *message);
int		yylex(YYSTYPE *yylval);

void PrintUnion(void *yylval)
{
	auto y = (YYSTYPE*) yylval;
	std::cout
		<< "{"
		<< " int: " << y->i
		<< " str: '" << ((unsigned long) y->s.cstr) << "'"
		<< " id: " << y->id
		<< " expr: " << y->expr
		<< " op: " << y->op
		<< " val: " << y->val
		<< " }"
		<< std::endl
		;
}

char buf[100];
char *s;

/* keep track of column numbers */
int lastLine = -1;
int yycolumn = -1;

#define YY_USER_ACTION { \
	if (yylineno != lastLine) { \
		yycolumn = 0; \
		lastLine = yylineno; \
	} \
	else \
		yycolumn += yyleng; \
}

%}

%option c++
%option yylineno

/* single- and double-quoted strings: */
%x SQSTR
%x DQSTR

comment		#.*\n
identifier	[_a-zA-Z][_a-zA-Z0-9]*
filename	[_a-zA-Z0-9\.\/]+
digit		[0-9]
whitespace	[ \t\n\r]+

%%

{comment}	setCString(yylval);		/* ignore */

\'		{ BEGIN SQSTR; s = buf; }	// open a string literal
\"		{ BEGIN DQSTR; s = buf; }

<SQSTR>\\\'	{ *s++ = '\''; *s++ = '\''; }	// escapes within a string
<DQSTR>\\\"	{ *s++ = '\"'; }

<SQSTR>"'"	{				// close a string literal
	*s = 0; BEGIN 0; yylval->s.set(buf, s);
	return STRING_LITERAL;
}

<DQSTR>\"	{
	*s = 0; BEGIN 0; yylval->s.set(buf, s);
	return STRING_LITERAL;
}

<SQSTR,DQSTR>\n	{ printf("invalid string"); exit(1); }	// other characters
<SQSTR,DQSTR>.	{ *s++ = *yytext; }

true						return TRUE;
false						return FALSE;
{digit}+	yylval->i = atoi(yytext);	return INT_LITERAL;
action						return ACTION;
files						return FILES;
file						return FILE;
function					return FUNCTION;
"++"						return CONCAT;
"::"						return PREFIX;
".+"						return SCALAR_ADD;
"{"|"}"|"("|")"|"["|"]"				return *yytext;
":"|","|";"|"="					return *yytext;
{identifier}	setCString(yylval);		return IDENTIFIER;
{filename}	setCString(yylval);		return FILENAME;
{whitespace}					/* ignore */
