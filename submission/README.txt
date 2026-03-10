Author: Aviel Dahaman
Class: COP 3402, Summer 25
Assignment: Homework 4 Compiler
README

File names: 	
	hw4compiler.c
	vm.c

Description: This program combines a lexical analyzer, a syntax parser, and an intermediate code generator to create a PL/0 compiler. It processes the tokens produced by the lexical analyzer to ensure that the next token follows the specifications of the language's syntax. As it processes the syntax, it also generates the assembly language code for each statement. A human-readable assembly code and symbol table are output to the terminal and an assembly code made of integers (not binary) is output to a file named 'elf.txt'. The elf.txt file may be used as input for the VM program to execute the program written in PL/0.

HOW TO RUN:
First, compile the program from the command line with the command:
gcc hw4compiler.c

To run the compiled hw4compiler program: ./a.out input.txt

To get meaningful results, the input file should be the source code of a PL/0 program. You may enter a different input file name instead of "input.txt". However, the program must be run with a command line argument supplying the source code's file name.

For the VM program, compile it by using the command:
gcc vm.c -o vm

To run the compiled vm program, use command: ./vm elf.txt

The elf.txt file is created as the output of the hw4compiler program. You may rename it to anything else, but it must be run with the program from the command line.

HOW TO WRITE AN INPUT FILE:
The file must be written in PL/0 and conform to these standards: if the program has constant variables, they must be declared in a single constant declaration line. All other variables must be declared in a single variable declaration line. The program expects a single statement and then a period to end the program. The single statement may be a begin statement that allows multiple statements within the bracket formed by its matching end statement. The context free grammar that the compiler follows is:

program ::= block "." . 
block ::= const-declaration  var-declaration procedure-declaration  statement.	
constdeclaration ::= [ “const” ident "=" number {"," ident "=" number} “;"].	
var-declaration  ::= [ "var" ident {"," ident} “;"].
procedure-declaration ::= {"procedure" ident ";" block ";"}.
statement   ::= [ ident ":=" expression
	      	| "begin" statement { ";" statement } "end" 
	      	| "if condition "then" statement "else" statement "fi"
		| "when" condition "do" statement
		| "read" ident 
		| "write"  expression 
	      	| empty ] .  
condition ::= expression  rel-op  expression.  
rel-op ::= "="|“<>"|"<"|"<="|">"|">=“.
expression ::=  term { ("+"|"-") term}.
term ::= factor {("*"|"/"|"mod") factor}. 
factor ::= ident | number | "(" expression ")“.
number ::= digit {digit}.
ident ::= letter {letter | digit}.
digit ;;= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9“.
letter ::= "a" | "b" | … | "y" | "z" | "A" | "B" | ... |"Y" | "Z".

 
Based on Wirth’s definition for EBNF we have the following rule:
[ ] means an optional item.
{ } means repeat 0 or more times.
Terminal symbols are enclosed in quote marks.
A period is used to indicate the end of the definition of a syntactic class.
