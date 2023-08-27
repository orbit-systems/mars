package phobos

// mars compiler frontend - lexer, parser
// produces abstract syntax tree to be passed to deimos backend

//        lexer            parser         checker
// file --------> tokens ---------> AST ----------> rich AST (with type info, etc.)
