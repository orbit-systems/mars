#include "orbit.h"
#include "lexer.h"
#include "error.h"
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

// allocate and zero a new AST node with an arena_list
mars_type new_type_node(arena_list* restrict al, mt_kind type) {
    mars_type node;
    void* node_ptr = arena_list_alloc(al, mt_kind_size[type], 8);
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
            max_align = max(max_align, align_of_type(t.as_struct->fields.raw[i].type));
        }
        return max_align;
    }
    case mt_union:{
        size_t max_align = 1;
        FOR_URANGE_EXCL(i, 0, t.as_union->fields.len) {
            max_align = max(max_align, align_of_type(t.as_union->fields.raw[i].type));
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
            if (t.as_struct->fields.raw[i].offset > max_offset) {
                max_offset = t.as_struct->fields.raw[i].offset;
                max_offset_size = size_of_type(t.as_struct->fields.raw[i].type);
            }
        }
        return max_offset + max_offset_size;
    }
    case mt_union:{
        size_t max_size = 0;
        FOR_URANGE_EXCL(i, 0, t.as_union->fields.len) {
            max_size = max(max_size, size_of_type(t.as_union->fields.raw[i].type));
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