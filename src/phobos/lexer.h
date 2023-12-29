#pragma once
#define LEXER_H

#include "../orbit.h"
#include "../strslice.h"


typedef u8 enum {
    tt_invalid = 0,
    tt_EOF,

    tt_meta_identifer_begin,
        tt_identifier,         // bruh
        tt_identifier_discard, // _
    tt_meta_identifer_end,

    tt_meta_literal_begin,
        tt_literal_int,    // 123
        tt_literal_float,  // 12.3
        tt_literal_bool,   // true, false
        tt_literal_string, // "bruh"
        tt_literal_null,   // null
    tt_meta_literal_end,

    tt_meta_operator_begin,
        tt_hash,           // #
        tt_uninit,         // ---
        tt_equal,          // =
        tt_dollar,         // $
        tt_colon,          // :
        tt_colon_colon,    // ::
        tt_semicolon,      // ;
        tt_period,         // .
        tt_comma,          // ,
        tt_exclam,         // !
        tt_carat,          // ^
        tt_add,            // +
        tt_sub,            // -
        tt_mul,            // *
        tt_div,            // /
        tt_mod,            // %
        tt_mod_mod,        // %%
        tt_tilde,          // ~
        tt_and,            // &
        tt_or,             // |
        tt_nor,            // ~|
        tt_lshift,         // <<
        tt_rshift,         // >>

        tt_and_and,        // &&
        tt_or_or,          // ||
        tt_tilde_tilde,    // ~~

        tt_meta_arrow_begin,
            tt_arrow_right,    // ->
            tt_arrow_left,     // <-
            tt_arrow_bidir,    // <->
        tt_meta_arrow_end,
    
        tt_meta_compound_assign_begin,
            tt_add_equal,      // +=
            tt_sub_equal,      // -=
            tt_mul_equal,      // *=
            tt_div_equal,      // /=
            tt_mod_equal,      // %=
            tt_mod_mod_equal,  // %%=

            tt_and_equal,      // &=
            tt_or_equal,       // |=
            tt_nor_equal,      // ~|=
            tt_xor_equal,      // ~=
            tt_lshift_equal,   // <<=
            tt_rshift_equal,   // >>=
        tt_meta_compound_assign_end,

        tt_meta_comparison_begin,
            tt_equal_equal,    // ==
            tt_not_equal,      // !=
            tt_less_than,      // <
            tt_less_equal,     // <=
            tt_greater_than,   // >
            tt_greater_equal,  // >=
        tt_meta_comparison_end,

        tt_open_paren,     // (
        tt_close_paren,    // )
        tt_open_brace,     // {
        tt_close_brace,    // }
        tt_open_bracket,   // [
        tt_close_bracket,  // ]
    tt_meta_operator_end,

    tt_meta_keyword_begin,
        tt_keyword_let,
        tt_keyword_mut,
        tt_keyword_def,
        tt_keyword_type,

        tt_keyword_asm,
        tt_keyword_bitcast,
        tt_keyword_break,
        tt_keyword_case,
        tt_keyword_cast,
        tt_keyword_defer,
        tt_keyword_enum,
        tt_keyword_elif,
        tt_keyword_else,
        tt_keyword_extern,
        tt_keyword_fallthrough,
        tt_keyword_for,
        tt_keyword_fn,
        tt_keyword_if,
        tt_keyword_import,
        tt_keyword_module,
        tt_keyword_return,
        tt_keyword_struct,
        tt_keyword_switch,
        tt_keyword_union,
        tt_keyword_while,

        tt_keyword_typeof,
        tt_keyword_sizeof,
        tt_keyword_alignof,
        tt_keyword_offsetof,

    tt_meta_keyword_end,

    tt_meta_type_begin,
        tt_type_keyword_int,
        tt_type_keyword_i8,
        tt_type_keyword_i16,
        tt_type_keyword_i32,
        tt_type_keyword_i64,

        tt_type_keyword_uint,
        tt_type_keyword_u8,
        tt_type_keyword_u16,
        tt_type_keyword_u32,
        tt_type_keyword_u64,

        tt_type_keyword_bool,

        tt_type_keyword_float,
        tt_type_keyword_f16,
        tt_type_keyword_f32,
        tt_type_keyword_f64,

        tt_type_keyword_addr,
    tt_meta_type_end,
} token_type;

typedef struct token_s {
    string text;
    token_type type;
} token;

typedef struct lexer_state_s {
    
} lexer_state;