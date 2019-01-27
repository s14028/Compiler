#include "translator.h"

translator::translator(std::vector<statement>& statements) : statements(statements)
{
	add_non_terminals();
	add_terminals();
}

std::vector<unsigned> translator::terminal_indices()
{
	std::vector<unsigned> terminals;

	for(unsigned i = 0; i < std::size(statements); i++)
	{
		if(std::holds_alternative<terminal>(statements[i]))
		{
			terminals.emplace_back(i);
		}
	}
	return terminals;
}
	
void translator::translate_language()
{
	std::vector<unsigned> terminals = terminal_indices();

	for(unsigned i : terminals)
	{
		terminal& tl = std::get<terminal>(statements[i]);

		terminal_chain.at(tl.type)(i);
	}

	std::transform(std::begin(instructions), std::end(instructions), std::begin(instructions),
	[i=0](const std::string& instruction) mutable
	{
		if(instruction.front() == 'J')
		{
			auto iterator = std::find(std::begin(instruction), std::end(instruction), ' ');
			unsigned space_index = std::distance(std::begin(instruction), iterator);

			const std::string expr = instruction.substr(0, space_index);
			const std::string addr = instruction.substr(space_index + 2);

			long value = std::stol(addr);
			value += i;

			return std::to_string(i++) + " " + expr + " $" + std::to_string(value);
		}
		return std::to_string(i++) + " " + instruction;
	});

	emplace_instruction(0, data_instruction());
}

unsigned translator::parse_expr(unsigned begin, unsigned end, unsigned insertion_index)
{
	while(begin != end)
	{
		const statement& st = statements[begin];

		std::visit([this, &insertion_index](auto&& node)
		{
			using T = std::decay_t<decltype(node)>;

			if constexpr (std::is_same_v<T, instruction_type>)
			{
				insertion_index = non_terminal_chain.at(node)(insertion_index);
				insertion_index--;
			}
			else if constexpr (std::is_same_v<T, constant>)
			{
				if(std::holds_alternative<long long int>(node))
				{
					long long int value = std::get<long long int>(node);
					emplace_instruction(insertion_index, "PUSH " + std::to_string(value));
				}
				else
				{
					bool value = std::get<bool>(node);
					if(value)
					{
						emplace_instruction(insertion_index, "PUSH 0"); // THIS IS NOT A BUG. TRUE = 0
					}
					else
					{
						emplace_instruction(insertion_index, "PUSH 1");
					}
				}
			}
			else if constexpr (std::is_same_v<T, std::string>)
			{
				auto iter = variables.find(node);

				if(iter == std::end(variables))
				{
					std::cerr << "ERROR: Unused variable " + node + " inside of expression.\n";
					exit(1);
				}
				unsigned addr = iter->second;
				emplace_instruction(insertion_index, "PUSH $" + std::to_string(addr));
			}
		}, st);

		begin++;
		insertion_index++;
	}
	return insertion_index;
}

std::vector<std::string> translator::get_instructions()
{
	return std::vector<std::string>(std::begin(instructions), std::end(instructions));
}

void translator::add_non_terminals()
{
	non_terminal_chain.emplace(std::make_pair(instruction_type::AND,
		[this](unsigned index)
		{
			return non_terminal_chain.at(instruction_type::SUM)(index);
		}
	));
	non_terminal_chain.emplace(std::make_pair(instruction_type::OR,
		[this](unsigned index)
		{
			return non_terminal_chain.at(instruction_type::MULTIPLICATION)(index);
		}
	));
	non_terminal_chain.emplace(std::make_pair(instruction_type::NOT,
		[this](unsigned index)
		{
			emplace_instruction(index++, "JZ $3");
			emplace_instruction(index++, "PUSH 0");
			emplace_instruction(index++, "JMP $2");
			emplace_instruction(index++, "PUSH 1");
			emplace_instruction(index++, "NOP");
			return index;
		}
	));
	non_terminal_chain.emplace(std::make_pair(instruction_type::LT,
		[this](unsigned index)
		{
			index = non_terminal_chain.at(instruction_type::SUBTRACTION)(index);
			emplace_instruction(index++, "JGEZ $3");
			emplace_instruction(index++, "PUSH 0");
			emplace_instruction(index++, "JMP $2");
			emplace_instruction(index++, "PUSH 1");
			emplace_instruction(index++, "NOP");
			return index;
		}
	));
	non_terminal_chain.emplace(std::make_pair(instruction_type::LE,
		[this](unsigned index)
		{
			index = non_terminal_chain.at(instruction_type::SUBTRACTION)(index);
			emplace_instruction(index++, "JGZ $3");
			emplace_instruction(index++, "PUSH 0");
			emplace_instruction(index++, "JMP $2");
			emplace_instruction(index++, "PUSH 1");
			emplace_instruction(index++, "NOP");
			return index;
		}
	));
	non_terminal_chain.emplace(std::make_pair(instruction_type::EQ,
		[this](unsigned index)
		{
			index = non_terminal_chain.at(instruction_type::SUBTRACTION)(index);
			emplace_instruction(index++, "JNZ $3");
			emplace_instruction(index++, "PUSH 0");
			emplace_instruction(index++, "JMP $2");
			emplace_instruction(index++, "PUSH 1");
			emplace_instruction(index++, "NOP");
			return index;
		}
	));
	non_terminal_chain.emplace(std::make_pair(instruction_type::NE,
		[this](unsigned index)
		{
			index = non_terminal_chain.at(instruction_type::SUBTRACTION)(index);
			emplace_instruction(index++, "JZ $3");
			emplace_instruction(index++, "PUSH 0");
			emplace_instruction(index++, "JMP $2");
			emplace_instruction(index++, "PUSH 1");
			emplace_instruction(index++, "NOP");
			return index;
		}
	));
	non_terminal_chain.emplace(std::make_pair(instruction_type::GE,
		[this](unsigned index)
		{
			index = non_terminal_chain.at(instruction_type::SUBTRACTION)(index);
			emplace_instruction(index++, "JLZ $3");
			emplace_instruction(index++, "PUSH 0");
			emplace_instruction(index++, "JMP $2");
			emplace_instruction(index++, "PUSH 1");
			emplace_instruction(index++, "NOP");
			return index;
		}
	));
	non_terminal_chain.emplace(std::make_pair(instruction_type::GT,
		[this](unsigned index)
		{
			index = non_terminal_chain.at(instruction_type::SUBTRACTION)(index);
			emplace_instruction(index++, "JLEZ $3");
			emplace_instruction(index++, "PUSH 0");
			emplace_instruction(index++, "JMP $2");
			emplace_instruction(index++, "PUSH 1");
			emplace_instruction(index++, "NOP");
			return index;
		}
	));
	non_terminal_chain.emplace(std::make_pair(instruction_type::SUM,
		[this](unsigned index)
		{
			emplace_instruction(index++, "ADD");
			return index;
		}
	));
	non_terminal_chain.emplace(std::make_pair(instruction_type::SUBTRACTION,
		[this](unsigned index)
		{
			emplace_instruction(index++, "NEG");
			emplace_instruction(index++, "ADD");
			return index;
		}
	));
	non_terminal_chain.emplace(std::make_pair(instruction_type::MULTIPLICATION,
		[this](unsigned index)
		{
			emplace_instruction(index++, "MUL");
			return index;
		}
	));
	non_terminal_chain.emplace(std::make_pair(instruction_type::DIVISION,
		[this](unsigned index)
		{
			emplace_instruction(index++, "DIV");
			return index;
		}
	));
	non_terminal_chain.emplace(std::make_pair(instruction_type::MODUL,
		[this](unsigned index)
		{
			emplace_instruction(index++, "POP $0");
			emplace_instruction(index++, "DUP");
			emplace_instruction(index++, "PUSH $0");
			index = non_terminal_chain.at(instruction_type::DIVISION)(index);
			emplace_instruction(index++, "PUSH $0");
			index = non_terminal_chain.at(instruction_type::MULTIPLICATION)(index);
			index = non_terminal_chain.at(instruction_type::SUBTRACTION)(index);
			return index;
		}
	));
}

void translator::add_terminals()
{
	terminal_chain.emplace(std::make_pair(terminal_type::IF,
		[this](const unsigned index)
		{
			terminal& tl = std::get<terminal>(statements[index]);

			unsigned begin = index - (tl.bool_expr_length + tl.first_section_length);

			unsigned section_closure = next_terminal(index - tl.first_section_length, index);
			unsigned instruction_index = terminal_to_instruction.at(section_closure);

			emplace_instruction(std::size(instructions), "NOP");
			emplace_instruction(instruction_index, "JNZ $" + std::to_string(std::size(instructions) - instruction_index));

			parse_expr(begin, begin + tl.bool_expr_length, instruction_index);
		}
	));
	terminal_chain.emplace(std::make_pair(terminal_type::IF_ELSE,
		[this](const unsigned index)
		{
			terminal& tl = std::get<terminal>(statements[index]);

			unsigned begin = index - (tl.bool_expr_length + tl.first_section_length + tl.second_section_length);

			unsigned first_section_closure = next_terminal(index - tl.second_section_length - tl.first_section_length, index - tl.second_section_length);
			unsigned second_section_closure = next_terminal(index - tl.second_section_length, index);
			unsigned first_instruction_index = terminal_to_instruction.at(first_section_closure);
			unsigned second_instruction_index = terminal_to_instruction.at(second_section_closure);

			emplace_instruction(std::size(instructions), "NOP");
			emplace_instruction(second_instruction_index, "JMP $" + std::to_string(std::size(instructions) - second_instruction_index));
			emplace_instruction(first_instruction_index, "JNZ $" + std::to_string((second_instruction_index - first_instruction_index) + 2));

			parse_expr(begin, begin + tl.bool_expr_length, first_instruction_index);
		}
	));
	terminal_chain.emplace(std::make_pair(terminal_type::WHILE,
		[this](const unsigned index)
		{
			terminal& tl = std::get<terminal>(statements[index]);

			unsigned begin = index - (tl.bool_expr_length + tl.first_section_length);

			unsigned section_closure = next_terminal(index - tl.first_section_length, index);
			unsigned instruction_index = terminal_to_instruction.at(section_closure);

			emplace_instruction(instruction_index, "JMP $" + std::to_string(std::size(instructions) - instruction_index + 1));

			parse_expr(begin, begin + tl.bool_expr_length, std::size(instructions));

			emplace_instruction(std::size(instructions), "JZ $" + std::to_string(-long(std::size(instructions) - instruction_index - 1)));
		}
	));
	terminal_chain.emplace(std::make_pair(terminal_type::DO_WHILE,
		[this](const unsigned index)
		{
			terminal& tl = std::get<terminal>(statements[index]);

			unsigned begin = index - tl.bool_expr_length;

			unsigned section_closure = next_terminal(index - tl.bool_expr_length - tl.first_section_length, index - tl.bool_expr_length);
			unsigned instruction_index = terminal_to_instruction.at(section_closure);

			parse_expr(begin, begin + tl.bool_expr_length, std::size(instructions));

			emplace_instruction(std::size(instructions), "JZ $" + std::to_string(-long(std::size(instructions) - instruction_index)));
		}
	));
	terminal_chain.emplace(std::make_pair(terminal_type::ASSIGNMENT,
		[this](const unsigned index)
		{
			terminal& tl = std::get<terminal>(statements[index]);

			unsigned instruction_index = std::size(instructions);
			terminal_to_instruction.emplace(std::make_pair(index, instruction_index));

			unsigned section_length = tl.first_section_length;
			parse_expr(index - section_length, index - 1, std::size(instructions));

			const std::string& identifier = std::get<std::string>(statements[index - 1]);
			auto iterator = variables.find(identifier);
			unsigned addr = 0;

			if(iterator != std::end(variables))
			{
				addr = iterator->second;
			}
			else
			{
				addr = std::size(variables) + 1;
				variables.emplace(std::make_pair(identifier, addr));
			}

			emplace_instruction(std::size(instructions), "POP $" + std::to_string(addr));
		}
	));
	terminal_chain.emplace(std::make_pair(terminal_type::PRINT,
		[this](const unsigned index)
		{
			terminal& tl = std::get<terminal>(statements[index]);

			unsigned instruction_index = std::size(instructions);
			terminal_to_instruction.emplace(std::make_pair(index, instruction_index));

			unsigned section_length = tl.first_section_length;
			parse_expr(index - section_length, index, std::size(instructions));

			emplace_instruction(std::size(instructions), "PRINT");
		}
	));
	terminal_chain.emplace(std::make_pair(terminal_type::READ,
		[this](const unsigned index)
		{
			unsigned instruction_index = std::size(instructions);
			terminal_to_instruction.emplace(std::make_pair(index, instruction_index));

			const std::string& identifier = std::get<std::string>(statements[index - 1]);
			auto iterator = variables.find(identifier);
			unsigned addr = 0;

			if(iterator != std::end(variables))
			{
				addr = iterator->second;
			}
			else
			{
				addr = std::size(variables) + 1;
				variables.emplace(std::make_pair(identifier, addr));
			}

			emplace_instruction(std::size(instructions), "READ");
			emplace_instruction(std::size(instructions), "POP $" + std::to_string(addr));
		}
	));
	terminal_chain.emplace(std::make_pair(terminal_type::EXIT,
		[this](const unsigned index)
		{
			unsigned instruction_index = std::size(instructions);
			terminal_to_instruction.emplace(std::make_pair(index, instruction_index));
			emplace_instruction(std::size(instructions), "STOP");
		}
	));
	terminal_chain.emplace(std::make_pair(terminal_type::PASS,
		[this](const unsigned index)
		{
			unsigned instruction_index = std::size(instructions);
			terminal_to_instruction.emplace(std::make_pair(index, instruction_index));
			emplace_instruction(std::size(instructions), "NOP");
		}
	));
}

std::string translator::data_instruction()
{
	unsigned count = std::size(variables) + 1;

	std::string data = "DATA";

	for(unsigned i = 0; i < count; i++)
	{
		data += " 0";
	}

	return data;
}

unsigned translator::next_terminal(unsigned begin, unsigned end)
{
	while(begin != end)
	{
		if(std::holds_alternative<terminal>(statements[begin]))
		{
			return begin;
		}
		begin++;
	}
	return end;
}

void translator::emplace_instruction(unsigned index, const std::string& instruction)
{
	auto iterator = std::begin(instructions);
	std::advance(iterator, index);

	instructions.emplace(iterator, instruction);
}
