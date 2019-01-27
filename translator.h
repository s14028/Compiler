#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <type_traits>
#include <variant>

#include "language_definition.h"

class translator
{
	// ================================
	// Statements parsed by bison
	std::vector<statement>& statements;
	// ================================



	// ================================
	// Map which stores index of terminal from statements vector and mapping to begining of that instruction in instructions vector
	std::unordered_map<unsigned, unsigned> terminal_to_instruction;
	// ================================

	

	// ================================
	// Map which stores identifier and connected with it index in DATA 0 0 0 0...0 line
	std::unordered_map<std::string, unsigned> variables;
	// ================================
	


	// ================================
	// Instructions to be produced
	std::vector<std::string> instructions;
	// ================================
	


	// ================================
	// COR for non terminals
	std::unordered_map<instruction_type, std::function<unsigned(unsigned)>> non_terminal_chain;
	// COR for terminals
	std::unordered_map<terminal_type, std::function<void(const unsigned)>> terminal_chain;
	// ================================

public:
	translator(std::vector<statement>& statements);

	std::vector<unsigned> terminal_indices();
	
	void translate_language();
	unsigned parse_expr(unsigned begin, unsigned end, unsigned insertion_index);
	std::vector<std::string> get_instructions();

private:
	void add_non_terminals();
	void add_terminals();
	std::string data_instruction();
	unsigned next_terminal(unsigned, unsigned);
	void emplace_instruction(unsigned, const std::string&);

};
