#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "parse.h"
#include "common/str.h"
#include "common/strmap.h"
#include "common/util.h"
#include "common/vec.h"
#include "coyote.h"
#include "lex.h"

Vec_typedef(char);

static void vec_char_append_str(Vec(char)* vec, const char* data) {
    usize len = strlen(data);
    for_n(i, 0, len) {
        vec_append(vec, data[i]);
    }
}

static void vec_char_append_many(Vec(char)* vec, const char* data, usize len) {
    for_n(i, 0, len) {
        vec_append(vec, data[i]);
    }
}

thread_local static struct {
    TyBufSlot* at;
    TyIndex* ptrs;
    u32 len;
    u32 cap;
} tybuf = {nullptr, nullptr, 0, 0};

#define TY(index, T) ((T*)&tybuf.at[index])
#define TY_KIND(index) ((TyBase*)&tybuf.at[index])->kind

void ty_init() {
    tybuf.len = 0;
    if (tybuf.at == nullptr) {
        tybuf.cap = 1024;
        tybuf.at = malloc(sizeof(tybuf.at[0]) * tybuf.cap);
        tybuf.ptrs = malloc(sizeof(tybuf.ptrs[0]) * tybuf.cap);
    }
    memset(tybuf.ptrs, 0, sizeof(tybuf.ptrs[0]) * tybuf.cap);
    
    for_n_eq(i, TY_VOID, TY_UQUAD) {
        TY(i,  TyBase)->kind = i;

        // init pointer cache
        TY(i + TY_PTR,  TyPtr)->kind = TY_PTR;
        TY(i + TY_PTR,  TyPtr)->to = i;
        tybuf.ptrs[i] = i + TY_PTR;
    }
    tybuf.len = TY_UQUAD + TY_PTR + 1;
}

#define ty_allocate(T) ty__allocate(sizeof(T), alignof(T) == 8)
static TyIndex ty__allocate(usize size, bool align64) {
    usize old_len = tybuf.len;

    usize slots = size / sizeof(TyBufSlot);
    if (align64 && (tybuf.len & 1)) {
        tybuf.len += 1; // pad to 64
    }
    
    if_unlikely (tybuf.len + slots > tybuf.cap) {
        tybuf.cap += tybuf.cap << 1;
        tybuf.at = realloc(tybuf.at, sizeof(tybuf.at[0]) * tybuf.cap);
        tybuf.ptrs = realloc(tybuf.ptrs, sizeof(tybuf.ptrs[0]) * tybuf.cap);
        memset(&tybuf.ptrs[old_len], 0, sizeof(tybuf.ptrs[0]) * (tybuf.cap - old_len));
    }

    usize pos = tybuf.len;
    tybuf.len += slots;
    return pos;
}

static TyIndex ty_get_ptr_target(TyIndex ptr) {
    return TY(ptr, TyPtr)->to;
}

static TyIndex ty_get_ptr(TyIndex t) {
    if (t < TY_PTR) {
        return t + TY_PTR;
    }
    if (tybuf.ptrs[t] == 0) {
        // we have to create a pointer type since none exists.
        TyIndex ptr = ty_allocate(TyPtr);
        TY(ptr, TyPtr)->kind = TY_PTR;
        TY(ptr, TyPtr)->to = t;
        
        tybuf.ptrs[t] = ptr;
        return ptr;
    } else {
        return tybuf.ptrs[t];
    }
}

// gross
thread_local ParseScope* global_scope;

static TyIndex ty_unwrap_alias(TyIndex t) {
    while (TY_KIND(t) == TY_ALIAS) {
        t = TY(t, TyAlias)->aliasing;
    }
    return t;
}

static TyIndex ty_unwrap_alias_or_enum(TyIndex t) {
    while (true) {
        switch (TY_KIND(t)) {
        case TY_ALIAS:
            t = TY(t, TyAlias)->aliasing;
            break;
        case TY_ENUM:
            t = TY(t, TyEnum)->backing_ty;
            break;
        default:
            return t;
        }
    }
    return t;
}

static void vec_char_print_num(Vec(char)* v, u64 val) {
    if (val < 10) {
        vec_append(v, '0' + val);
    } else {
        vec_char_print_num(v, val / 10);
        vec_append(v, '0' + val % 10);
    }
}

static void _ty_name(Vec(char)* v, TyIndex t) {
    switch (TY_KIND(t)) {
    case TY__INVALID: vec_char_append_str(v, "!!!"); return;
    case TY_VOID:     vec_char_append_str(v, "VOID"); return;
    case TY_BYTE:     vec_char_append_str(v, "BYTE"); return;
    case TY_UBYTE:    vec_char_append_str(v, "UBYTE"); return;
    case TY_INT:      vec_char_append_str(v, "INT"); return;
    case TY_UINT:     vec_char_append_str(v, "UINT"); return;
    case TY_LONG:     vec_char_append_str(v, "LONG"); return;
    case TY_ULONG:    vec_char_append_str(v, "ULONG"); return;
    case TY_QUAD:     vec_char_append_str(v, "QUAD"); return;
    case TY_UQUAD:    vec_char_append_str(v, "UQUAD"); return;
    case TY_PTR:
        vec_char_append_str(v, "^");
        _ty_name(v, ty_get_ptr_target(t));
        return;
    case TY_ARRAY:
        _ty_name(v, TY(t, TyArray)->to);
        vec_append(v, '[');
        vec_char_print_num(v, TY(t, TyArray)->len);
        vec_append(v, ']');
        return;
    case TY_ALIAS:
    case TY_ALIAS_INCOMPLETE:
        ;
        CompactString compact_name = TY(t, TyAlias)->entity->name;
        string name = from_compact(compact_name);
        vec_char_append_many(v, name.raw, name.len);
        return;
    case TY_FN: {
        vec_char_append_str(v, "FN(");
        TyFn* fn = TY(t, TyFn);
        for_n(i, 0, fn->len - 1) {
            Ty_FnParam* param = &fn->params[i];
            vec_char_append_str(v, param->out ? "OUT " : "");
            string name = from_compact(param->name);
            vec_char_append_many(v, name.raw, name.len);
            vec_char_append_str(v, ": ");
            _ty_name(v, param->ty);
            vec_char_append_str(v, ", ");
        }
        if (fn->variadic) {
            Ty_FnParam* param = &fn->params[fn->len - 1];
            vec_char_append_str(v, "... ");
            string argv = from_compact(param->varargs.argv);
            string argc = from_compact(param->varargs.argc);
            vec_char_append_many(v, argv.raw, argv.len);
            vec_char_append_str(v, " ");
            vec_char_append_many(v, argc.raw, argc.len);
        } else {
            Ty_FnParam* param = &fn->params[fn->len - 1];
            vec_char_append_str(v, param->out ? "OUT " : "");
            string name = from_compact(param->name);
            vec_char_append_many(v, name.raw, name.len);
            vec_char_append_str(v, ": ");
            _ty_name(v, param->ty);

        }

        vec_char_append_str(v, ")");
        if (fn->ret_ty != TY_VOID) {
            vec_char_append_str(v, ": ");
            _ty_name(v, fn->ret_ty);
        }

    } return;
    default: 
        // attempt to find a matching alias
        // scuffed af lmao
        for_n(i, 0, global_scope->map.cap) {
            Entity* ent = global_scope->map.vals[i];
            if (ent == nullptr) {
                continue;
            }
            if (ent->kind != ENTKIND_TYPE) {
                continue;
            }
            TyKind ty_kind = TY_KIND(ent->ty);
            if (ty_kind != TY_ALIAS && ty_kind != TY_ALIAS_INCOMPLETE) {
                continue;
            }
            string name = from_compact(ent->name);
            TyAlias* alias = TY(ent->ty, TyAlias);
            if (alias->aliasing != t) {
                continue;
            }
            vec_char_append_many(v, name.raw, name.len);
            return;
        }
        break;
    }
    vec_char_append_str(v, "???");
}

const char* ty_name(TyIndex t) {
    Vec(char) buf = vec_new(char, 16);
    _ty_name(&buf, t);
    vec_append(&buf, 0);
    return buf.at;
}

static bool ty_is_scalar(TyIndex t) {
    if_unlikely (t == TY_VOID) {
        return false;
    }
    if_likely (t < TY_PTR) {
        return true;
    }
    
    t = ty_unwrap_alias(t);

    u8 ty_kind = TY(t, TyBase)->kind;
    return ty_kind == TY_PTR || ty_kind == TY_ENUM;
}

static bool ty_is_integer(TyIndex t) {
    if (t == TY_VOID) {
        return false;
    }
    if_likely (t < TY_PTR) {
        return true;
    }

    t = ty_unwrap_alias_or_enum(t);

    if_likely (t < TY_PTR) {
        return true;
    }

    return false;
}

static bool ty_is_signed(TyIndex t) {
    switch (t) {
    case TY_BYTE:
    case TY_INT:
    case TY_LONG:
    case TY_QUAD:
        return true;
    default:
        return false;
    }
}

thread_local static usize target_ptr_size = 4;
thread_local static usize target_ptr_align = 4;
thread_local static TyIndex target_word  = TY_LONG;
thread_local static TyIndex target_uword = TY_ULONG;
#define TY_VOIDPTR (TY_VOID + TY_PTR)

static usize ty_size(TyIndex t) {
    switch (t) {
    case TY_VOID: return 0;
    case TY_BYTE:
    case TY_UBYTE: return 1;
    case TY_INT:
    case TY_UINT: return 2;
    case TY_LONG:
    case TY_ULONG: return 4;
    case TY_QUAD:
    case TY_UQUAD: return 8;
    }

    switch (TY_KIND(t)) {
    case TY_PTR:
        return target_ptr_size;
    case TY_ARRAY:
        return TY(t, TyArray)->len * ty_size(TY(t, TyArray)->to);
    case TY_STRUCT:
    case TY_STRUCT_PACKED:
    case TY_UNION:
        return TY(t, TyRecord)->size;
    case TY_ENUM:
        return ty_size(TY(t, TyEnum)->backing_ty);
    default:
    }
    TODO("AAAA");
}

static usize ty_align(TyIndex t) {
    switch (t) {
    case TY_VOID: return 0;
    case TY_BYTE:
    case TY_UBYTE: return 1;
    case TY_INT:
    case TY_UINT: return 2;
    case TY_LONG:
    case TY_ULONG: return 4;
    case TY_QUAD:
    case TY_UQUAD: return 8;
    }

    switch (TY_KIND(t)) {
    case TY_PTR:
        return target_ptr_align;
    case TY_ARRAY:
        return ty_align(TY(t, TyArray)->to);
    case TY_STRUCT:
    case TY_STRUCT_PACKED:
    case TY_UNION:
        return TY(t, TyRecord)->align;
    case TY_ENUM:
        return ty_align(TY(t, TyEnum)->backing_ty);
    default:
    }
    TODO("AAAA");
}

static bool ty_equal(TyIndex t1, TyIndex t2) {
    if_likely (t1 == t2) {
        return true;
    }
    t1 = ty_unwrap_alias(t1);
    t2 = ty_unwrap_alias(t2);
    if_likely (t1 == t2) {
        return true;
    }

    if_unlikely (TY_KIND(t1) == TY_PTR && TY_KIND(t2) == TY_PTR) {
        return ty_equal(TY(t1, TyPtr)->to, TY(t2, TyPtr)->to);
    }

    if_unlikely (TY_KIND(t1) == TY_ARRAY && TY_KIND(t2) == TY_ARRAY) {
        TyArray* t1_array = TY(t1, TyArray);
        TyArray* t2_array = TY(t2, TyArray);
        return t1_array->len == t2_array->len && ty_equal(t1_array->to, t2_array->to);
    }

    // function type checking
    if_unlikely (TY_KIND(t1) == TY_FN && TY_KIND(t2) == TY_FN) {
        TyFn* fn1 = TY(t1, TyFn);
        TyFn* fn2 = TY(t2, TyFn);

        if (fn1->len != fn2->len || fn1->variadic != fn2->variadic) {
            return false;
        }

        if (!ty_equal(fn1->ret_ty, fn2->ret_ty)) {
            return false;
        }

        for_n(i, 0, fn1->len - 1) {
            Ty_FnParam p1 = fn1->params[i];
            Ty_FnParam p2 = fn2->params[i];
            
            if (p1.out != p2.out) {
                return false;
            }
            if (!ty_equal(p1.ty, p2.ty)) {
                return false;
            }
            if (!string_eq(from_compact(p1.name), from_compact(p2.name))) {
                return false;
            }
        }

        if (fn1->variadic) {
            Ty_FnParam p1 = fn1->params[fn1->len - 1];
            Ty_FnParam p2 = fn2->params[fn1->len - 1];
            if (!string_eq(from_compact(p1.varargs.argv), from_compact(p2.varargs.argv))) {
                return false;
            }
            if (!string_eq(from_compact(p1.varargs.argc), from_compact(p2.varargs.argc))) {
                return false;
            }
        } else if (fn1->len != 0) {
            Ty_FnParam p1 = fn1->params[fn1->len - 1];
            Ty_FnParam p2 = fn2->params[fn1->len - 1];
            
            if (p1.out != p2.out) {
                return false;
            }
            if (!ty_equal(p1.ty, p2.ty)) {
                return false;
            }
            if (!string_eq(from_compact(p1.name), from_compact(p2.name))) {
                return false;
            }
        }
        return true; // guess they are equal after all...
    }

    return t1 == t2;
}


static bool ty_can_cast(TyIndex dst, TyIndex src, bool src_is_constant) {
    if (ty_equal(dst, src)) {
        return true;
    }

    dst = ty_unwrap_alias_or_enum(dst);
    src = ty_unwrap_alias_or_enum(src);
    
    if (ty_is_scalar(dst) && ty_is_scalar(src)) {
        // signedness cannot mix
        return true;
    }

    return false;
}

static bool ty_compatible(TyIndex dst, TyIndex src, bool src_is_constant) {
    if (ty_equal(dst, src)) {
        return true;
    }

    dst = ty_unwrap_alias_or_enum(dst);
    src = ty_unwrap_alias_or_enum(src);

    if (ty_is_integer(dst) && ty_is_integer(src)) {
        // signedness cannot mix
        return src_is_constant || ty_is_signed(dst) == ty_is_signed(src);
    }

    if (TY_KIND(dst) == TY_PTR && (src == TY_VOIDPTR || src_is_constant)) {
        return true;
    }
    if (TY_KIND(src) == TY_PTR && (dst == TY_VOIDPTR || dst == target_uword)) {
        return true;
    }

    return false;
}

void enter_scope(Parser* p) {
    // re-use subscope if possible
    if (p->current_scope->sub) {
        p->current_scope = p->current_scope->sub;
        if (p->current_scope->map.size != 0) {
            strmap_reset(&p->current_scope->map);
        }
        return;
    }

    // create a new scope
    ParseScope* scope = malloc(sizeof(ParseScope));
    scope->sub = nullptr;
    scope->super = p->current_scope;
    p->current_scope->sub = scope;
    strmap_init(&scope->map, 128);

    p->current_scope = p->current_scope->sub;
}

void exit_scope(Parser* p) {
    if (p->current_scope == p->global_scope) {
        CRASH("exited global scope");
    }
    p->current_scope = p->current_scope->super;
}

// general dynamic buffer for parsing shit

VecPtr_typedef(void);
static thread_local VecPtr(void) dynbuf;

static inline usize dynbuf_start() {
    return dynbuf.len;
}

static inline void dynbuf_restore(usize start) {
    dynbuf.len = start;
}

static inline usize dynbuf_len(usize start) {
    return dynbuf.len - start;
}

static inline void** dynbuf_to_arena(Parser* p, usize start) {
    usize len = dynbuf.len - start;
    void** items = arena_alloc(&p->arena, len * sizeof(items[0]), alignof(void*));
    memcpy(items, &dynbuf.at[start], len * sizeof(items[0]));
    return items;
}

Entity* new_entity(Parser* p, string ident, EntityKind kind) {
    Entity* entity = arena_alloc(&p->entities, sizeof(Entity), alignof(Entity));
    *entity = (Entity){0};
    entity->name = to_compact(ident);
    entity->kind = kind;
    entity->ty = TY__INVALID;
    strmap_put(&p->current_scope->map, ident, entity);
    return entity;
}

Entity* get_entity(Parser* p, string key) {
    ParseScope* scope = p->current_scope;
    while (scope) {
        Entity* entity = strmap_get(&scope->map, key);
        if (entity != STRMAP_NOT_FOUND) {
            return entity;
        }
        scope = scope->super;
    }
    return nullptr;
}

static bool token_is_within(SrcFile* f, char* raw) {
    return (uintptr_t)f->src.raw < (uintptr_t)raw
        && (uintptr_t)f->src.raw + f->src.len > (uintptr_t)raw;
}

static SrcFile* where_from(Parser* ctx, string span) {
    for_n (i, 0, ctx->sources.len) {
        SrcFile* f = ctx->sources.at[i];
        if (token_is_within(f, span.raw)) {
            return f;
        }
    }
    return nullptr;
}

static usize preproc_depth(Parser* ctx, u32 index) {
    usize depth = 0;
    for_n(i, 0, index) {
        Token t = ctx->tokens[i];
        switch (t.kind) {
        case TOK_PREPROC_MACRO_PASTE:
        case TOK_PREPROC_DEFINE_PASTE:
        case TOK_PREPROC_INCLUDE_PASTE:
            depth++;
            break;
        case TOK_PREPROC_PASTE_END:
            depth--;
            break;
        }
    }
    return depth;
}

void token_error(Parser* ctx, ReportKind kind, u32 start_index, u32 end_index, const char* msg) {
    // find out if we're in a macro somewhere
    bool inside_preproc = preproc_depth(ctx, start_index) != 0 || preproc_depth(ctx, end_index) != 0;

    Vec_typedef(ReportLine);
    Vec(ReportLine) reports = vec_new(ReportLine, 8);

    i32 unmatched_ends = 0;
    for (i64 i = (i64)start_index; i > 0; --i) {
        Token t = ctx->tokens[i];

        ReportLine report = {};
        report.kind = REPORT_NOTE;

        switch (t.kind) {
        case TOK_PREPROC_DEFINE_PASTE:
        case TOK_PREPROC_MACRO_PASTE:
            if (unmatched_ends != 0) {
                unmatched_ends--;
                break;
            }
            SrcFile* from = where_from(ctx, tok_span(t));
            if (!from) {
                CRASH("unable to locate macro paste span source file");
            }
            report.msg = strprintf("using macro '"str_fmt"'", str_arg(tok_span(t)));
            report.path = from->path;
            report.src = from->src;
            report.snippet = tok_span(t);
            vec_append(&reports, report);
            break;
        case TOK_PREPROC_PASTE_END:
            unmatched_ends++;
            break;
        }
    }

    // find main line snippet
    SrcFile* main_file = ctx->sources.at[0];
    if (inside_preproc) {
        // for_n(i, 0, reports.len) {
        //     report_line(&reports.at[i]);
        // }

        string main_highlight = {};
        for_n(i, end_index, ctx->tokens_len) {
            Token t = ctx->tokens[i];
            if (t.kind == TOK_PREPROC_PASTE_END && preproc_depth(ctx, i + 1) == 0) {
                main_highlight = tok_span(t);
                break;
            }
        }

        // vec_append(&reports, main_line_report);

        // construct the line
        u32 expanded_snippet_begin_index = start_index;    
        while (true) {
            u8 kind = ctx->tokens[expanded_snippet_begin_index].kind;
            if (kind == TOK_NEWLINE) {
                expanded_snippet_begin_index += 1;
                break;
            }
            if (expanded_snippet_begin_index == 0) {
                break;
            }
            expanded_snippet_begin_index -= 1;
        }
        u32 expanded_snippet_end_index = end_index;
        while (true) {
            u8 kind = ctx->tokens[expanded_snippet_end_index].kind;
            if (kind == TOK_NEWLINE || expanded_snippet_end_index == ctx->tokens_len - 1) {
                break;
            }
            expanded_snippet_end_index += 1;
        }

        u32 expanded_snippet_highlight_start = 0;
        u32 expanded_snippet_highlight_len = 0;

        Vec(char) expanded_snippet = vec_new(char, 256);

        Token last_token = {};
        for_n_eq(i, expanded_snippet_begin_index, expanded_snippet_end_index) {
            if (i == start_index) {
                expanded_snippet_highlight_start = expanded_snippet.len;
            }
            Token t = ctx->tokens[i];
            if (TOK__PARSE_IGNORE < t.kind) {
                continue;
            }

            if (i != expanded_snippet_begin_index) {
                // if there's space between tokens
                if (last_token.raw + last_token.len != t.raw) {
                    vec_append(&expanded_snippet, ' ');
                    if (i == start_index) {
                        expanded_snippet_highlight_start++;
                    }
                }
            }
            
            vec_char_append_many(&expanded_snippet, tok_raw(t), t.len);
            if (i == end_index) {
                expanded_snippet_highlight_len = expanded_snippet.len - expanded_snippet_highlight_start;
            }
            last_token = t;
        }

        string src = {
            .raw = expanded_snippet.at,
            .len = expanded_snippet.len,
        };
        string snippet = {
            .raw = expanded_snippet.at + expanded_snippet_highlight_start,
            .len = expanded_snippet_highlight_len,
        };

        ReportLine rep = {
            .kind = kind,
            .msg = str(msg),
            .path = main_file->path,
            .src = main_file->src,
            .snippet = main_highlight,

            .reconstructed_line = src,
            .reconstructed_snippet = snippet,
        };

        report_line(&rep);

        for_n(i, 0, reports.len) {
            report_line(&reports.at[i]);
        }
    } else {
        string start_span = tok_span(ctx->tokens[start_index]);
        string end_span = tok_span(ctx->tokens[end_index]);
        string span = {
            .raw = start_span.raw,
            .len = (usize)end_span.raw - (usize)start_span.raw + end_span.len,
        };
        ReportLine rep = {
            .kind = kind,
            .msg = str(msg),
            .path = main_file->path,
            .snippet = span,
            .src = main_file->src,
        };

        report_line(&rep);
    }

    if (kind == REPORT_ERROR) {
        exit(1);
    }
}

static void advance(Parser* p) {
    do { // skip past "transparent" tokens.
        if (p->tokens[p->cursor].kind == TOK_EOF) {
            break;
        }
        ++p->cursor;
        switch (p->tokens[p->cursor].kind) {
        case TOK_PREPROC_DEFINE_PASTE:
        case TOK_PREPROC_MACRO_PASTE:
            enter_scope(p);
            break;
        case TOK_PREPROC_PASTE_END:
            exit_scope(p);
            break;
        }
    } while (TOK__PARSE_IGNORE < p->tokens[p->cursor].kind);
    p->current = p->tokens[p->cursor];
    // printf("-> "str_fmt"\n", str_arg(tok_span(p->current)));
}

static Token peek(Parser* p, usize n) {
    u32 cursor = p->cursor;
    for_n(_, 0, n) {
        do { // skip past "transparent" tokens.
            if (p->tokens[cursor].kind == TOK_EOF) {
                return p->tokens[cursor];
            }
            ++cursor;
        } while (TOK__PARSE_IGNORE < p->tokens[cursor].kind);
    }
    return p->tokens[cursor];
}

static bool has_eof_or_nl(Parser* p, u32 pos) {
    while (p->tokens[pos].kind != TOK_EOF) {
        if (p->tokens[pos].kind == TOK_NEWLINE) {
            return true;
        }
        if (p->tokens[pos].kind > TOK__PARSE_IGNORE) {
            pos++;
            continue;
        }
        return false;
    }
    return true;
}


inline static bool match(Parser* p, u8 kind) {
    return p->current.kind == kind;
}

static void parse_error(Parser* p, u32 start, u32 end, ReportKind kind, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (p->flags.error_on_warn && kind == REPORT_WARNING) {
        kind = REPORT_ERROR;
    }

    thread_local static char sprintf_buf[512];

    vsnprintf(sprintf_buf, sizeof(sprintf_buf), fmt, args);

    va_end(args);

    token_error(p, kind, start, end, sprintf_buf);
}

static void expect(Parser* p, u8 kind) {
    if_unlikely (kind != p->current.kind) {
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "expected '%s', got '%s'", token_kind[kind], token_kind[p->current.kind]);
    }
}

#define new_expr(p, kind, ty, field) \
    new_expr_(p, kind, ty, offsetof(Expr, extra) + sizeof(((Expr*)nullptr)->field))

static Expr* new_expr_(Parser* p, u8 kind, TyIndex ty, usize size) {
    Expr* expr = arena_alloc(&p->arena, size, alignof(Expr));
    expr->kind = kind;
    expr->ty = ty;
    expr->token_index = p->cursor;
    return expr;
}

i64 eval_integer_dec(Parser* p, const char* raw, u32 len, u32 index) {
    i64 val = 0;
    for (usize i = 0; i < len; ++i) {
        val *= 10;
        isize char_val = raw[i] - '0';
        if (char_val < 0 || char_val > 9) {
            parse_error(p, index, index, REPORT_ERROR, "invalid decimal digit '%c'", raw[i]);
        }
        val += char_val;
    }
    return val;
}

i64 eval_integer_hex(Parser* p, const char* raw, u32 len, u32 index) {
    i64 val = 0;
    for (usize i = 0; i < len; ++i) {
        val *= 16;
        char c = raw[i];
        if ('a' <= c && c <= 'f') {
            val += c - 'a' + 10;
        } else if ('A' <= c && c <= 'F') {
            val += c - 'A' + 10;
        } else if ('0' <= c && c <= '9') {
            val += c - '0';
        } else {
            parse_error(p, index, index, REPORT_ERROR, "invalid hexadecimal digit '%c'", c);
        }
    }
    return val;
}

i64 eval_integer_oct(Parser* p, const char* raw, u32 len, u32 index) {
    i64 val = 0;
    for (usize i = 0; i < len; ++i) {
        val *= 8;
        char c = raw[i];
        if ('0' <= c && c <= '8') {
            val += c - '0';
        } else {
            parse_error(p, index, index, REPORT_ERROR, "invalid hexidecimal digit '%c'", c);
        }
    }
    return val;
}

i64 eval_integer(Parser* p, Token t, u32 index) {
    u32 len = t.len;
    char* raw = tok_raw(t);
    bool is_negative = false;
    if (raw[0] == '-') {
        raw += 1;
        len -= 1;
        is_negative = true;
    }

    i64 val = 0;
    if (raw[0] == '0' && len > 1) {
        switch (raw[1]) {
        case 'x':
            val = eval_integer_hex(p, raw + 2, len - 2, index);
            break;
        default:
            // leading zero octal will be the death of me
            val = eval_integer_oct(p, raw, len, index);
            break;
        }
    } else {
        val = eval_integer_dec(p, raw, len, index);
    }
    
    return is_negative ? -val : val;
}

u32 expr_leftmost_token(Expr* expr) {
    switch (expr->kind) {
    case EXPR_LITERAL:
    case EXPR_STR_LITERAL:
    case EXPR_NOT:
    case EXPR_ADDROF:
    case EXPR_ENTITY:
    case EXPR_CALL:
    case EXPR_COMPOUND_LITERAL:
    case EXPR_INDEXED_ITEM:
    case EXPR_EMPTY_COMPOUND_LITERAL:
        return expr->token_index;
    case EXPR_DEREF:
        return expr_leftmost_token(expr->unary);
    case EXPR_DEREF_MEMBER:
    case EXPR_MEMBER:
        return expr_leftmost_token(expr->member_access.aggregate);
    case EXPR_ADD:
    case EXPR_SUB:
    case EXPR_MUL:
    case EXPR_DIV:
    case EXPR_REM:
    case EXPR_AND:
    case EXPR_OR:
    case EXPR_XOR:
    case EXPR_LSH:
    case EXPR_RSH:
        return expr_leftmost_token(expr->binary.lhs);
    default:
        TODO("unknown expr kind %u", expr->kind);
    }
}

u32 expr_rightmost_token(Expr* expr) {
    switch (expr->kind) {
    case EXPR_LITERAL:
    case EXPR_STR_LITERAL:
    case EXPR_DEREF:
    case EXPR_ENTITY:
    case EXPR_DEREF_MEMBER:
    case EXPR_MEMBER:
    case EXPR_CALL:
    case EXPR_INDEXED_ITEM:
    case EXPR_COMPOUND_LITERAL:
        return expr->token_index;
    case EXPR_EMPTY_COMPOUND_LITERAL:
        return expr->token_index + 1;
    case EXPR_NOT:
    case EXPR_ADDROF:
        return expr_rightmost_token(expr->unary);
    case EXPR_ADD:
    case EXPR_SUB:
    case EXPR_MUL:
    case EXPR_DIV:
    case EXPR_REM:
    case EXPR_AND:
    case EXPR_OR:
    case EXPR_XOR:
    case EXPR_LSH:
    case EXPR_RSH:
        return expr_rightmost_token(expr->binary.rhs);
    default:
        TODO("unknown expr kind %u", expr->kind);
    }
}

#define error_at_expr(p, expr, report_kind, msg, ...) \
    parse_error(p, expr_leftmost_token(expr), expr_rightmost_token(expr), report_kind, msg __VA_OPT__(,) __VA_ARGS__)

TyIndex parse_type_terminal(Parser* p, bool allow_incomplete) {
    switch (p->current.kind) {
    case TOK_KW_VOID:  advance(p); return TY_VOID;
    case TOK_KW_BYTE:  advance(p); return TY_BYTE;
    case TOK_KW_UBYTE: advance(p); return TY_UBYTE;
    case TOK_KW_INT:   advance(p); return TY_INT;
    case TOK_KW_UINT:  advance(p); return TY_UINT;
    case TOK_KW_LONG:  advance(p); return TY_LONG;
    case TOK_KW_ULONG: advance(p); return TY_ULONG;
    case TOK_KW_QUAD:  advance(p); return TY_QUAD;
    case TOK_KW_UQUAD: advance(p); return TY_UQUAD;
    case TOK_KW_WORD:  advance(p); return target_word;
    case TOK_KW_UWORD: advance(p); return target_uword;
    case TOK_OPEN_PAREN:
        advance(p);
        TyIndex inner = parse_type(p, allow_incomplete);
        expect(p, TOK_CLOSE_PAREN);
        advance(p);
        return inner;
    case TOK_CARET:
        advance(p);
        return ty_get_ptr(parse_type_terminal(p, true));
    case TOK_IDENTIFIER:
        ;
        string span = tok_span(p->current);
        Entity* entity = get_entity(p, span);
        if (!entity) { // create an incomplete type
            if (!allow_incomplete) {
                parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "cannot use incomplete type");
            }
            TyIndex incomplete = ty_allocate(TyAlias);
            entity = new_entity(p, span, ENTKIND_TYPE);
            entity->ty = incomplete;
            advance(p);
            return incomplete;
        }
        if (entity->kind != ENTKIND_TYPE) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "symbol is not a type");
        }
        if (!allow_incomplete && TY(entity->ty, TyAlias)->kind == TY_ALIAS_INCOMPLETE) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "cannot use incomplete type");
        }
        advance(p);
        TyIndex t = entity->ty;
        while (TY_KIND(t) == TY_ALIAS) {
            t = TY(t, TyAlias)->aliasing;
        }
        return t;
    default:
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "expected type expression");
    }
    return TY_VOID;
}

TyIndex parse_type(Parser* p, bool allow_incomplete) {
    TyIndex left = parse_type_terminal(p, allow_incomplete);
    while (p->current.kind == TOK_OPEN_BRACKET) {
        if (TY_KIND(left) == TY_ALIAS_INCOMPLETE) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "cannot use incomplete type");
        }
        advance(p);
        TyIndex arr = ty_allocate(TyArray);
        TY(arr, TyArray)->to = left;
        TY(arr, TyArray)->kind = TY_ARRAY;
        Expr* len_expr = parse_expr(p);

        if (!ty_is_integer(len_expr->ty)) {
            error_at_expr(p, len_expr, REPORT_ERROR, "array length must be integer");
        }
        if (len_expr->kind != EXPR_LITERAL) {
            error_at_expr(p, len_expr, REPORT_ERROR, "array length must be compile-time known");
        }
        if (len_expr->literal > (u64)INT32_MAX) {
            error_at_expr(p, len_expr, REPORT_WARNING, "array length is... excessive");
        }
        TY(arr, TyArray)->len = len_expr->literal;
        expect(p, TOK_CLOSE_BRACKET);
        advance(p);
        left = arr;
    }
    return left;
}

static bool is_lvalue(Expr* e) {
    switch (e->kind) {
    case EXPR_ENTITY:
        return e->entity->kind == ENTKIND_VAR;
    case EXPR_DEREF:
    case EXPR_DEREF_MEMBER:
    case EXPR_MEMBER:
    case EXPR_PTR_INDEX:
    case EXPR_ARRAY_INDEX:
        return true;
    default:
        return false;
    }
}

Expr* parse_atom_terminal(Parser* p) {
    string span = tok_span(p->current);
    Expr* atom = nullptr;
    switch (p->current.kind) {
    case TOK_OPEN_PAREN:
        advance(p);
        atom = parse_expr(p);
        expect(p, TOK_CLOSE_PAREN);
        advance(p);
        break;
    case TOK_INTEGER:
        atom = new_expr(p, EXPR_LITERAL, target_uword, literal);
        atom->literal = eval_integer(p, p->current, p->cursor);
        advance(p);
        break;
    case TOK_KW_TRUE:
        atom = new_expr(p, EXPR_LITERAL, target_uword, literal);
        atom->literal = 1;
        advance(p);
        break;
    case TOK_KW_FALSE:
        atom = new_expr(p, EXPR_LITERAL, target_uword, literal);
        atom->literal = 0;
        advance(p);
        break;
    case TOK_KW_NULLPTR:
        atom = new_expr(p, EXPR_LITERAL, TY_VOIDPTR, literal);
        atom->literal = 0;
        advance(p);
        break;
    case TOK_KW_ALIGNOF:
        atom = new_expr(p, EXPR_LITERAL, target_uword, literal);
        advance(p);
        TyIndex type = parse_type(p, false); 
        atom->literal = ty_align(type);
        break;
    case TOK_KW_SIZEOF:
        atom = new_expr(p, EXPR_LITERAL, target_uword, literal);
        advance(p);
        type = parse_type(p, false); 
        atom->literal = ty_size(type);
        break;
    case TOK_STRING:
        atom = new_expr(p, EXPR_STR_LITERAL, ty_get_ptr(TY_UBYTE), lit_string);
        advance(p);
        // atom->literal = ty_size(type);
        break;
    case TOK_IDENTIFIER:
        // find an entity
        ;
        Entity* entity = get_entity(p, span);
        if_unlikely (entity == nullptr) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
                "symbol does not exist");
        }
        if_unlikely (entity->kind == ENTKIND_TYPE) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
                "entity is a type");
        }

        if (entity->kind == ENTKIND_VARIANT) {
            atom = new_expr(p, EXPR_LITERAL, entity->ty, literal);
            atom->literal = entity->variant_value;
        } else {
            atom = new_expr(p, EXPR_ENTITY, entity->ty, entity);
            atom->entity = entity;
        }

        advance(p);
        break;
    default:
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
            "expected expression");
    }

    return atom;
}

Expr* parse_atom(Parser* p) {
    Expr* atom = parse_atom_terminal(p);
    Expr* left = nullptr;

    while (true) {
        switch (p->current.kind) {
        case TOK_OPEN_BRACKET: {
            // UNREACHABLE;
            left = atom;
            TyKind left_ty_kind = TY_KIND(left->ty);
            if_likely (left_ty_kind == TY_PTR) {
                atom = new_expr(p, EXPR_PTR_INDEX, TY(left->ty, TyPtr)->to, binary);
                atom->binary.lhs = left;
                advance(p);
                Expr* index = parse_expr(p);
                if_unlikely (!ty_is_integer(index->ty)) {
                    parse_error(p, expr_leftmost_token(left), p->cursor, REPORT_ERROR, 
                        "index type %s is not an integer", ty_name(left->ty));
                }
                atom->binary.rhs = index;
                expect(p, TOK_CLOSE_BRACKET);
                advance(p);
            } else if_likely (left_ty_kind == TY_ARRAY) {
                atom = new_expr(p, EXPR_ARRAY_INDEX, TY(left->ty, TyPtr)->to, binary);
                atom->binary.lhs = left;
                advance(p);
                Expr* index = parse_expr(p);
                if_unlikely (!ty_is_integer(index->ty)) {
                    parse_error(p, expr_leftmost_token(left), p->cursor, REPORT_ERROR, 
                        "index type %s is not an integer", ty_name(left->ty));
                }
                atom->binary.rhs = index;
                expect(p, TOK_CLOSE_BRACKET);
                advance(p);
            } else {
                parse_error(p, expr_leftmost_token(left), p->cursor, REPORT_ERROR, 
                    "cannot index type %s", ty_name(left->ty));
            }
        } break;
        case TOK_CARET: {
            if_unlikely (peek(p, 1).kind == TOK_DOT) {
                goto member_deref;
            }
            left = atom;
            if_unlikely (TY_KIND(left->ty) != TY_PTR) {
                parse_error(p, expr_leftmost_token(left), p->cursor, REPORT_ERROR, 
                    "cannot dereference type %s", ty_name(left->ty));
            }
            TyIndex ptr_target = ty_get_ptr_target(left->ty);
            // if_unlikely (!ty_is_scalar(ptr_target)) {
            //     parse_error(p, expr_leftmost_token(left), p->cursor, REPORT_ERROR, 
            //         "cannot use non-scalar type %s", ty_name(ptr_target));
            // }
            atom = new_expr(p, EXPR_DEREF, ptr_target, unary);
            atom->unary = left;
            advance(p);
        } break;
        case TOK_CARET_DOT: {
            member_deref:
            left = atom;

            TyIndex left_ty = ty_unwrap_alias(left->ty);

            if_unlikely (TY_KIND(left_ty) != TY_PTR) {
                parse_error(p, expr_leftmost_token(left), expr_rightmost_token(left), REPORT_ERROR, 
                    "cannot dereference type %s", ty_name(left->ty));
            }
            TyIndex record_ty = ty_unwrap_alias(ty_get_ptr_target(left_ty));
            TyKind record_ty_kind = TY_KIND(record_ty);

            if_unlikely (record_ty_kind != TY_STRUCT && record_ty_kind != TY_STRUCT_PACKED && record_ty_kind != TY_UNION) {
                parse_error(p, expr_leftmost_token(left), expr_rightmost_token(left), REPORT_ERROR, 
                    "type %s is not a STRUCT or UNION", ty_name(record_ty));
            }
            TyRecord* record = TY(record_ty, TyRecord);
            advance(p);

            expect(p, TOK_IDENTIFIER);
            string member_span = tok_span(p->current);

            // search for field
            u32 member_index = 0;
            TyIndex member_ty = TY_VOID;
            for_n(i, 0, record->len) {
                CompactString member_name = record->members[i].name;
                if (string_eq(from_compact(member_name), member_span)) {
                    member_index = i;
                    member_ty = record->members[i].type;
                    break;
                }
            }

            if_unlikely (member_ty == TY_VOID) {
                parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
                    "type %s has no member '"str_fmt"'", ty_name(record_ty), str_arg(member_span));
            }

            atom = new_expr(p, EXPR_DEREF_MEMBER, member_ty, member_access);
            atom->member_access.aggregate = left;
            atom->member_access.member_index = member_index;
            atom->ty = member_ty;
            advance(p);
        } break;
        case TOK_DOT: {
            left = atom;

            TyIndex record_ty = ty_unwrap_alias(left->ty);
            TyKind record_ty_kind = TY_KIND(record_ty);

            if_unlikely (record_ty_kind != TY_STRUCT && record_ty_kind != TY_STRUCT_PACKED && record_ty_kind != TY_UNION) {
                parse_error(p, expr_leftmost_token(left), expr_rightmost_token(left), REPORT_ERROR, 
                    "type %s is not a STRUCT or UNION", ty_name(record_ty));
            }
            TyRecord* record = TY(record_ty, TyRecord);
            advance(p);

            expect(p, TOK_IDENTIFIER);
            string member_span = tok_span(p->current);

            // search for field
            u32 member_index = 0;
            TyIndex member_ty = TY_VOID;
            for_n(i, 0, record->len) {
                CompactString member_name = record->members[i].name;
                if (string_eq(from_compact(member_name), member_span)) {
                    member_index = i;
                    member_ty = record->members[i].type;
                    break;
                }
            }

            if_unlikely (member_ty == TY_VOID) {
                parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
                    "type %s has no member '"str_fmt"'", ty_name(record_ty), str_arg(member_span));
            }

            atom = new_expr(p, EXPR_MEMBER, member_ty, member_access);
            atom->member_access.aggregate = left;
            atom->member_access.member_index = member_index;
            atom->ty = member_ty;
            advance(p);
        } break;
        case TOK_OPEN_PAREN: {
            // function call
            left = atom;

            TyIndex fn_ty = left->ty;

            // make sure we're calling an fn/fnptr lol

            // if not function and not pointer to non-function
            if_unlikely (!(TY_KIND(fn_ty) == TY_FN || (TY_KIND(fn_ty) == TY_PTR && TY_KIND(TY(fn_ty, TyPtr)->to) == TY_FN))) {
                error_at_expr(p, left, REPORT_ERROR, "type %s is not an FN or FNPTR", ty_name(fn_ty));
            }

            if (TY_KIND(fn_ty) == TY_PTR) {
                fn_ty = TY(fn_ty, TyPtr)->to;
            }

            TyFn* fn = TY(fn_ty, TyFn);
            if (fn->variadic) {
                TODO("variadic function calls");
            }

            atom = new_expr(p, EXPR_CALL, fn->ret_ty, call);

            u32 args_start = dynbuf_start();

            // okay, actually check arguments
            advance(p);
            usize arg_n = 0;
            while (!match(p, TOK_CLOSE_PAREN)) {
                if_unlikely (arg_n >= fn->len) {
                    parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "too many arguments, expected %u", fn->len);
                }

                Expr* arg = nullptr;
                Ty_FnParam* param = &fn->params[arg_n];
                if_unlikely (param->out) {
                    expect(p, TOK_KW_OUT);
                    advance(p);
                    arg = parse_expr(p);
                    // out_arg must be an lvalue (assignable)
                    if_unlikely(!is_lvalue(arg)) {
                        error_at_expr(p, arg, REPORT_ERROR, "OUT argument must be an l-value");
                    }
                } else {
                    arg = parse_expr(p);
                }

                if_unlikely (!ty_compatible(param->ty, arg->ty, arg->kind == EXPR_LITERAL)) {
                    error_at_expr(p, arg, REPORT_ERROR, 
                        "type %s cannot coerce to %s", ty_name(arg->ty), ty_name(param->ty));
                }

                if_likely (match(p, TOK_COMMA)) {
                    advance(p);
                } else {
                    break;
                }

                vec_append(&dynbuf, arg);

                arg_n++;
            }
            expect(p, TOK_CLOSE_PAREN);
            advance(p);
            
            Expr** args = (Expr**)dynbuf_to_arena(p, args_start);
            dynbuf_restore(args_start);

            atom->call.callee = left;
            atom->call.args = args;
            atom->call.args_len = arg_n;

            // UNREACHABLE;
        } break;
        default:
            return atom;
        }
    }

    return atom;
}

Expr* parse_unary(Parser* p) {
    // ArenaState save = arena_save(&p->arena);

    while_unlikely (match(p, TOK_KW_NOTHING)) {
        advance(p);
    }

    u32 op_position = p->cursor;

    switch (p->current.kind) {
    case TOK_KW_ALIGNOFVALUE: {
        advance(p);

        ArenaState save = arena_save(&p->arena);

        Expr* value = parse_expr(p);
        TyIndex type = value->ty;

        arena_restore(&p->arena, save);

        Expr* alignofvalue = new_expr(p, EXPR_LITERAL, target_uword, literal);
        alignofvalue->literal = ty_align(type);

        return alignofvalue;
    }
    case TOK_KW_SIZEOFVALUE: {
        advance(p);

        ArenaState save = arena_save(&p->arena);

        Expr* value = parse_expr(p);
        TyIndex type = value->ty;

        arena_restore(&p->arena, save);

        Expr* sizeofvalue = new_expr(p, EXPR_LITERAL, target_uword, literal);
        sizeofvalue->literal = ty_size(type);

        return sizeofvalue;
    }
    case TOK_AND: {
        advance(p);
        Expr* inner = parse_unary(p);
        if_unlikely (!is_lvalue(inner)) {
            error_at_expr(p, inner, REPORT_ERROR, 
                "cannot take address of r-value");
        }
        Expr* addrof = new_expr(p, EXPR_ADDROF, ty_get_ptr(inner->ty), unary);
        addrof->unary = inner;
        addrof->token_index = op_position;
        return addrof;
    }
    case TOK_TILDE: {
        advance(p);
        Expr* inner = parse_unary(p);
        if_unlikely (!ty_is_integer(inner->ty)) {
            error_at_expr(p, inner, REPORT_ERROR, 
                "type %s is not an integer", ty_name(inner->ty));
        }
        if (inner->kind == EXPR_LITERAL) {
            inner->literal = ~inner->literal; // reuse this expr
            return inner;
        } else {
            Expr* not = new_expr(p, EXPR_NOT, inner->ty, unary);
            not->unary = inner;
            not->token_index = op_position;
            return not;
        }
    }
    case TOK_MINUS: {
        advance(p);
        Expr* inner = parse_unary(p);
        if_unlikely (!ty_is_integer(inner->ty)) {
            error_at_expr(p, inner, REPORT_ERROR, 
                "type %s is not an integer", ty_name(inner->ty));
        }
        if (inner->kind == EXPR_LITERAL) {
            inner->literal = -inner->literal; // reuse this expr
            return inner;
        } else {
            Expr* not = new_expr(p, EXPR_NEG, inner->ty, unary);
            not->unary = inner;
            not->token_index = op_position;
            return not;
        }
    }
    case TOK_KW_NOT: {
        advance(p);
        Expr* inner = parse_unary(p);
        if_unlikely (!ty_is_scalar(inner->ty)) {
            error_at_expr(p, inner, REPORT_ERROR, 
                "type %s is not scalar", ty_name(inner->ty));
        }
        if (inner->kind == EXPR_LITERAL) {
            inner->literal = !inner->literal; // reuse this expr
            return inner;
        } else {
            Expr* not = new_expr(p, EXPR_BOOL_NOT, inner->ty, unary);
            not->unary = inner;
            not->token_index = op_position;
            return not;
        }
    }
    case TOK_KW_CAST: {
        advance(p);
        Expr* inner = parse_expr(p);
        expect(p, TOK_KW_TO);
        advance(p);
        TyIndex to_ty = parse_type(p, false);

        if_unlikely(!ty_can_cast(to_ty, inner->ty, inner->kind == EXPR_LITERAL)) {
            error_at_expr(p, inner, REPORT_ERROR, 
                "type %s cannot cast to %s", ty_name(inner->ty), ty_name(to_ty));
        }

        if (inner->kind == EXPR_LITERAL) {
            inner->ty = to_ty;
            return inner;
        }

        Expr* cast = new_expr(p, EXPR_CAST, to_ty, unary);
        cast->unary = inner;
        return cast;
    }
    default:
        return parse_atom(p);
    }
}

static isize bin_precedence(u8 kind) {
    switch (kind) {
        case TOK_MUL:
        case TOK_DIV:
        case TOK_REM:
            return 10;
        case TOK_PLUS:
        case TOK_MINUS:
            return 9;
        case TOK_LSHIFT:
        case TOK_RSHIFT:
        case TOK_KW_ROR:
            return 8;
        case TOK_AND:
            return 7;
        case TOK_XOR:
            return 6;
        case TOK_OR:
            return 5;
        case TOK_LESS:
        case TOK_LESS_EQ:
        case TOK_GREATER:
        case TOK_GREATER_EQ:
            return 4;
        case TOK_EQ_EQ:
        case TOK_NOT_EQ:
            return 3;
        case TOK_KW_AND:
            return 2;
        case TOK_KW_OR:
            return 1;
    }
    return -1;
}

static ExprKind binary_expr_kind(u8 tok_kind) {
    if_unlikely(tok_kind == TOK_KW_AND) {
        return EXPR_BOOL_AND;
    }
    if_unlikely(tok_kind == TOK_KW_OR) {
        return EXPR_BOOL_OR;
    }
    if_unlikely(tok_kind == TOK_KW_ROR) {
        return EXPR_ROR;
    }

    return tok_kind - (TOK_PLUS + EXPR_ADD);
}

static bool is_bool_op(ExprKind op_kind) {
    return op_kind == EXPR_BOOL_AND || op_kind == EXPR_BOOL_OR;
}

Expr* parse_binary(Parser* p, isize precedence) {
    ArenaState save = arena_save(&p->arena);
    Expr* lhs = parse_unary(p);
    
    while (precedence < bin_precedence(p->current.kind)) {
        isize n_prec = bin_precedence(p->current.kind);
        ExprKind op_kind = binary_expr_kind(p->current.kind);
        u32 op_token_index = p->cursor;
        
        advance(p);
        Expr* rhs = parse_binary(p, n_prec);

        TyIndex op_ty = target_uword;
        if (!is_bool_op(op_kind)) {
            if_unlikely (!ty_compatible(lhs->ty, rhs->ty, rhs->kind == EXPR_LITERAL)) {
                parse_error(p, op_token_index, op_token_index, REPORT_ERROR, 
                    "types %s and %s are not compatible", ty_name(lhs->ty), ty_name(rhs->ty));
            }
            // parse_error(p, op_token_index, op_token_index, REPORT_NOTE, "yuh");
            // printf("is_bool ");
            op_ty = lhs->ty;
        } else {
            if_unlikely(!ty_is_scalar(lhs->ty)) {
                parse_error(p, expr_leftmost_token(lhs), expr_rightmost_token(lhs), REPORT_ERROR, 
                    "type %s is not scalar", ty_name(lhs->ty));
            }
            if_unlikely(!ty_is_scalar(rhs->ty)) {
                parse_error(p, expr_leftmost_token(rhs), expr_rightmost_token(rhs), REPORT_ERROR, 
                    "type %s is not scalar", ty_name(rhs->ty));
            }
        }

        if (lhs->kind == EXPR_LITERAL && rhs->ty != EXPR_LITERAL) {
            op_ty = rhs->ty;
        }
        
        if (lhs->kind == EXPR_LITERAL && rhs->kind == EXPR_LITERAL) {
            bool signed_op = ty_is_signed(lhs->ty) || ty_is_signed(rhs->ty);
            u64 lhs_val = lhs->literal;
            u64 rhs_val = rhs->literal;
            u32 leftmost = expr_leftmost_token(lhs);

            arena_restore(&p->arena, save);
            Expr* lit = new_expr(p, EXPR_LITERAL, op_ty, literal);
            lit->token_index = leftmost;
            
            switch (op_kind) {
            case EXPR_ADD: lit->literal = lhs_val + rhs_val; break;
            case EXPR_SUB: lit->literal = lhs_val - rhs_val; break;
            case EXPR_MUL: lit->literal = lhs_val * rhs_val; break;
            case EXPR_DIV: 
                if (signed_op) {
                    lit->literal = (isize)lhs_val / (isize)rhs_val;
                } else {
                    lit->literal = lhs_val / rhs_val;
                }
                break;
            case EXPR_REM: 
                if (signed_op) {
                    lit->literal = (isize)lhs_val % (isize)rhs_val;
                } else {
                    lit->literal = lhs_val % rhs_val;
                }
                break;
            case EXPR_AND: lit->literal = lhs_val & rhs_val; break;
            case EXPR_OR:  lit->literal = lhs_val | rhs_val; break;
            case EXPR_XOR: lit->literal = lhs_val ^ rhs_val; break;
            case EXPR_LSH: lit->literal = lhs_val << rhs_val; break;
            case EXPR_RSH:
                if (signed_op) {
                    lit->literal = (isize)lhs_val >> (isize)rhs_val;
                } else {
                    lit->literal = lhs_val >> rhs_val;
                }
                break;
            case EXPR_ROR: lit->literal = (lhs_val >> rhs_val) 
                                        | (lhs_val << (ty_size(target_uword) * 8 - rhs_val)); // 💀
                break;
            case EXPR_EQ:  lit->literal = lhs_val == rhs_val; break;
            case EXPR_NEQ: lit->literal = lhs_val != rhs_val; break;
            case EXPR_LESS_EQ:
                if (signed_op) {
                    lit->literal = (isize)lhs_val <= (isize)rhs_val;
                } else {
                    lit->literal = lhs_val <= rhs_val;
                }
                break;
            case EXPR_GREATER_EQ:
                if (signed_op) {
                    lit->literal = (isize)lhs_val >= (isize)rhs_val;
                } else {
                    lit->literal = lhs_val >= rhs_val;
                }
                break;
            case EXPR_LESS:
                if (signed_op) {
                    lit->literal = (isize)lhs_val < (isize)rhs_val;
                } else {
                    lit->literal = lhs_val < rhs_val;
                }
                break;
            case EXPR_GREATER:
                if (signed_op) {
                    lit->literal = (isize)lhs_val > (isize)rhs_val;
                } else {
                    lit->literal = lhs_val > rhs_val;
                }
                break;
            default:
                UNREACHABLE;
            }
            lhs = lit;
        } else {
            Expr* op = new_expr(p, op_kind, op_ty, binary); 
            op->binary.lhs = lhs;
            op->binary.rhs = rhs;
            lhs = op;
        }
    }

    return lhs;
}

Expr* parse_expr(Parser* p) {
    return parse_binary(p, 0);
}

#define new_stmt(p, kind, field) \
    new_stmt_(p, kind, offsetof(Stmt, extra) + sizeof(((Stmt*)nullptr)->field))

static Stmt* new_stmt_(Parser* p, u8 kind, usize size) {
    Stmt* stmt = arena_alloc(&p->arena, size, alignof(Stmt));
    memset(stmt, 0, size);
    stmt->retkind = RETKIND_NO;
    stmt->kind = kind;
    stmt->token_index = p->cursor;
    return stmt;
}

static Entity* get_or_create(Parser* p, string ident) {
    Entity* entity = get_entity(p, ident);
    if (!entity) {
        entity = new_entity(p, ident, ENTKIND_VAR);
    } else if (entity->storage != STORAGE_EXTERN) {
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "symbol already exists");
    }
    return entity;
}

static bool is_global_data(Expr* expr) {
    switch (expr->kind) {
    case EXPR_ENTITY:
        // should really just be 'true' but do the extra check, whatever
        return expr->entity->storage != STORAGE_LOCAL && expr->entity->storage != STORAGE_OUT_PARAM;
    case EXPR_MEMBER:
        return is_global_data(expr->member_access.aggregate);
    default:
        return false;
    }
}

static void ensure_linktime_const(Parser* p, Expr* expr) {
    switch (expr->kind) {
    case EXPR_LITERAL:
        return;
    case EXPR_ADDROF:
        if_unlikely (!is_global_data(expr->unary)) {
            break;
        }
        return;
    case EXPR_COMPOUND_LITERAL:
        for_n(i, 0, expr->compound_lit.len) {
            ensure_linktime_const(p, expr->compound_lit.values[i]);
        }
        return;
    case EXPR_INDEXED_ITEM:
        ensure_linktime_const(p, expr->indexed_item.value);
        return;
    default:
        break;
    }
    error_at_expr(p, expr, REPORT_ERROR, "expression is not link-time constant");
}

Expr* parse_initializer(Parser* p, TyIndex ty);

Expr* parse_array_initializer(Parser* p, TyIndex array_ty) {
    u32 init_start_token = p->cursor;

    expect(p, TOK_OPEN_BRACE);
    advance(p);
    if_unlikely (match(p, TOK_CLOSE_BRACE)) {
        Expr* empty = new_expr(p, EXPR_EMPTY_COMPOUND_LITERAL, array_ty, nothing);
        empty->token_index = p->cursor;
        advance(p);
        return empty;
    }

    TyIndex elem_ty = TY(array_ty, TyArray)->to;
    u32 array_len = TY(array_ty, TyArray)->len;

    u32 exprs_start = dynbuf_start();
    u32 exprs_len = 0;

    // [ expr ] = initializer
    // expr

    u32 index = 0;
    while (!match(p, TOK_CLOSE_BRACE)) {

        if (match(p, TOK_OPEN_BRACKET)) {
            advance(p);
            Expr* expr = parse_expr(p);
            expect(p, TOK_CLOSE_BRACKET);
            advance(p);
            expect(p, TOK_EQ);
            advance(p);

            if_unlikely (expr->kind != EXPR_LITERAL || !ty_is_integer(expr->ty)) {
                error_at_expr(p, expr, REPORT_ERROR, "index must be a compile-time constant integer");
            }
            index = expr->literal;

            if_unlikely (index >= array_len) {
                error_at_expr(p, expr, REPORT_ERROR, "index (%u) must be less than array length (%u)", index, array_len);
            }

            Expr* value = parse_initializer(p, elem_ty);
            if_unlikely (!ty_compatible(elem_ty, value->ty, value->kind == EXPR_LITERAL)) {
                error_at_expr(p, value, REPORT_ERROR, "type %s cannot coerce to %s",
                    ty_name(value->ty), ty_name(elem_ty));
            }

            Expr* item = new_expr(p, EXPR_INDEXED_ITEM, elem_ty, indexed_item);
            item->indexed_item.index = index;
            item->indexed_item.value = value;
            vec_append(&dynbuf, item);
        } else {
            Expr* value = parse_initializer(p, elem_ty);
            if_unlikely (!ty_compatible(elem_ty, value->ty, value->kind == EXPR_LITERAL)) {
                error_at_expr(p, value, REPORT_ERROR, "type %s cannot coerce to %s",
                    ty_name(value->ty), ty_name(elem_ty));
            }

            if_unlikely (index >= array_len) {
                error_at_expr(p, value, REPORT_ERROR, "index (%u) must be less than array length (%u)", index, array_len);
            }
            vec_append(&dynbuf, value);
        }

        index++;
        exprs_len++;

        if_likely (match(p, TOK_COMMA)) {
            advance(p);
            continue;
        } else {
            break;
        }
    }
    expect(p, TOK_CLOSE_BRACE);
    advance(p);


    Expr** exprs = (Expr**)dynbuf_to_arena(p, exprs_start);

    Expr* array_init = new_expr(p, EXPR_COMPOUND_LITERAL, array_ty, compound_lit);
    array_init->compound_lit.len = exprs_len;
    array_init->compound_lit.values = exprs;
    array_init->token_index = init_start_token;

    dynbuf_restore(exprs_start);

    return array_init;
}


Expr* parse_record_initializer(Parser* p, TyIndex record_ty) {
    u32 init_start_token = p->cursor;

    expect(p, TOK_OPEN_BRACE);
    advance(p);
    if_unlikely (match(p, TOK_CLOSE_BRACE)) {
        Expr* empty = new_expr(p, EXPR_EMPTY_COMPOUND_LITERAL, record_ty, nothing);
        empty->token_index = p->cursor;
        advance(p);
        return empty;
    }

    u32 exprs_start = dynbuf_start();
    u32 exprs_len = 0;

    // [ ident ] = initializer

    while (!match(p, TOK_CLOSE_BRACE)) {

        expect(p, TOK_OPEN_BRACKET);
        advance(p);
        expect(p, TOK_IDENTIFIER);
        string member_name = tok_span(p->current);

        u32 member_index = 0;
        TyIndex member_ty = TY__INVALID;

        TyRecord* record = TY(record_ty, TyRecord);
        for_n (i, 0, record->len) {
            Ty_RecordMember member = record->members[i];
            if (string_eq(member_name, from_compact(member.name))) {
                member_index = i;
                member_ty = member.type;
            }
        }
        if (member_ty == TY__INVALID) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "member '"str_fmt"' does not exist", str_arg(member_name));
        }

        advance(p);
        expect(p, TOK_CLOSE_BRACKET);
        advance(p);
        expect(p, TOK_EQ);
        advance(p);

        Expr* value = parse_initializer(p, member_ty);
        if_unlikely (!ty_compatible(member_ty, value->ty, value->kind == EXPR_LITERAL)) {
            error_at_expr(p, value, REPORT_ERROR, "type %s cannot coerce to %s",
                ty_name(value->ty), ty_name(member_ty));
        }

        Expr* item = new_expr(p, EXPR_INDEXED_ITEM, member_ty, indexed_item);
        item->indexed_item.index = member_index;
        item->indexed_item.value = value;
        vec_append(&dynbuf, item);

        exprs_len++;

        if_likely (match(p, TOK_COMMA)) {
            advance(p);
            continue;
        } else {
            break;
        }
    }
    expect(p, TOK_CLOSE_BRACE);
    advance(p);


    Expr** exprs = (Expr**)dynbuf_to_arena(p, exprs_start);

    Expr* array_init = new_expr(p, EXPR_COMPOUND_LITERAL, record_ty, compound_lit);
    array_init->compound_lit.len = exprs_len;
    array_init->compound_lit.values = exprs;
    array_init->token_index = init_start_token;

    dynbuf_restore(exprs_start);

    return array_init;
}

Expr* parse_initializer(Parser* p, TyIndex ty) {
    switch (TY_KIND(ty)) {
    case TY_ARRAY:
        return parse_array_initializer(p, ty);
    case TY_STRUCT:
    case TY_STRUCT_PACKED:
        return parse_record_initializer(p, ty);
    default:
        return parse_expr(p);
    }
}

Stmt* parse_var_decl(Parser* p, StorageKind storage) {
    Stmt* decl = new_stmt(p, STMT_VAR_DECL, var_decl);
    
    string identifier = tok_span(p->current);
    Entity* var = get_or_create(p, identifier);
    decl->var_decl.var = var;
    // if (var->storage == STORAGE_EXTERN && storage == STORAGE_PRIVATE) {
    //         parse_error(p, var->decl->token_index, var->decl->token_index, REPORT_NOTE, "previous EXTERN declaration");
    //     parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "previously EXTERN variable cannot be PRIVATE");
    // }

    advance(p);
    expect(p, TOK_COLON);
    advance(p);

    if (!match(p, TOK_EQ)) {
        u32 type_start = p->cursor;
        TyIndex decl_ty = parse_type(p, false);
        if_unlikely (var->storage == STORAGE_EXTERN && !ty_equal(var->ty, decl_ty)) {
            parse_error(p, var->decl->token_index, var->decl->token_index, REPORT_NOTE, "previous EXTERN declaration");
            parse_error(p, type_start, p->cursor - 1, REPORT_ERROR, "type %s differs from EXTERN type %s",
                ty_name(decl_ty), ty_name(var->ty));
        }
        if_unlikely (ty_size(decl_ty) == 0) {
            parse_error(p, type_start, p->cursor - 1, REPORT_ERROR, "variable must have non-zero size");
        }
        var->ty = decl_ty;
        if (match(p, TOK_EQ)) {
            if (storage == STORAGE_EXTERN) {
                parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "EXTERN variable cannot have a value");
            }
            advance(p);
            Expr* value = parse_initializer(p, var->ty);
            decl->var_decl.expr = value;
            if_unlikely (!ty_compatible(decl_ty, value->ty, value->kind == EXPR_LITERAL)) {
                error_at_expr(p, value, REPORT_ERROR, "type %s cannot coerce to %s",
                    ty_name(value->ty), ty_name(decl_ty));
            }
            // if_unlikely (!ty_is_scalar(value->ty)) {
            //     error_at_expr(p, value, REPORT_ERROR, "cannot use non-scalar type %s",
            //         ty_name(value->ty));
            // }
            // this is a global declaration
            if (storage != STORAGE_LOCAL) {
                ensure_linktime_const(p, value);
            }
        }
    } else {
        if (storage == STORAGE_EXTERN) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "EXTERN variable must specify a type");
        }
        advance(p);
        Expr* value = parse_initializer(p, var->ty);
        if_unlikely (var->storage == STORAGE_EXTERN && !ty_compatible(var->ty, value->ty, true)) {
            // parse_error(p, type_start, p->cursor - 1, REPORT_NOTE, "from previous declaration");
            error_at_expr(p, value, REPORT_ERROR, "type %s cannot coerce to EXTERN type %s",
                    ty_name(value->ty), ty_name(var->ty));
        }
        
        // if_unlikely (!ty_is_scalar(value->ty)) {
        //     error_at_expr(p, value, REPORT_ERROR, "cannot use non-scalar type %s",
        //         ty_name(value->ty));
        // }
        // this is a global declaration
        if (storage != STORAGE_LOCAL) {
            // if_unlikely (!is_linktime_const(value)) {
            //     error_at_expr(p, value, REPORT_ERROR, "expression is not link-time constant");
            // }
            ensure_linktime_const(p, value);
        }

        decl->var_decl.expr = value;
        if (var->storage != STORAGE_EXTERN) {
            var->ty = value->ty;
        }
    }

    var->storage = storage;
    var->decl = decl;

    return decl;
}

Stmt* parse_stmt_assign(Parser* p, u8 assign_kind, Expr* left_expr) {
    Stmt* assign = new_stmt(p, assign_kind, assign);
    assign->assign.lhs = left_expr;
    if_unlikely (!is_lvalue(left_expr)) {
        error_at_expr(p, left_expr, REPORT_ERROR, "expression is not an l-value");
    }

    advance(p);
    Expr* value = parse_expr(p);
    if_unlikely (!ty_compatible(left_expr->ty, value->ty, value->kind == EXPR_LITERAL)) {
        error_at_expr(p, value, REPORT_ERROR, "type %s cannot coerce to %s",
            ty_name(value->ty), ty_name(left_expr->ty));
    }
    if_unlikely (!ty_is_scalar(value->ty)) {
        error_at_expr(p, value, REPORT_ERROR, "cannot use non-scalar type %s",
            ty_name(value->ty));
    }
    assign->assign.rhs = value;
    return assign;
}

Stmt* parse_stmt_expr(Parser* p) {
    // expression! who fuckin knows
    u32 start = p->cursor;
    Expr* expr = parse_expr(p);
    if (TOK_EQ <= p->current.kind && p->current.kind <= TOK_RSHIFT_EQ) {
        // assignment statement
        return parse_stmt_assign(p, STMT_ASSIGN + TOK_EQ - p->current.kind, expr);
    } else {
        // expression statement
        if_unlikely (expr->kind != EXPR_CALL && expr->ty != TY_VOID) {
            error_at_expr(p, expr, REPORT_WARNING, "unused expression result");
        }
        Stmt* stmt_expr = new_stmt(p, STMT_EXPR, expr);
        stmt_expr->expr = expr;
        stmt_expr->token_index = start;
        if_likely(expr->kind == EXPR_CALL) {
            TyFn* fn_ty;
            if (TY_KIND(expr->call.callee->ty) == TY_PTR) {
                fn_ty = TY(TY(expr->call.callee->ty, TyPtr)->to, TyFn);
            } else {
                fn_ty = TY(expr->call.callee->ty, TyFn);
            }

            if (fn_ty->is_noreturn) {
                stmt_expr->retkind = RETKIND_YES;
            }
        }
        return stmt_expr;
    }
}

static StmtList parse_stmt_block(Parser* p) {
    u32 stmts_start = dynbuf_start();
    u32 stmts_len = 0;
    ReturnKind retkind = RETKIND_NO;
    while (!match(p, TOK_KW_END)) {
        Stmt* stmt = parse_stmt(p);
        if (stmt == nullptr) {
            continue;
        }
        retkind = max(retkind, stmt->retkind);
        vec_append(&dynbuf, stmt);
        stmts_len++;
    }
    Stmt** stmts = (Stmt**)dynbuf_to_arena(p, stmts_start);
    dynbuf_restore(stmts_start);

    return (StmtList){
        .stmts = stmts,
        .len = stmts_len,
        .retkind = retkind,
    };
}

static Stmt* parse_do_stmt(Parser* p) {
    expect(p, TOK_KW_DO);
    Stmt* block = new_stmt(p, STMT_BLOCK, block);
    advance(p);

    enter_scope(p);
    block->block = parse_stmt_block(p);
    advance(p);

    exit_scope(p);
    return block;
}

static Stmt* parse_while(Parser* p) {
    Stmt* while_ = new_stmt(p, STMT_WHILE, while_);
    advance(p);
    Expr* cond = parse_expr(p);

    if_unlikely (!ty_is_scalar(cond->ty)) {
        error_at_expr(p, cond, REPORT_ERROR, "condition must be scalar");
    }
    expect(p, TOK_KW_DO);
    advance(p);

    enter_scope(p);
    while_->while_.block = parse_stmt_block(p);
    while_->retkind = while_->while_.block.retkind;
    // if (cond->kind == EXPR_LITERAL && cond->literal) {
    //     while_->retkind = RETKIND_YES;
    // }

    advance(p);

    exit_scope(p);
    return while_;
}

static StmtList parse_if_block_(Parser* p) {
    u32 stmts_start = dynbuf_start();
    u32 stmts_len = 0;
    ReturnKind retkind = RETKIND_NO;
    while (true) {
        switch (p->current.kind) {
        case TOK_KW_END:
        case TOK_KW_ELSE:
        case TOK_KW_ELSEIF:
            goto end;
        }
        Stmt* stmt = parse_stmt(p);
        if (stmt == nullptr) {
            continue;
        }
        retkind = max(retkind, stmt->retkind);
        vec_append(&dynbuf, stmt);
        stmts_len++;
    }
    end:
    ;
    Stmt** stmts = (Stmt**)dynbuf_to_arena(p, stmts_start);
    dynbuf_restore(stmts_start);

    return (StmtList){
        .stmts = stmts,
        .len = stmts_len,
        .retkind = retkind,
    };
}

Stmt* parse_stmt_if(Parser* p) {
    Stmt* if_ = new_stmt(p, STMT_IF, if_);
    advance(p);

    Expr* cond = parse_expr(p);
    if_unlikely (!ty_is_scalar(cond->ty)) {
        error_at_expr(p, cond, REPORT_ERROR, "condition must be scalar");
    }
    if_->if_.cond = cond;
    expect(p, TOK_KW_THEN);
    advance(p);
    enter_scope(p);
    StmtList if_true = parse_if_block_(p);
    exit_scope(p);
    if_->if_.block = if_true;
    // if_->retkind = if_true.retkind;
    Stmt* if_false = nullptr;
    switch (p->current.kind) {
    case TOK_KW_END:
        advance(p);
        break;
    case TOK_KW_ELSE:
        if_false = new_stmt(p, STMT_BLOCK, block);
        advance(p);
        if_false->block = parse_stmt_block(p);
        if_false->retkind = if_false->block.retkind;
        advance(p);
        break;
    case TOK_KW_ELSEIF:
        if_false = parse_stmt_if(p);
        break;
    }
    
    if (if_true.retkind == RETKIND_YES && if_false && if_false->retkind == RETKIND_YES) {
        if_->retkind = RETKIND_YES;
    } else if (if_true.retkind == RETKIND_MAYBE || (if_false && if_false->retkind == RETKIND_MAYBE)) {
        if_->retkind = RETKIND_MAYBE;
    } else {
        if_->retkind = RETKIND_NO;
    }
    if_->if_.else_ = if_false;
    return if_;
}

Stmt* parse_stmt(Parser* p) {
    switch (p->current.kind) {
    case TOK_KW_LEAVE: {
        TyFn* current_fn = TY(p->current_function->ty, TyFn);
        TyIndex ret_ty = current_fn->ret_ty;
        if (ret_ty != TY_VOID) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "cannot LEAVE from non-VOID function");
        }
        if (current_fn->is_noreturn) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "cannot LEAVE from NORETURN function");
        }
        Stmt* leave = new_stmt(p, STMT_LEAVE, nothing);
        leave->retkind = RETKIND_YES;
        advance(p);
        return leave;
    }
    case TOK_KW_RETURN: {
        TyFn* current_fn = TY(p->current_function->ty, TyFn);
        TyIndex ret_ty = current_fn->ret_ty;
        Stmt* return_ = new_stmt(p, STMT_RETURN, expr);
        if (current_fn->is_noreturn) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "cannot RETURN from NORETURN function");
        }
        if (ret_ty == TY_VOID) {
            if (p->flags.xrsdk) {
                parse_error(p, p->cursor, p->cursor, REPORT_WARNING, "void RETURN is not XR/SDK compatible");
            }
            if (!has_eof_or_nl(p, p->cursor + 1)) {
                parse_error(p, p->cursor, p->cursor, REPORT_WARNING, "misleading whitespace - void RETURN statement stops here");
            }
            advance(p);
        } else {
            advance(p);
            return_->expr = parse_expr(p);
            if (!ty_compatible(ret_ty, return_->expr->ty, return_->expr->kind == EXPR_LITERAL)) {
                error_at_expr(p, return_->expr, REPORT_ERROR, "type %s cannot coerce to %s",
                    ty_name(return_->expr->ty), ty_name(ret_ty));
            }
        }
        return_->retkind = RETKIND_YES;
        return return_;
    }
    case TOK_KW_UNREACHABLE:
        Stmt* unreachable_ = new_stmt(p, STMT_UNREACHABLE, nothing);
        unreachable_->retkind = RETKIND_YES;
        advance(p);
        return unreachable_;
    case TOK_KW_BREAK:
        Stmt* continue_ = new_stmt(p, STMT_CONTINUE, nothing);
        advance(p);
        return continue_;
    case TOK_KW_CONTINUE:
        Stmt* break_ = new_stmt(p, STMT_BREAK, nothing);
        advance(p);
        return break_;
    case TOK_IDENTIFIER:
        if (peek(p, 1).kind == TOK_COLON) {
            return parse_var_decl(p, STORAGE_LOCAL);
        } else {
            return parse_stmt_expr(p);
        }
        break;
    case TOK_KW_NOTHING:
        advance(p);
        return nullptr; // nothing
    case TOK_KW_IF:
        return parse_stmt_if(p);
    case TOK_KW_WHILE:
        return parse_while(p);
    case TOK_KW_DO:
        return parse_do_stmt(p);
    case TOK_KW_PUBLIC:
    case TOK_KW_PRIVATE:
    case TOK_KW_EXTERN:
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
            "storage class cannot be used locally");
        break;
    case TOK_KW_TYPE:
    case TOK_KW_STRUCT:
    case TOK_KW_UNION:
    case TOK_KW_ENUM:
    case TOK_KW_FNPTR:
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
            "cannot locally declare type");
        break;
    case TOK_KW_FN:
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
            "cannot locally declare function");
        break;
    default:
        return parse_stmt_expr(p);
    }
    return nullptr;
}

Entity* get_incomplete_type_entity(Parser* p, string identifier) {
    Entity* entity = get_entity(p, identifier);
    if (!entity) {
        entity = new_entity(p, identifier, ENTKIND_TYPE);
        entity->ty = ty_allocate(TyAlias);
        TY_KIND(entity->ty) = TY_ALIAS_INCOMPLETE;
        TY(entity->ty, TyAlias)->entity = entity;
    } else if (entity->kind != ENTKIND_TYPE) {
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
            "symbol already exists");
    }
    return entity;
}

TyIndex parse_fn_prototype(Parser* p) {
    ArenaState a_save = arena_save(&p->arena);

    usize params_len = 0;
    // temporarily allocate a bunch of function params
    Ty_FnParam* params = arena_alloc(&p->arena, sizeof(Ty_FnParam) * TY_FN_MAX_PARAMS, alignof(Ty_FnParam));

    bool is_variadic = false;

    expect(p,TOK_OPEN_PAREN);
    advance(p);
    while (p->current.kind != TOK_CLOSE_PAREN) {
        Ty_FnParam* param = &params[params_len];
        if (params_len > TY_FN_MAX_PARAMS) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "too many parameters (max 32)");
        }

        if (match(p, TOK_VARARG)) {
            is_variadic = true;
            advance(p);
            expect(p, TOK_IDENTIFIER);
            string argv = tok_span(p->current);
            for_n(i, 0, params_len) {
                if (string_eq(from_compact(params[i].name), argv)) {
                    parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "parameter name already used");
                }
            }
            param->varargs.argv = to_compact(argv);
            advance(p);
            expect(p, TOK_IDENTIFIER);
            string argc = tok_span(p->current);
            for_n(i, 0, params_len) {
                if (string_eq(from_compact(params[i].name), argc)) {
                    parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "parameter name already used");
                }
            }
            if (string_eq(argv, argc)) {
                parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "parameter name already used");
            }
            param->varargs.argc = to_compact(argc);
            advance(p);
            params_len++;
            break;
        }

        param->out = false;
        switch (p->current.kind) {
        case TOK_KW_IN:
            advance(p);
            break;
        case TOK_KW_OUT:
            param->out = true;
            advance(p);
            break;
        default:
            if (p->flags.xrsdk) {
                parse_error(p, p->cursor, p->cursor, REPORT_WARNING, "implicit IN is not XR/SDK compatible");
            }
        }

        expect(p, TOK_IDENTIFIER);
        string ident = tok_span(p->current);
        for_n(i, 0, params_len) {
            if (string_eq(from_compact(params[i].name), ident)) {
                parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "parameter name already used");
            }
        }

        param->name = to_compact(ident);

        advance(p);
        expect(p, TOK_COLON);
        advance(p);

        u32 ty_begin = p->cursor;
        param->ty = parse_type(p, false);
        if_unlikely (!ty_is_scalar(param->ty)) {
            parse_error(p, ty_begin, p->cursor - 1, REPORT_ERROR, "cannot use non-scalar type %s", ty_name(param->ty));
        }

        params_len++;

        if (match(p, TOK_COMMA)) {
            advance(p);
        } else {
            break;
        }
    }
    expect(p, TOK_CLOSE_PAREN);
    advance(p);

    TyIndex ret_ty = TY_VOID;

    bool is_noreturn = false;

    if (match(p, TOK_COLON)) {
        advance(p);
        if (match(p, TOK_KW_NORETURN)) {
            if (p->flags.xrsdk) {
                parse_error(p, p->cursor, p->cursor, REPORT_WARNING, "NORETURN is not XR/SDK compatible");
            }
            is_noreturn = true;
            advance(p);
        } else {
            u32 ty_begin = p->cursor;
            ret_ty = parse_type(p, false);
            if_unlikely (!ty_is_scalar(ret_ty)) {
                parse_error(p, ty_begin, p->cursor - 1, REPORT_ERROR, "cannot use non-scalar type %s", ty_name(ret_ty));
            }
        }
    }

    // allocate final type
    TyIndex proto = ty__allocate(sizeof(TyFn) + sizeof(Ty_FnParam) * params_len, max(alignof(TyFn), alignof(Ty_FnParam)) == 8);
    TyFn* fn = TY(proto, TyFn);
    fn->kind = TY_FN;
    fn->len = params_len;
    fn->variadic = is_variadic;
    fn->is_noreturn = is_noreturn;
    fn->ret_ty = ret_ty;
    memcpy(fn->params, params, sizeof(Ty_FnParam) * params_len);

    arena_restore(&p->arena, a_save);
    return proto;
}

Stmt* parse_fn_decl(Parser* p, u8 storage) {
    // advance(p);
    advance(p);

    TyIndex fnptr_ty = TY__INVALID;
    u32 fnptr_decl_loc = 0;
    if (match(p, TOK_OPEN_PAREN)) {
        advance(p);
        expect(p, TOK_IDENTIFIER);
        u32 ty_loc = p->cursor;
        Entity* ty_entity = get_entity(p, tok_span(p->current));
        TyIndex fnptr = parse_type(p, false);
        if (TY_KIND(fnptr) != TY_PTR) {
            parse_error(p, ty_loc, ty_loc, REPORT_ERROR, "provided type %s is not an FNPTR", ty_name(fnptr));
        }
        fnptr_ty = TY(fnptr, TyPtr)->to;
        if (TY_KIND(fnptr_ty) != TY_FN) {
            parse_error(p, ty_loc, ty_loc, REPORT_ERROR, "provided type %s is not an FNPTR", ty_name(fnptr));
        }
        fnptr_decl_loc = ty_entity->decl->token_index;
        expect(p, TOK_CLOSE_PAREN);
        advance(p);
    }

    expect(p, TOK_IDENTIFIER);
    u32 ident_pos = p->cursor;
    string identifier = tok_span(p->current);
    Entity* fn = get_or_create(p, identifier);
    // if (fn->storage == STORAGE_EXTERN && storage == STORAGE_PRIVATE) {
    //     parse_error(p, fn->decl->token_index, fn->decl->token_index, REPORT_NOTE, "previous EXTERN declaration");
    //     parse_error(p, ident_pos, ident_pos, REPORT_ERROR, "previously EXTERN function cannot be PRIVATE");
    // }
    advance(p);
    TyIndex decl_ty = parse_fn_prototype(p);
    if (fn->ty == TY__INVALID) {
        fn->ty = decl_ty;
    }
    if (fn->storage == STORAGE_EXTERN && !ty_equal(fn->ty, decl_ty)) {
        parse_error(p, fn->decl->token_index, fn->decl->token_index, REPORT_NOTE, "previous EXTERN declaration");
        parse_error(p, ident_pos, ident_pos, REPORT_ERROR, "type differs from previous EXTERN type");
    }
    if (fnptr_ty != TY__INVALID && !ty_equal(fnptr_ty, decl_ty)) {
        parse_error(p, fnptr_decl_loc, fnptr_decl_loc, REPORT_NOTE, "from FNPTR declaration");
        parse_error(p, ident_pos, ident_pos, REPORT_ERROR, "type differs from provided FNPTR type");
    }
    if (fnptr_ty != TY__INVALID) {
        decl_ty = fnptr_ty;
    }

    fn->ty = decl_ty;
    if (storage != STORAGE_EXTERN) {
        Stmt* fn_decl = new_stmt(p, STMT_FN_DECL, fn_decl);
        fn_decl->fn_decl.fn = fn;
        fn->decl = fn_decl;

        p->current_function = fn;

        enter_scope(p);

        // define parameters
        TyFn* fn_type = TY(decl_ty, TyFn);
        for_n(i, 0, fn_type->len - 1) {
            Ty_FnParam* param = &fn_type->params[i];
            Entity* param_entity = new_entity(p, from_compact(param->name), ENTKIND_VAR);
            param_entity->ty = param->ty;
            param_entity->storage = param->out ? STORAGE_OUT_PARAM : STORAGE_LOCAL;
        }
        if (fn_type->variadic) {
            Ty_FnParam* param = &fn_type->params[fn_type->len - 1];
            Entity* argv_entity = new_entity(p, from_compact(param->varargs.argv), ENTKIND_VAR);
            argv_entity->ty = ty_get_ptr(TY_VOIDPTR);
            argv_entity->storage = STORAGE_LOCAL;
            Entity* argc_entity = new_entity(p, from_compact(param->varargs.argc), ENTKIND_VAR);
            argc_entity->storage = STORAGE_LOCAL;
            argc_entity->ty = target_uword;
        } else if (fn_type->len != 0) {
            Ty_FnParam* param = &fn_type->params[fn_type->len - 1];
            Entity* param_entity = new_entity(p, from_compact(param->name), ENTKIND_VAR);
            param_entity->ty = param->ty;
            param_entity->storage = param->out ? STORAGE_OUT_PARAM : STORAGE_LOCAL;
        }

        u32 stmts_start = dynbuf_start();
        u32 stmts_len = 0;
        bool has_returned = false;
        bool has_warned = false;
        while (!match(p, TOK_KW_END)) {
            Stmt* stmt = parse_stmt(p);
            if (stmt == nullptr) {
                continue;
            }
            if (has_returned && !has_warned && stmt->kind != STMT_UNREACHABLE) {
                // parse_error(p, ident_pos, ident_pos, REPORT_NOTE, "in function '"str_fmt"'", str_arg(identifier));
                parse_error(p, stmt->token_index, stmt->token_index, REPORT_WARNING, "dead code after control flow diverges");
                has_warned = true;
            }

            if (stmt->retkind == RETKIND_YES) {
                has_returned = true;
            }
            vec_append(&dynbuf, stmt);
            stmts_len++;
        }
        Stmt** stmts = (Stmt**)dynbuf_to_arena(p, stmts_start);
        dynbuf_restore(stmts_start);

        if (!has_returned && fn_type->is_noreturn) {
            parse_error(p, ident_pos, ident_pos, REPORT_NOTE, "in function '"str_fmt"'", str_arg(identifier));
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "control might reach end of NORETURN function");
        }

        if (!has_returned && fn_type->ret_ty != TY_VOID) {
            parse_error(p, ident_pos, ident_pos, REPORT_NOTE, "in function '"str_fmt"'", str_arg(identifier));
            parse_error(p, p->cursor, p->cursor, REPORT_WARNING, "function may not return with defined value");
        }
        advance(p);

        exit_scope(p);

        p->current_function = nullptr;

        fn_decl->fn_decl.body.stmts = stmts;
        fn_decl->fn_decl.body.len = stmts_len;
    } else {
        fn->decl = new_stmt(p, STMT_DECL_LOCATION, nothing);
        fn->decl->token_index = ident_pos;
    }
    fn->storage = storage;
    return nullptr;
}

static bool nuh_uh_recursive_alias(TyIndex t, TyIndex err_on) {
    if (t == err_on) {
        return true;
    }

    switch (TY_KIND(t)) {
    case TY_PTR:
        return nuh_uh_recursive_alias(TY(t, TyPtr)->to, err_on);
    case TY_ARRAY:
        return nuh_uh_recursive_alias(TY(t, TyArray)->to, err_on);
    case TY_ALIAS:
        return nuh_uh_recursive_alias(TY(t, TyAlias)->aliasing, err_on);
    default:
        return false;
    }
}

static inline uintptr_t align_forward(uintptr_t ptr, uintptr_t align) {
    return (ptr + align - 1) & ~(align - 1);
}

TyIndex parse_record_decl(Parser* p, TyKind kind) {
    ArenaState a_save = arena_save(&p->arena);

    usize record_align = 1;

    usize params_len = 0;
    // temporarily allocate a bunch of members
    Ty_RecordMember* members = arena_alloc(&p->arena, sizeof(Ty_RecordMember) * TY_RECORD_MAX_MEMBERS, alignof(Ty_RecordMember));

    usize max_size = 0;
    usize offset = 0;
    usize n = 0;
    for (; p->current.kind != TOK_KW_END; n++) {

        expect(p, TOK_IDENTIFIER);
        if (n >= TY_RECORD_MAX_MEMBERS) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "too many fields (max 255)");
        }
        Ty_RecordMember* member = &members[n];
        string member_name = tok_span(p->current);

        for_n(i, 0, n) {
            CompactString name = members[i].name;
            if (string_eq(from_compact(name), member_name)) {
                parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "duplicate member name");
            }
        }

        member->name = to_compact(member_name);

        advance(p);
        expect(p, TOK_COLON);
        advance(p);

        TyIndex member_ty = parse_type(p, false);
        usize member_size = ty_size(member_ty);
        usize member_align = ty_align(member_ty);
        if (kind != TY_STRUCT_PACKED) {
            record_align = max(record_align, member_align);
        }

        if (kind == TY_STRUCT) {
            offset = align_forward(offset, member_align);
        }
        member->offset = offset;
        member->type = member_ty;

        max_size = max(max_size, member_size);
        if (kind != TY_UNION) {
            offset += member_size;
        }

        if (match(p, TOK_COMMA)) {
            advance(p);
            continue;
        } else {
            break;
        }
    }
    expect(p, TOK_KW_END);
    advance(p);

    TyIndex record_index = ty__allocate(sizeof(TyRecord) + sizeof(Ty_RecordMember) * n, max(alignof(TyRecord), alignof(Ty_RecordMember)));
    TyRecord* record = TY(record_index, TyRecord);
    record->align = record_align;
    record->kind = kind;
    if (kind == TY_UNION) {
        record->size = max_size;
    } else {
        if (kind == TY_STRUCT) {
            align_forward(offset, record_align);
        }
        record->size = offset;
    }
    record->len = n;
    memcpy(record->members, members, sizeof(Ty_RecordMember) * n);

    arena_restore(&p->arena, a_save);
    return record_index;
}

TyIndex parse_enum_decl(Parser* p) {
    expect(p, TOK_COLON);
    advance(p);

    u32 ty_begin = p->cursor;
    TyIndex backing_ty = parse_type(p, false);
    TyIndex real_backing_ty = ty_unwrap_alias(backing_ty);
    if_unlikely (!ty_is_integer(real_backing_ty)) {
        parse_error(p, ty_begin, p->cursor - 1, REPORT_ERROR, "ENUM backing type must be an integer");
    }


    TyIndex enum_ty = ty_allocate(TyEnum);
    TY(enum_ty, TyEnum)->kind = TY_ENUM;
    TY(enum_ty, TyEnum)->backing_ty = backing_ty;
    // UNREACHABLE;

    ArenaState save = arena_save(&p->arena);

    u64 running_value = 0;
    while (!match(p, TOK_KW_END)) {
        expect(p, TOK_IDENTIFIER);
        string span = tok_span(p->current);
        if_unlikely (get_entity(p, span)) {
            parse_error(p, p->cursor, p->cursor, REPORT_ERROR, "symbol already exists");
        }

        advance(p);
        if (match(p, TOK_EQ)) {
            advance(p);
            Expr* value = parse_expr(p);
            if (value->kind != EXPR_LITERAL) {
                error_at_expr(p, value, REPORT_ERROR, "expected a constant integer expression");
            }
            running_value = value->literal;
        }

        Entity* entity = new_entity(p, span, ENTKIND_VARIANT);
        entity->kind = ENTKIND_VARIANT;
        entity->ty = enum_ty;
        entity->variant_value = running_value;

        if (match(p, TOK_COMMA)) {
            advance(p);
            running_value++;
            continue;
        } else {
            break;
        }
    }
    expect(p, TOK_KW_END);
    advance(p);

    arena_restore(&p->arena, save);

    return enum_ty;
}

void parse_global_decl(Parser* p) {
    switch (p->current.kind) {
    case TOK_KW_NOTHING:
        advance(p);
        parse_global_decl(p);
        return;
    case TOK_KW_TYPE: {
        advance(p);
        Stmt* typedecl_loc = new_stmt(p, STMT_DECL_LOCATION, nothing);
        u32 identifier_pos = p->cursor;
        expect(p, TOK_IDENTIFIER);
        string identifier = tok_span(p->current);
        Entity* entity = get_incomplete_type_entity(p, identifier);
        advance(p);
        expect(p, TOK_COLON);
        advance(p);
        // TY_KIND(entity->ty) = TY_ALIAS_IN_PROGRESS;
        TyIndex aliased_ty = parse_type(p, false);
        if (nuh_uh_recursive_alias(aliased_ty, entity->ty)) {
            parse_error(p, identifier_pos, identifier_pos, REPORT_ERROR, "TYPE aliases cannot be recursive");
        }
        TY(entity->ty, TyAlias)->aliasing = aliased_ty;
        TY(entity->ty, TyAlias)->entity = entity;
        TY_KIND(entity->ty) = TY_ALIAS;
        entity->decl = typedecl_loc;
    } break;
    case TOK_KW_FNPTR: {
        advance(p);
        Stmt* typedecl_loc = new_stmt(p, STMT_DECL_LOCATION, nothing);
        expect(p, TOK_IDENTIFIER);
        string identifier = tok_span(p->current);
        Entity* entity = get_incomplete_type_entity(p, identifier);
        advance(p);
        TyIndex fn_ty = parse_fn_prototype(p);
        TyIndex fnptr = ty_get_ptr(fn_ty);
        TY(entity->ty, TyAlias)->aliasing = fnptr;
        TY(entity->ty, TyAlias)->entity = entity;
        TY_KIND(entity->ty) = TY_ALIAS;
        entity->decl = typedecl_loc;
        break;
    }
    case TOK_KW_ENUM: {
        Stmt* typedecl_loc = new_stmt(p, STMT_DECL_LOCATION, nothing);
        advance(p);
        expect(p, TOK_IDENTIFIER);
        string identifier = tok_span(p->current);
        Entity* entity = get_incomplete_type_entity(p, identifier);
        advance(p);
        TyIndex enum_ = parse_enum_decl(p);

        TY(entity->ty, TyAlias)->aliasing = enum_;
        TY(entity->ty, TyAlias)->entity = entity;
        TY_KIND(entity->ty) = TY_ALIAS;
        entity->decl = typedecl_loc;
        break;
    }
    case TOK_KW_STRUCT: {
        // hello there
        Stmt* typedecl_loc = new_stmt(p, STMT_DECL_LOCATION, nothing);
        advance(p);
        TyKind kind = TY_STRUCT;
        if (match(p, TOK_KW_PACKED)) {
            kind = TY_STRUCT_PACKED;
            advance(p);
        }

        expect(p, TOK_IDENTIFIER);
        string identifier = tok_span(p->current);
        Entity* entity = get_incomplete_type_entity(p, identifier);
        advance(p);
        TyIndex record = parse_record_decl(p, kind);

        TY(entity->ty, TyAlias)->aliasing = record;
        TY(entity->ty, TyAlias)->entity = entity;
        TY_KIND(entity->ty) = TY_ALIAS;
        entity->decl = typedecl_loc;
        break;
    }
    case TOK_KW_UNION: {
        Stmt* typedecl_loc = new_stmt(p, STMT_DECL_LOCATION, nothing);
        advance(p);

        expect(p, TOK_IDENTIFIER);
        string identifier = tok_span(p->current);
        Entity* entity = get_incomplete_type_entity(p, identifier);
        advance(p);
        TyIndex record = parse_record_decl(p, TY_UNION);
        // printf("union size %llu align %llu\n", ty_size(record), ty_align(record));

        TY(entity->ty, TyAlias)->aliasing = record;
        TY(entity->ty, TyAlias)->entity = entity;
        TY_KIND(entity->ty) = TY_ALIAS;
        entity->decl = typedecl_loc;
        break;
    }
    case TOK_KW_PUBLIC:
    case TOK_KW_PRIVATE:
    case TOK_KW_EXPORT:
    case TOK_KW_EXTERN:
        ;
        u64 effective_storage = p->current.kind - TOK_KW_PUBLIC + STORAGE_PUBLIC;
        advance(p);
        if (p->current.kind == TOK_KW_FN) {
            parse_fn_decl(p, effective_storage);
            break;
        }
        parse_var_decl(p, effective_storage);
        break;
    case TOK_IDENTIFIER:
        parse_var_decl(p, STORAGE_PRIVATE);
        break;
    case TOK_KW_FN:
        parse_fn_decl(p, STORAGE_PUBLIC);
        break;
    default:
        parse_error(p, p->cursor, p->cursor, REPORT_ERROR, 
            "expected global declaration");
    }
}

CompilationUnit parse_unit(Parser* p) {
    ty_init();

    global_scope = p->global_scope;

    dynbuf = vecptr_new(void, 256);

    while (p->current.kind != TOK_EOF) {
        parse_global_decl(p);
    }

    CompilationUnit cu = {};
    cu.tokens = p->tokens;
    cu.tokens_len = p->tokens_len;
    cu.sources = p->sources;
    cu.top_scope = p->global_scope;
    cu.arena = p->arena;

    vec_destroy(&dynbuf);

    return cu;
}
