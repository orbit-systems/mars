package phobos

lexer_info :: struct {
    
}

lexer_token :: struct {
    kind   : token_kind,
    lexeme : string,
    pos    : position,
}

position :: struct {
    file : string,
    row  : int,
    col  : int,
    line : int,
}

token_kind :: enum {
    invalid,
    EOF,

    identifier,     // bruh

    meta_literal_begin,
        literal_int,    // 123
        literal_float,  // 12.3
        literal_bool,   // true/false
        literal_string, // "bruh"
        literal_null,   // null
    meta_literal_end,

    meta_operator_begin,
        hash,           // #
        uninit,         // ---
        discard,        // _
        equal,          // =
        colon,          // :
        semicolon,      // ;
        period,         // .
        comma,          // ,
        exclam,         // !
        carat,          // ^
        add,            // +
        sub,            // -
        mul,            // *
        div,            // /
        mod,            // %
        mod_mod,        // %%
        tilde,          // ~
        and,            // &
        or,             // |
        nor,            // ~|
        lshift,         // <<
        rshift,         // >>

        and_and,        // &&
        or_or,          // ||
        tilde_tilde,    // ~~

        meta_arrow_begin,
            arrow_right,    // ->
            arrow_left,     // <-
            arrow_bidir,    // <->
        meta_arrow_end,
    
        meta_compound_assign_begin,
            add_equal,      // +=
            sub_equal,      // -=
            mul_equal,      // *=
            div_equal,      // /=
            mod_equal,      // %=
            mod_mod_equal,  // %%=

            and_equal,      // &=
            or_equal,       // |=
            nor_equal,      // ~|=
            xor_equal,      // ~=
            lshift_equal,   // <<=
            rshift_equal,   // >>=
        meta_compound_assign_end,

        meta_comparison_begin,
            equal_equal,    // ==
            not_equal,      // !=
            less_than,      // <
            less_equal,     // <=
            greater_than,   // >
            greater_equal,  // >=
        meta_comparison_end,

        open_paren,     // (
        close_paren,    // )
        open_brace,     // {
        close_brace,    // }
        open_bracket,   // [
        close_bracket,  // ]
    meta_operator_end,

    meta_keyword_begin,
        keyword_asm,
        keyword_bitcast,
        keyword_break,
        keyword_case,
        keyword_cast,
        keyword_defer,
        keyword_enum,
        keyword_elif,
        keyword_else,
        keyword_fallthrough,
        keyword_for,
        keyword_if,
        keyword_import,
        keyword_module,
        keyword_return,
        keyword_sizeof,
        keyword_struct,
        keyword_switch,
        keyword_union,
        keyword_while,
    meta_keyword_end,

}
