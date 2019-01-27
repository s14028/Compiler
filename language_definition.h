#pragma once

#include <string>
#include <type_traits>
#include <variant>

enum class terminal_type : unsigned char
{
	IF = 0,
	IF_ELSE = 1,
	WHILE = 2,
	DO_WHILE = 3,
	ASSIGNMENT = 4,
	PRINT = 5,
	READ = 6,
	EXIT = 7,
	PASS = 8
};

struct terminal
{
	terminal_type type;
	unsigned bool_expr_length;
	unsigned first_section_length;
	unsigned second_section_length;
};

enum class instruction_type : unsigned char
{
	AND = 0,
	OR = 1,
	NOT = 2,
	LT = 3,
	LE = 4,
	EQ = 5,
	NE = 6,
	GE = 7,
	GT = 8,
	SUM = 9,
	SUBTRACTION = 10,
	MULTIPLICATION = 11,
	DIVISION = 12,
	MODUL = 13,
};

using constant = std::variant<
	long long int,
	bool
>;

using statement = std::variant<
	terminal, // Terminal
	instruction_type, // Instruction
	constant, // Constant
	std::string // Variable
>;

