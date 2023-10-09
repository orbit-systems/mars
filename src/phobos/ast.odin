package phobos

// complete program AST - this is what phobos should pass to deimos
// contains all packages and other information
program_tree :: struct {
    modules : [dynamic]^module
}

new_program :: proc() -> (prog: ^program_tree) {
    prog = new(program_tree)
    prog.modules = make([dynamic]^module)

    return
}

module :: struct {
    program  : ^program_tree, // what program is this module contained in?
    files    : [dynamic]^file,

    global_scope : ^scope_meta,

    name     : string, // internal name. this is the name used in the module declarations.
    path     : string,
}

new_module :: proc(program: ^program_tree, path: string) -> (mod: ^module) {
    mod = new(module)
    mod.program = program
    append(&(program.modules), mod)
    mod.global_scope = new_scope(nil, true)
    mod.path = path

    return
}

file :: struct {
    module   : ^module,
    path     : string,
    src      : string,
    imported : [dynamic]int, // indexes into the program_ast modules list
    stmts    : [dynamic]AST,
}

new_file :: proc(mod: ^module, path, src: string) -> (f: ^file) {
    f = new(file)
    f.module = mod
    f.path = path
    f.src = src
    return
}

add_global_stmt :: proc(f: ^file, stmt: AST) {
    append(&(f.stmts), stmt)
}

AST :: union {

    ^scope_meta, // points to a new scope

    ^module_decl_stmt,       // module bruh;
    ^external_stmt,          // external {};

    ^decl_stmt,              // a : int; a : int = 0; a : int = ---;
    ^assign_stmt,            // a = 1 + 2;
    //^compound_assign_stmt,   // a += 3;

    ^expr_stmt,              // (expression); often results in an unused expression error.
    //^call_stmt,              // func();
    ^stmt_group_stmt,        // groups statements together

    //^if_stmt,
    //^while_stmt,
    //^for_stmt
    //^switch_stmt,
    //^case_clause_stmt,

    ^basic_type,

    ^array_type,
    ^slice_type,
    ^pointer_type,
    ^funcptr_type,
    ^struct_type,
    ^union_type,
    ^enum_type,



    ^ident_expr,             // points to entity
    ^basic_literal_expr,

    

    // ! compound_literal_expr,
    // ! enum_literal_expr,

    ^paren_expr,
    ^op_unary_expr,
    ^op_binary_expr,

    ^cast_expr,
    ^bitcast_expr,
    ^len_expr,
    ^base_expr,
    ^sizeof_expr,
    
    //struct_selector_expr,
    //union_selector_expr,
    //enum_variant_expr,
    //module_selector_expr,
    ^selector_expr,
    ^array_index_expr,

    ^call_expr,



}

// ties the scope tree to the abstract syntax tree
scope_meta :: struct {

    members : [dynamic]^entity, // entities in this scope level (does not include superscope entities)
    superscope : ^scope_meta,
    subscopes  : [dynamic]^scope_meta,
    is_global : bool,

    next : AST,
}

new_scope :: proc(super: ^scope_meta, is_global := false) -> (scope: ^scope_meta) {
    scope = new(scope_meta)
    scope.superscope = super
    scope.is_global = is_global
    return
}

entity :: struct {
    scope       : ^scope_meta,
    identifier  : string,
    declaration : AST,
    type        : AST,
    is_library  : bool,
    is_const    : bool,
}

module_decl_stmt :: struct {
    start, end: ^lexer_token,
}

new_module_decl_stmt :: proc(s, e: ^lexer_token) -> (node: ^module_decl_stmt) {
    node = new(module_decl_stmt)
    node.start = s
    node.end = e
    return
}

external_stmt :: struct {
    decls      : [dynamic]AST,
    external   : ^lexer_token,
    start, end : ^lexer_token,
}

stmt_group_stmt :: struct {
    stmts : [dynamic]AST,
    start, end: ^lexer_token,
}

expr_stmt :: struct {
    exprs : [dynamic]AST,
}

decl_stmt :: struct {
    lhs, types, rhs : [dynamic]AST,
    is_constant : bool,
}

assign_stmt :: struct {
    lhs, rhs : [dynamic]AST,
}

compound_assign_stmt :: struct {
    lhs, rhs : [dynamic]AST,
    op : ^lexer_token,
}

basic_literal_expr :: struct {
    type  : AST,
    tok   : ^lexer_token,
}

compound_literal_expr :: struct {
    type: AST,
    // TODO
}

proc_literal_expr :: struct {

}

enum_literal_expr :: struct {

}

basic_type :: enum u8 {
    invalid = 0,
    none,       // this is what functions without a return type return. this is also what statements "return".
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

    mars_addr,

    untyped_int,
    untyped_float,
    untyped_bool,
    untyped_null,

    internal_lib,
    internal_alias,
}

array_type :: struct {
    length     : AST,
    entry_type : AST,
}

slice_type :: struct {
    entry_type : AST,
}

pointer_type :: struct {
    entry_type : AST,
}

funcptr_type :: struct {
    param_field_idents  : [dynamic]string,
    param_field_types   : [dynamic]AST,
    return_field_idents : [dynamic]string,
    return_field_types  : [dynamic]AST,
    positional          : bool,
}

struct_type :: struct {
    field_idents  : [dynamic]string,
    field_offsets : [dynamic]int,
    field_types   : [dynamic]AST,
}

union_type :: struct {
    field_idents : [dynamic]string,
    field_types  : [dynamic]AST,
}

enum_type :: struct {
    backing_type   : AST,
    variant_idents : [dynamic]string,
    variant_vals   : [dynamic]AST,
}

ident_expr :: struct {
    ident  : string,
    entity : ^entity,
    tok    : ^lexer_token,
    is_discard : bool,
}

paren_expr :: struct {
    child : AST,
    open, close : ^lexer_token,
}

op_unary_expr :: struct {
    op    : ^lexer_token,
    child : AST,
}

op_binary_expr :: struct {
    op       : ^lexer_token,
    lhs, rhs : AST,
}

cast_expr :: struct {
    op      : ^lexer_token,
    cast_to : AST, // type expression
    child   : AST, // what to cast
}

bitcast_expr :: struct {
    op      : ^lexer_token,
    cast_to : AST, // type expression
    child   : AST, // what to cast
}

len_expr :: struct {
    op    : ^lexer_token,
    child : AST,
}

base_expr :: struct {
    op    : ^lexer_token,
    child : AST,
}

sizeof_expr :: struct {
    op    : ^lexer_token,
    child : AST,
}

array_index_expr :: struct {
    lhs         : AST,
    index       : AST,
    open, close : ^lexer_token,
}

call_expr :: struct {
    lhs         : AST,
    args        : [dynamic]AST,
    open, close : ^lexer_token,
}

selector_expr :: struct {
    source   : AST,
    op       : ^lexer_token,
    selector : AST,
    implicit : bool,
}