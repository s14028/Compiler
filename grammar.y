%{
#include <iostream>
#include <algorithm>
#include <iterator>
#include <functional>
#include <tuple>
#include <vector>

#include <cstdio>

#include "language_definition.h"
#include "translator.h"

extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE* yyin;

std::vector<statement> statements;

void yyerror(const char* error);

void push_integer(const long long int integer)
{
	constant cons(integer);
	statement st(cons);

	statements.emplace_back(st);
}

void push_bool(const bool value)
{
	constant cons(value);
	statement st(cons);

	statements.emplace_back(st);
}

void push_variable(std::string&& identifier)
{
	statement st(std::forward<std::string>(identifier));

	statements.emplace_back(st);
}

void push_terminal(terminal&& tl)
{
	statement st(std::forward<terminal>(tl));

	statements.emplace_back(st);
}

void push_instruction(const instruction_type instruction)
{
	statement st(instruction);

	statements.emplace_back(st);
}
%}

%define parse.error verbose

%union {
	unsigned statements_count;
	long long int integer_value;
	bool bool_value;
	char* identifier_value;
}

%token IF ELSE WHILE DO
%token AND OR NOT
%token LT LE EQ NE GE GT
%token SUM SUBTRACTION MULTIPLICATION DIVISION MODUL
%token ASSIGNMENT
%token PRINT READ EXIT PASS
%token SECTION_BEGIN SECTION_END

%token <integer_value> INTEGER
%token <bool_value> BOOL
%token <identifier_value> IDENTIFIER

%left OR
%left AND
%left NOT

%left SUM SUBTRACTION
%left MULTIPLICATION DIVISION
%left MODUL
%left UNARY_PLUS UNARY_MINUS

%type <statements_count> block statement block_expr if_expr while_expr logical_expr arithmetical_expr assignment_expr function_expr

%start program

%%

program:
	block
	;

block:
	block statement {
		$$ = $1 + $2;
	}
	| statement {
		$$ = $1;
	}
	;

statement:
	block_expr {
		$$ = $1;
	}
	| assignment_expr {
		$$ = $1;
	}
	| function_expr {
		$$ = $1;
	}
	;

block_expr:
	if_expr {
		$$ = $1;
	}
	| while_expr {
		$$ = $1;
	}
	;

if_expr:
	IF logical_expr ':' SECTION_BEGIN block SECTION_END {
		unsigned bool_expr_length = $2;
		unsigned section_length = $5;
		terminal tl {terminal_type::IF, bool_expr_length, section_length, 0};

		push_terminal(std::forward<terminal>(tl));

		$$ = bool_expr_length + section_length + 1;
	}
	| IF logical_expr ':' SECTION_BEGIN block SECTION_END ELSE ':' SECTION_BEGIN block SECTION_END {
		unsigned bool_expr_length = $2;
		unsigned if_section_length = $5;
		unsigned else_section_length = $10;

		terminal tl {terminal_type::IF_ELSE, bool_expr_length, if_section_length, else_section_length};

		push_terminal(std::forward<terminal>(tl));

		$$ = bool_expr_length + if_section_length + else_section_length + 1;
	}
	;

while_expr:
	WHILE logical_expr ':' SECTION_BEGIN block SECTION_END {
		unsigned bool_expr_length = $2;
		unsigned section_length = $5;
		terminal tl {terminal_type::WHILE, bool_expr_length, section_length, 0};

		push_terminal(std::forward<terminal>(tl));

		$$ = bool_expr_length + section_length + 1;
	}
	| DO ':' SECTION_BEGIN block SECTION_END WHILE logical_expr {
		unsigned bool_expr_length = $7;
		unsigned section_length = $4;
		terminal tl {terminal_type::DO_WHILE, bool_expr_length, section_length, 0};

		push_terminal(std::forward<terminal>(tl));

		$$ = bool_expr_length + section_length + 1;
	}

logical_expr:
	logical_expr OR logical_expr {
		push_instruction(instruction_type::OR);
		$$ = $1 + $3 + 1;
	}
	|	logical_expr AND logical_expr {
		push_instruction(instruction_type::OR);
		$$ = $1 + $3 + 1;
	}
	|	logical_expr EQ logical_expr {
		push_instruction(instruction_type::EQ);
		$$ = $1 + $3 + 1;
	}
	| NOT logical_expr {
		push_instruction(instruction_type::NOT);
		$$ = $2 + 1;
	}
	| '(' logical_expr ')' {
		$$ = $2;
	}
	| arithmetical_expr LT arithmetical_expr {
		push_instruction(instruction_type::LT);
		$$ = $1 + $3 + 1;
	}
	| arithmetical_expr LE arithmetical_expr {
		push_instruction(instruction_type::LE);
		$$ = $1 + $3 + 1;
	}
	| arithmetical_expr EQ arithmetical_expr {
		push_instruction(instruction_type::EQ);
		$$ = $1 + $3 + 1;
	}
	| arithmetical_expr NE arithmetical_expr {
		push_instruction(instruction_type::NE);
		$$ = $1 + $3 + 1;
	}
	| arithmetical_expr GE arithmetical_expr {
		push_instruction(instruction_type::GE);
		$$ = $1 + $3 + 1;
	}
	| arithmetical_expr GT arithmetical_expr {
		push_instruction(instruction_type::GT);
		$$ = $1 + $3 + 1;
	}
	| BOOL {
		push_bool($1);
		$$ = 1;
	}
	;

arithmetical_expr:
	arithmetical_expr SUM arithmetical_expr {	
		push_instruction(instruction_type::SUM);
		$$ = $1 + $3 + 1;
	}
	| arithmetical_expr SUBTRACTION arithmetical_expr {
		push_instruction(instruction_type::SUBTRACTION);
		$$ = $1 + $3 + 1;
	}
	| arithmetical_expr MULTIPLICATION arithmetical_expr {
		push_instruction(instruction_type::MULTIPLICATION);
		$$ = $1 + $3 + 1;
	}
	| arithmetical_expr DIVISION arithmetical_expr {
		push_instruction(instruction_type::DIVISION);
		$$ = $1 + $3 + 1;
	}
	| arithmetical_expr MODUL arithmetical_expr {
		push_instruction(instruction_type::MODUL);
		$$ = $1 + $3 + 1;
	}
	| '(' arithmetical_expr ')' {
		$$ = $2;
	}
	| SUM arithmetical_expr %prec UNARY_PLUS {
		$$ = $2;
	}
	| SUBTRACTION arithmetical_expr %prec UNARY_MINUS {
		push_integer(-1);
		push_instruction(instruction_type::MULTIPLICATION);

		$$ = $2 + 2;
	}
	| INTEGER {
		push_integer($1);
		$$ = 1;
	}
	| IDENTIFIER {
		std::string identifier($1);
		push_variable(std::forward<std::string>(identifier));

		free($1);
		$$ = 1;
	}
	;

assignment_expr:
	IDENTIFIER ASSIGNMENT arithmetical_expr {
		std::string identifier($1);
		terminal tl {terminal_type::ASSIGNMENT, 0, $3 + 1, 0};

		push_variable(std::forward<std::string>(identifier));
		push_terminal(std::forward<terminal>(tl));

		free($1);
		$$ = $3 + 2;
	}
	;

function_expr:
	PRINT logical_expr {
		terminal tl {terminal_type::PRINT, 0, $2, 0};
		push_terminal(std::forward<terminal>(tl));

		$$ = $2 + 1;
	}
	| PRINT arithmetical_expr {
		terminal tl {terminal_type::PRINT, 0, $2, 0};
		push_terminal(std::forward<terminal>(tl));

		$$ = $2 + 1;
	}
	| READ IDENTIFIER {
		std::string identifier($2);
		terminal tl {terminal_type::READ, 0, 1, 0};

		push_variable(std::forward<std::string>(identifier));
		push_terminal(std::forward<terminal>(tl));

		free($2);
		$$ = 2;
	}
	| EXIT {
		terminal tl {terminal_type::EXIT, 0, 0, 0};
		push_terminal(std::forward<terminal>(tl));

		$$ = 1;
	}
	| PASS {
		terminal tl {terminal_type::PASS, 0, 0, 0};
		push_terminal(std::forward<terminal>(tl));

		$$ = 1;
	}
	;
%%

void yyerror(const char* error)
{
	std::cout << error << std::endl;
	exit(-1);
}

int main(int argv, char** args)
{
	if(argv > 1)
	{
		yyin = fopen(args[1], "r");
		if(!yyin)
		{
			std::cout << "Cann't open file named: " << args[1] << std::endl;
			return 1;
		}
	}
	yyparse();

	std::for_each(std::begin(statements), std::end(statements), [](const statement& st)
	{
		std::visit([](auto&& node) {
			using T = std::decay_t<decltype(node)>;

      if constexpr (std::is_same_v<T, terminal>)
			{
				std::cout << "Block " << (unsigned) node.type << " "
					<< node.bool_expr_length << " "
					<< node.first_section_length << " "
					<< node.second_section_length << "\n";
			}
			else if constexpr (std::is_same_v<T, instruction_type>)
			{
				std::cout << "Instruction_type " << (unsigned) node << "\n";
			}
			else if constexpr (std::is_same_v<T, constant>)
			{
				std::cout << "Constant ";
				if(std::holds_alternative<long long int>(node))
				{
					std::cout << std::get<long long int>(node) << "\n";
				}
				else
				{
					std::cout << std::get<bool>(node) << "\n";
				}
			}
			else if constexpr (std::is_same_v<T, std::string>)
			{
				std::cout << "Variable " << node << std::endl;
			}
		}, st);
	});

	translator tr(statements);

	tr.translate_language();
	std::vector<std::string> instructions = tr.get_instructions();

	std::copy(std::begin(instructions), std::end(instructions), std::ostream_iterator<std::string>(std::cout, "\n"));
}
