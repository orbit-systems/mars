#include "orbit.h"
#include "lexer.h"
#include "term.h"
#include "arena.h"
#include "type.h"

size_t mt_kind_size[] = {
    0,
#define TYPE(ident, identstr, structdef) sizeof(mtype_##ident),
    TYPE_NODES
#undef TYPE
    0
};

char* mt_kind_str[] = {
    "invalid",
#define TYPE(ident, identstr, structdef) identstr,
    TYPE_NODES
#undef TYPE
    "COUNT",
};

// allocate and zero a new AST node with an arena
mars_type new_type_node(arena* restrict a, mt_kind type) {
    mars_type node;
    void* node_ptr = arena_alloc(a, mt_kind_size[type], 8); // default align to 8
    if (node_ptr == NULL) {
        general_error("new_type_node() could not allocate type node of type '%s' with size %d", mt_kind_str[type], mt_kind_size[type]);
    }
    memset(node_ptr, 0, mt_kind_size[type]);
    node.rawptr = node_ptr;
    node.type = type;
    return node;
}

// counts on the type not being cyclic/infinite size
size_t align_of_type(mars_type t) {
    switch (t.type) {
    case mt_basic_u8:
    case mt_basic_i8:
    case mt_basic_bool:
        return 1;
    case mt_basic_u16:
    case mt_basic_i16:
    case mt_basic_f16: 
        return 2;
    case mt_basic_u32:
    case mt_basic_i32:
    case mt_basic_f32: 
        return 4;
    case mt_basic_u64:
    case mt_basic_i64:
    case mt_basic_f64: 
    case mt_pointer:
    case mt_slice:
        return 8;
    case mt_array:
        return align_of_type(t.as_array->subtype);
    case mt_struct: {
        size_t max_align = 1;
        FOR_URANGE_EXCL(i, 0, t.as_struct->fields.len) {
            max_align = max(max_align, align_of_type(t.as_struct->fields.at[i].type));
        }
        return max_align;
    }
    case mt_union:{
        size_t max_align = 1;
        FOR_URANGE_EXCL(i, 0, t.as_union->fields.len) {
            max_align = max(max_align, align_of_type(t.as_union->fields.at[i].type));
        }
        return max_align;
    }
    case mt_alias:
        return size_of_type(t.as_alias->subtype);
    default:
        general_error("internal: invalid type '%s' (%d) passed to align_of_type()", mt_kind_str[t.type] , t.type);
        break;
    }
    return 0;
}

// counts on the type not being cyclic/infinite size
size_t size_of_type(mars_type t) {
    switch (t.type) {
    case mt_basic_u8:
    case mt_basic_i8:
    case mt_basic_bool:
        return 1;
    case mt_basic_u16:
    case mt_basic_i16:
    case mt_basic_f16: 
        return 2;
    case mt_basic_u32:
    case mt_basic_i32:
    case mt_basic_f32: 
        return 4;
    case mt_basic_u64:
    case mt_basic_i64:
    case mt_basic_f64: 
    case mt_pointer:
        return 8;
    case mt_slice:
        return 16;
    case mt_array: {
        // size + inter-member padding
        size_t real_size = size_of_type(t.as_array->subtype);
        size_t functional_size = align_forward(real_size, align_of_type(t.as_array->subtype));
        return functional_size*(t.as_array->length-1) + real_size;
    }
    case mt_struct: {
        size_t max_offset = 0;
        size_t max_offset_size = 0;
        FOR_URANGE_EXCL(i, 0, t.as_struct->fields.len) {
            if (t.as_struct->fields.at[i].offset > max_offset) {
                max_offset = t.as_struct->fields.at[i].offset;
                max_offset_size = size_of_type(t.as_struct->fields.at[i].type);
            }
        }
        return max_offset + max_offset_size;
    }
    case mt_union:{
        size_t max_size = 0;
        FOR_URANGE_EXCL(i, 0, t.as_union->fields.len) {
            max_size = max(max_size, size_of_type(t.as_union->fields.at[i].type));
        }
        return max_size;
    }
    case mt_alias:
        return size_of_type(t.as_alias->subtype);
    default:
        general_error("internal: invalid type '%s' (%d) passed to size_of_type()", mt_kind_str[t.type] , t.type);
        return 0;
    }
}

// SLOW AND LEAKS A SHIT TON OF MEMORY
// THIS SHOULD ONLY BE USED FOR ERRORS 
// AND WHERE LEAKING MEMORY DOESNT MATTER
string type_to_str(mars_type t) {
    string s = to_string("");
    switch (t.type) {
    case mt_basic_none: return to_string("none");

    case mt_untyped_bool:    return to_string("untyped bool");
    case mt_untyped_float:   return to_string("untyped float");
    case mt_untyped_int:     return to_string("untyped int");
    case mt_untyped_string:  return to_string("untyped string");
    case mt_untyped_pointer: return to_string("untyped pointer");

    case mt_basic_u8:   return to_string("u8");
    case mt_basic_u16:  return to_string("u16");
    case mt_basic_u32:  return to_string("u32");
    case mt_basic_u64:  return to_string("u64");
    case mt_basic_i8:   return to_string("i8");
    case mt_basic_i16:  return to_string("i16");
    case mt_basic_i32:  return to_string("i32");
    case mt_basic_i64:  return to_string("i64");
    case mt_basic_f16:  return to_string("f16");
    case mt_basic_f32:  return to_string("f32");
    case mt_basic_f64:  return to_string("f64");
    case mt_basic_addr: return to_string("addr");
    case mt_basic_bool: return to_string("bool");
    case mt_alias:      return t.as_alias->name;
    case mt_multi:
        s = to_string("(");
        FOR_URANGE_EXCL(i, 0, t.as_multi->subtypes.len) {
            s = string_concat(s, type_to_str(t.as_multi->subtypes.at[i]));
            if (i != t.as_multi->subtypes.len - 1) {
                s = string_concat(s, to_string(", "));
            }
        }
        return string_concat(s, to_string(")"));
    case mt_slice:
        return string_concat(to_string("[]"), type_to_str(t.as_slice->subtype));
    case mt_pointer:
        return string_concat(to_string("^"), type_to_str(t.as_slice->subtype));
    case mt_array:
        return string_concat(strprintf("[%ull]", t.as_array->length), type_to_str(t.as_array->subtype));
    case mt_struct:
        s = to_string("union {");
        FOR_URANGE_EXCL(i, 0, t.as_struct->fields.len) {
            s = string_concat(s, t.as_struct->fields.at[i].ident);
            s = string_concat(s, to_string(": "));
            s = string_concat(s, type_to_str(t.as_struct->fields.at[i].type));
            if (i != t.as_multi->subtypes.len - 1) {
                s = string_concat(s, to_string(", "));
            }
        }
        return string_concat(s, to_string("}"));
    case mt_union:
        s = to_string("struct {");
        FOR_URANGE_EXCL(i, 0, t.as_struct->fields.len) {
            s = string_concat(s, t.as_struct->fields.at[i].ident);
            s = string_concat(s, to_string(": "));
            s = string_concat(s, type_to_str(t.as_struct->fields.at[i].type));
            if (i != t.as_multi->subtypes.len - 1) {
                s = string_concat(s, to_string(", "));
            }
        }
        return string_concat(s, to_string("}"));
    default:
        return to_string("<INVALID TYPE>");
    }
}