CC=g++
FLAGS=-std=c++1z

lex.yy.c: lexer.l
	flex lexer.l

grammar.tab.c grammar.tab.h: grammar.y language_definition.h translator.h
	bison -d grammar.y

main: grammar.tab.c grammar.tab.h lex.yy.c translator.cpp
	$(CC) $(FLAGS) -g grammar.tab.c lex.yy.c translator.cpp -lfl -o main

clean:
	rm grammar.tab.*
	rm lex.yy.c
	rm main
