package phobos

AST :: union {



   //module_meta,

   //module_decl_stmt,       // module bruh;
   //import_decl_stmt,       // x :: import "bruh";
   //external_block_stmt,    // external {} ;

   //var_decl_stmt,          // a : int, a : int = 0, a : int = ---;
   //const_decl_stmt,        // c :: 1, c : int : 1;
   //assign_stmt,            // a = 1 + 2;
   //compound_assign_stmt,   // a += 3;

   //expr_stmt,              // (expression); often results in an unused expression error.
   //call_stmt,              // funct();
   //stmt_group_stmt,        // groups statements together, often paired with the new_scope_stmt statement.




   AST_type_expr,              // like 'int', 'f32', or a compound type like `struct{a:int, b: float}`

   //ident_expr,             // identifier
   //literal_expr,           // 1, 2.0, false, null : literal value

   //struct_field_expr,      
   //union_field_expr,
   //enum_variant_expr,
   //array_access_expr,

   //op_unary_expr,
   //op_binary_expr,

   //call_expr,





}

AST_type_expr :: union {
    AST_basic_type_expr,

    AST_array_type_expr,
    AST_slice_type_expr,
    AST_pointer_type_expr,

    AST_struct_type_expr,
    AST_union_type_expr,
    AST_enum_type_expr,

}

AST_basic_type_expr :: enum {
    invalid = 0,
    undetermined,
    none,

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

AST_array_type_expr :: struct {
    length     : int,
    entry_type : ^AST,
}

AST_slice_type_expr :: struct {
    entry_type : ^AST,
}

AST_pointer_type_expr :: struct {
    entry_type : ^AST,
}

AST_struct_type_expr :: struct {
    entry_field_idents : []string,
    entry_field_types  : []^AST,
}

AST_union_type_expr :: struct {
    entry_field_idents : []string,
    entry_field_types  : []^AST,
}

AST_enum_type_expr :: struct {
    backing_type       : ^AST,
    entry_field_idents : []string,
}




// determine if an expression is constant or can be evaluated at compile time
is_constant_expr :: proc(expr : ^AST) -> bool {
    TODO("(sandwich) is_constant_expr")
    return false
}