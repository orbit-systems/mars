package phobos

// mars compiler frontend - lexer, parser
// produces abstract syntax tree to be passed to deimos backend

//        lexer            parser                seman
// file --------> tokens ---------> parse tree ----------> AST

process_file :: proc(file_name: string, file_data: string) {
	//lex

	lexer_context: lexer_info
	lexer_context.file_name = file_name
	lexer_context.file_data = file_data

	lex_next_token(&lexer_context)

	//parse

	//semen lmao

	//pass to deimos
}