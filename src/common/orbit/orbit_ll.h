#pragma once

#include <stdlib.h>
#include <assert.h>

// doubly-linked list library in the style of orbit_da.

#define ll(T) _ll_##T

typedef struct _ll_ {
    struct _ll_* next;
    struct _ll_* prev;
} _ll_;

#define ll_typedef(T) typedef struct _ll_##T { \
    struct _ll_##T *next;\
    struct _ll_##T *prev;\
    T this;\
} _ll_##T

#define ll_new(T) ((_ll_##T *) calloc(1, sizeof(_ll_##T)))

// insert new_ptr after node_ptr
#define ll_insert_next(node_ptr, new_ptr) do {\
    __typeof__(node_ptr) _node_ptr = (node_ptr); \
    __typeof__(new_ptr) _new_ptr  = (new_ptr); \
    _new_ptr->prev = _node_ptr; \
    _new_ptr->next = _node_ptr->next; \
    if (_node_ptr->next != NULL) \
        _node_ptr->next->prev = _new_ptr;\
    _node_ptr->next = _new_ptr;\
} while (0)

// insert new_ptr before node_ptr
#define ll_insert_prev(node_ptr, new_ptr) do {\
    __typeof__(node_ptr) _node_ptr = (node_ptr); \
    __typeof__(new_ptr) _new_ptr  = (new_ptr); \
    _new_ptr->next = _node_ptr; \
    _new_ptr->prev = _node_ptr->prev; \
    if (_node_ptr->prev != NULL) \
        _node_ptr->prev->next = _new_ptr;\
    _node_ptr->prev = _new_ptr;\
} while (0)

// insert new_ptr at the front of the list
#define ll_push_front(list_ptr, new_ptr) do {\
    __typeof__(list_ptr) _list_ptr  = (list_ptr); \
    __typeof__(new_ptr) _new_ptr  = (new_ptr); \
    while (_list_ptr->prev != NULL) _list_ptr = _list_ptr->prev; \
    _list_ptr->prev = _new_ptr; \
    _new_ptr->next = _list_ptr; \
} while (0)

// insert new_ptr at the back of the list
#define ll_push_back(list_ptr, new_ptr) do {\
    __typeof__(list_ptr) _list_ptr  = (list_ptr); \
    __typeof__(new_ptr) _new_ptr  = (new_ptr); \
    while (_list_ptr->next != NULL) _list_ptr = _list_ptr->next; \
    _list_ptr->next = _new_ptr; \
    _new_ptr->prev = _list_ptr; \
} while (0)

// removes a node from the list
#define ll_remove(node_ptr) do {\
    if ((node_ptr)->next != NULL) (node_ptr)->next->prev = (node_ptr)->prev;\
    if ((node_ptr)->prev != NULL) (node_ptr)->prev->next = (node_ptr)->next;\
} while (0)

#define ll_remove_free(list_ptr) do {\
    if ((node_ptr)->next != NULL) (node_ptr)->next->prev = (node_ptr)->prev;\
    if ((node_ptr)->prev != NULL) (node_ptr)->prev->next = (node_ptr)->next;\
    free(list_ptr);\
} while (0)
