package phobos

lexer_info :: struct {
    
}

lexer_token :: struct {
    kind   : token_kind,
    lexeme : string,
    pos    : position_in_file,
}

position_in_file :: struct {
    row  : int,
    col  : int,
    line : int,
}

token_kind :: enum {
    invalid,
    EOF,
    comment,
    
    meta_literal_begin,
        identifier,     // bruh
        literal_int,    // 123
        literal_float,  // 12.3
        literal_bool,   // true/false
        literal_string, // "bruh"
        literal_null,   // null
    meta_literal_end,

    meta_operator_begin,
        equal,  // =
        exclam, // !
        carat,  // ^
        add,
        sub,
        mul,
        div,
        mod,
        mod_mod,
        tilde,
        and,
        or,
        nor,
        lshift,
        rshift,
        and_and,
        or_or,
        tilde_tilde,

}
