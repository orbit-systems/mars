package phobos

// complete program AST - this is what phobos should pass to deimos
// contains all packages and other information
program_AST :: struct {
    modules : [dynamic]^module_AST
}

//
module_AST :: struct {
    program : ^program_AST, // what program is this module contained in?
    files   : [dynamic]^file_AST,   //
    name    : string, // internal name. this is the name used in the module declarations.
}


file_AST :: struct {
    module   : ^module_AST,
    path     : string,
    imported : [dynamic]int, // indexes into the program_ast modules list
    root     : AST, // stmt_group_stmt
}

AST :: union {

    ^module_decl_stmt,       // module bruh;
    //^external_stmt,          // external {};

    //^decl_stmt,              // a : int, a : int = 0, a : int = ---;
    //^assign_stmt,            // a = 1 + 2;
    //^compound_assign_stmt,   // a += 3;

    //^expr_stmt,              // (expression); often results in an unused expression error.
    //^call_stmt,              // func();
    ^stmt_group_stmt,        // groups statements together

    //^if_stmt,
    //^while_stmt,
    //^for_stmt
    //^switch_stmt,
    //^case_clause_stmt,

    ^basic_type_expr,

    ^array_type_expr,
    ^slice_type_expr,
    ^pointer_type_expr,
    ^funcptr_type_expr,

    ^struct_type_expr,
    ^union_type_expr,
    ^enum_type_expr,

    //ident_expr,             // points to entity
    //literal_expr,           // literal value expression
    // ! basic_literal_expr,
    // ! compound_literal_expr,
    // ! enum_literal_expr,

    //paren_expr,
    //op_unary_expr,
    //op_binary_expr,
    
    //selector_expr,
    //struct_selector_expr,
    //union_selector_expr,
    //enum_variant_expr,
    //module_selector_expr,
    //array_index_expr,

    //call_expr,



}

entity :: struct {
    // scope:
    ident : string,
}

new_entity :: proc(name: string) -> ^entity {
    e := new(entity)
    e.ident = name
    return e
}

module_decl_stmt :: struct {
    name_entity : ^entity
}

stmt_group_stmt :: struct {
    stmts: [dynamic]AST
}

basic_literal_expr :: struct {
    // TODO
    type  : AST,
    token : ^lexer_token,
}

compound_literal_expr :: struct {

    // TODO
}

proc_literal_expr :: struct {

}

enum_literal_expr :: struct {

}

basic_type_expr :: enum {
    invalid = 0,
    none,
    unresolved, // type probably exists but hasn't been derived yet - probably used in an implicit type

    mars_i8,
    mars_i16,
    mars_i32,
    mars_i64,

    mars_u8,
    mars_u16,
    mars_u32,
    mars_u64,

    mars_b8,
    mars_b16,
    mars_b32,
    mars_b64,

    mars_f16,
    mars_f32,
    mars_f64,

    mars_rawptr,

    untyped_int,
    untyped_float,
    untyped_bool,
    untyped_null,
}

array_type_expr :: struct {
    length     : int,
    entry_type : AST,
}

slice_type_expr :: struct {
    entry_type : AST,
}

pointer_type_expr :: struct {
    entry_type : AST,
}

funcptr_type_expr :: struct {
    param_field_idents  : [dynamic]string,
    param_field_types   : [dynamic]AST,
    return_field_idents : [dynamic]string,
    return_field_types  : [dynamic]AST,
    positional          : bool,
}

struct_type_expr :: struct {
    field_idents  : [dynamic]string,
    field_offsets : [dynamic]int,
    field_types   : [dynamic]AST,
}

union_type_expr :: struct {
    field_idents : [dynamic]string,
    field_types  : [dynamic]AST,
}

enum_type_expr :: struct {
    backing_type   : AST,
    variant_idents : [dynamic]string,
    variant_vals   : [dynamic]AST,
}