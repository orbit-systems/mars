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

	tokens: [dynamic]lexer_token

	for i:=0; i < 500; i+=1 {
		int_token := lex_next_token(&lexer_context)
		append(&tokens, int_token)
		if int_token.kind == .EOF {
			break;
		}
	}

	for token in tokens {
		fmt.printf("{}," token.lexeme)
	}
	fmt.printf("\n")

	//parse

	//semen lmao

	//pass to deimos
}