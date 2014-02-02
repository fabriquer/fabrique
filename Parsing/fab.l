%{

#include "AST/ast.h"

#include "Parsing/Lexer.h"
#include "Parsing/lex.h"
#include "Parsing/yacc.h"

#include <math.h>

void		yyerror(const char *message);
int		yylex(YYSTYPE *yylval);

char buf[100];
char *s;

/* keep track of column numbers */
int lastLine = -1;
int lastLength = 0;
int yycolumn = -1;

#define YY_USER_ACTION { \
	if (yylineno != (lastLine + 1)) { \
		yycolumn = 1; \
		lastLine = yylineno - 1; \
	} \
	else \
		yycolumn += lastLength; \
	\
	lastLength = yyleng; \
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
tab		[\t]
newline		[\n]
whitespace	[ \r]+

%%

{comment}	setCString(yylval); yylval->s.len--;	/* ignore */

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

true							return TRUE;
false							return FALSE;
{digit}+		yylval->i = atoi(yytext);	return INT_LITERAL;
if			setRange(yylval);		return IF;
else			setRange(yylval);		return ELSE;
foreach			setRange(yylval);		return FOREACH;
as			setRange(yylval);		return AS;
action			setRange(yylval);		return ACTION;
files			setRange(yylval);		return FILES;
file			setRange(yylval);		return FILE_TOKEN;
function		setRange(yylval);		return FUNCTION;
return			setRange(yylval);		return RETURN;
not			setRange(yylval);		return NOT;
and			setRange(yylval);		return AND;
or			setRange(yylval);		return OR;
xor			setRange(yylval);		return XOR;
"<-"			setRange(yylval);		return INPUT;
"+"			setRange(yylval);		return ADD;
"::"			setRange(yylval);		return PREFIX;
".+"			setRange(yylval);		return SCALAR_ADD;
"{"|"}"|"("|")"|"["|"]"	setRange(yylval);		return *yytext;
":"|","|";"|"="		setRange(yylval);		return *yytext;
{identifier}		setCString(yylval);		return IDENTIFIER;
{filename}		setCString(yylval);		return FILENAME;
{tab}			yycolumn += 7;			/* ignore */
{newline}		yycolumn--;			/* ignore */
{whitespace}						/* ignore */
