#include "iron/iron.h"

static inline bool is_pow2(usize n) {
    return (n & (n - 1)) == 0;
}

static inline usize align_forward_p2(usize value, usize align) {
    return (value + (align - 1)) & ~(align - 1);
}

FeStackItem* fe_stack_item_new(FeTy ty, FeComplexTy* cty) {
    FeStackItem* item = fe_zalloc(sizeof(FeStackItem));
    item->_offset = FE_STACK_OFFSET_UNDEF;
    item->ty = ty;
    item->complex_ty = cty;

    return item;
}

FeStackItem* fe_stack_remove(FeFunc* f, FeStackItem* item) {
    if (item->next) {
        item->next->prev = item->prev;
    } else {
        f->stack_top = item->prev;
    }

    if (item->prev) {
        item->prev->next = item->next;
    } else {
        f->stack_bottom = item->next;
    }

    item->next = nullptr;
    item->prev = nullptr;

    return item;
}

FeStackItem* fe_stack_append_bottom(FeFunc* f, FeStackItem* item) {
    if (f->stack_bottom == nullptr) {
        f->stack_bottom = item;
        f->stack_top = item;
        return item;
    }

    f->stack_bottom->prev = item;
    item->next = f->stack_bottom;
    f->stack_bottom = item;
    return item;
}

FeStackItem* fe_stack_append_top(FeFunc* f, FeStackItem* item) {
    if (f->stack_top == nullptr) {
        f->stack_bottom = item;
        f->stack_top = item;
        return item;
    }

    f->stack_top->next = item;
    item->prev = f->stack_top;
    f->stack_top = item;
    return item;
}

u32 fe_stack_calculate_size(FeFunc* f) {
    u32 stack_size = 0;

    FeStackItem* item = f->stack_bottom;
    while (item != nullptr) {
        usize size  = fe_ty_get_size(item->ty, item->complex_ty);
        usize align = fe_ty_get_align(item->ty, item->complex_ty);

        item->_offset = stack_size;
        stack_size = align_forward_p2(stack_size, align);
        stack_size += size;
        item = item->next;
    }

    stack_size = align_forward_p2(stack_size, f->mod->target->stack_pointer_align);
    return stack_size;
}
