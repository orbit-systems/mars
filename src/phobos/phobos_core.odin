package phobos

import "core:fmt"
import "core:time"

// mars compiler frontend - lexer, parser
// produces abstract syntax tree to be passed to deimos backend

//        lexer            parser                seman
// file --------> tokens ---------> parse tree ----------> AST

process_file :: proc(path: string, file_data: string) {

	//lex

	overall_timer : time.Stopwatch

	lexer_context: lexer_info
	lexer_init(&lexer_context, path, file_data)

	int_token : lexer_token
	count : u64 = 0
	time.stopwatch_start(&overall_timer)
	for count = 0; int_token.kind != .EOF; count += 1 {
		int_token = lex_next_token(&lexer_context)
		//fmt.printf("%v ", int_token.lexeme)
	}
	time.stopwatch_stop(&overall_timer)
	time_to_lex := time.duration_seconds(time.stopwatch_duration(overall_timer))
	fmt.printf("%d tokens parsed\n", count)
	fmt.printf("%d lines parsed\n", lexer_context.pos.line)
	fmt.printf("in %v seconds\n", time_to_lex)


	//parse

	//semen lmao

	//pass to deimos
}