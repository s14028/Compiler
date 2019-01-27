%{
#include <iostream>
#include <fstream>
#include <string>

#include <cstdio>

#include "grammar.tab.h"
extern "C" int yylex();

YY_BUFFER_STATE initial_buffer;

const std::string temporary_fname = ".temporary.lex";

unsigned indentation_spaces = 0;
int prev_indentation = 0;
unsigned number_of_closes = 0;
%}

IF "if"
ELSE "else"
WHILE "while"
DO "do"

AND "and"
OR "or"
NOT "not"
LT "<"
LE "<="
EQ "="
NE "!="
GE ">="
GT ">"

SUM "+"
SUBTRACTION "-"
MULTIPLICATION "*"
DIVISION "/"
MODUL "%"

ASSIGNMENT ":="

PRINT "print"
READ "read"
EXIT "exit"
PASS "pass"

INTEGER ([1-9][0-9]*)|0
TRUE "true"
FALSE "false"
IDENTIFIER [A-Za-z_][A-Za-z0-9_]*

%x CLOSE_SECTION
%x CLOSE_AND_DELETE

%%

{IF} return IF;
{ELSE} return ELSE;
{WHILE} return WHILE;
{DO} return DO;

{AND} return AND;
{OR} return OR;
{NOT} return NOT;

{LT} return LT;
{LE} return LE;
{EQ} return EQ;
{NE} return NE;
{GE} return GE;
{GT} return GT;

{SUM} return SUM;
{SUBTRACTION} return SUBTRACTION;
{MULTIPLICATION} return MULTIPLICATION;
{DIVISION} return DIVISION;
{MODUL} return MODUL;

{ASSIGNMENT} return ASSIGNMENT;

{PRINT} return PRINT;
{READ} return READ;
{EXIT} return EXIT;
{PASS} return PASS;

{INTEGER} {
	std::string text(yytext);
	long long int value = std::stoll(text);
	yylval.integer_value = value;
	return INTEGER;
}

{TRUE} {
	yylval.bool_value = true;
	return BOOL;
}

{FALSE} {
	yylval.bool_value = false;
	return BOOL;
}

{IDENTIFIER} {
	char* identifier = strdup(yytext);
	yylval.identifier_value = identifier;
	return IDENTIFIER;
}

[:]	return ':';

^" "+ {
	unsigned current_indentation = yyleng;
	unsigned _prev_indentation = prev_indentation;
	prev_indentation = current_indentation;

	if(indentation_spaces == 0)
	{
		indentation_spaces = current_indentation;
		return SECTION_BEGIN;
	}
	if(current_indentation == _prev_indentation + indentation_spaces)
	{
		return SECTION_BEGIN;
	}
	if(current_indentation < _prev_indentation)
	{
		unsigned diff = _prev_indentation - current_indentation;

		if(diff % indentation_spaces == 0)
		{
			number_of_closes = diff / indentation_spaces;
			yyless(0);
			BEGIN(CLOSE_SECTION);
		}
	}
}

\n/[^ ] {
	if(prev_indentation > 0)
	{
		prev_indentation -= indentation_spaces;
		yyless(0);
		return SECTION_END;
	}
}

<INITIAL><<EOF>> {
	if(prev_indentation > 0)
	{
		initial_buffer = YY_CURRENT_BUFFER;
		number_of_closes = prev_indentation / indentation_spaces;

		std::ofstream tfile(temporary_fname, std::ios_base::out);

		tfile << " ";
		tfile.flush();

		tfile.close();

		yyin = fopen(temporary_fname.c_str(), "r");

		yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));

		BEGIN(CLOSE_AND_DELETE);
	}
	else
	{
		yyterminate();
	}
}

" "+ ;

[\t\n]+ ;

<CLOSE_SECTION>" "+ {
	if(number_of_closes > 0)
	{
		number_of_closes--;
		yyless(0);
		return SECTION_END;
	}
	BEGIN(INITIAL);
}

<CLOSE_AND_DELETE>" " {
	if(number_of_closes > 0)
	{
		number_of_closes--;
		yyless(0);
		return SECTION_END;
	}
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_switch_to_buffer(initial_buffer);
	remove(temporary_fname.c_str());
}

%%
