#include "../include/son_sema.h"

static SonSymbol* alloc_symbol(SonArena *arena);
static bool define_symbol(SonScope *scope, SonSymbol *sym);

void son_sema_init(SonSema *sema, SonArena *arena) {
    sema->arena = arena;
    sema->has_error = false;
    sema->error_count = 0;
    sema->current_module = NULL;
    sema->current_expected_type = NULL;

    sema->global_scope = (SonScope*)son_arena_alloc(arena, sizeof(SonScope), sizeof(void*));
    memset(sema->global_scope, 0, sizeof(SonScope));
    sema->global_scope->arena = arena;
    sema->current_scope = sema->global_scope;

    // Built-in OS and Runtime symbols
    SonDecl *decl_wf = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_wf, 0, sizeof(SonDecl));
    decl_wf->kind = DECL_FUNCTION;
    decl_wf->name = son_slice_from_cstr("son_fs_write_file");
    decl_wf->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_wf->func.return_type->kind = TYPE_BOOL;
    SonSymbol *sym_wf = alloc_symbol(arena);
    sym_wf->kind = SYM_FUNCTION;
    sym_wf->name = decl_wf->name;
    sym_wf->type = decl_wf->func.return_type;
    sym_wf->decl = decl_wf;
    define_symbol(sema->global_scope, sym_wf);

    SonDecl *decl_wfl = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_wfl, 0, sizeof(SonDecl));
    decl_wfl->kind = DECL_FUNCTION;
    decl_wfl->name = son_slice_from_cstr("son_fs_write_file_len");
    decl_wfl->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_wfl->func.return_type->kind = TYPE_BOOL;
    SonSymbol *sym_wfl = alloc_symbol(arena);
    sym_wfl->kind = SYM_FUNCTION;
    sym_wfl->name = decl_wfl->name;
    sym_wfl->type = decl_wfl->func.return_type;
    sym_wfl->decl = decl_wfl;
    define_symbol(sema->global_scope, sym_wfl);

    SonDecl *decl_fsz = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_fsz, 0, sizeof(SonDecl));
    decl_fsz->kind = DECL_FUNCTION;
    decl_fsz->name = son_slice_from_cstr("son_fs_file_size");
    decl_fsz->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_fsz->func.return_type->kind = TYPE_I64;
    SonSymbol *sym_fsz = alloc_symbol(arena);
    sym_fsz->kind = SYM_FUNCTION;
    sym_fsz->name = decl_fsz->name;
    sym_fsz->type = decl_fsz->func.return_type;
    sym_fsz->decl = decl_fsz;
    define_symbol(sema->global_scope, sym_fsz);

    SonDecl *decl_frd = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_frd, 0, sizeof(SonDecl));
    decl_frd->kind = DECL_FUNCTION;
    decl_frd->name = son_slice_from_cstr("son_fs_read_file");
    decl_frd->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_frd->func.return_type->kind = TYPE_I64;
    SonSymbol *sym_frd = alloc_symbol(arena);
    sym_frd->kind = SYM_FUNCTION;
    sym_frd->name = decl_frd->name;
    sym_frd->type = decl_frd->func.return_type;
    sym_frd->decl = decl_frd;
    define_symbol(sema->global_scope, sym_frd);

    SonDecl *decl_sys = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_sys, 0, sizeof(SonDecl));
    decl_sys->kind = DECL_FUNCTION;
    decl_sys->name = son_slice_from_cstr("son_os_system");
    decl_sys->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_sys->func.return_type->kind = TYPE_I32;
    SonSymbol *sym_sys = alloc_symbol(arena);
    sym_sys->kind = SYM_FUNCTION;
    sym_sys->name = decl_sys->name;
    sym_sys->type = decl_sys->func.return_type;
    sym_sys->decl = decl_sys;
    define_symbol(sema->global_scope, sym_sys);

    SonDecl *decl_tnt = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_tnt, 0, sizeof(SonDecl));
    decl_tnt->kind = DECL_FUNCTION;
    decl_tnt->name = son_slice_from_cstr("son_time_now_ticks");
    decl_tnt->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_tnt->func.return_type->kind = TYPE_I64;
    SonSymbol *sym_tnt = alloc_symbol(arena);
    sym_tnt->kind = SYM_FUNCTION;
    sym_tnt->name = decl_tnt->name;
    sym_tnt->type = decl_tnt->func.return_type;
    sym_tnt->decl = decl_tnt;
    define_symbol(sema->global_scope, sym_tnt);

    SonDecl *decl_tf = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_tf, 0, sizeof(SonDecl));
    decl_tf->kind = DECL_FUNCTION;
    decl_tf->name = son_slice_from_cstr("son_time_frequency");
    decl_tf->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_tf->func.return_type->kind = TYPE_I64;
    SonSymbol *sym_tf = alloc_symbol(arena);
    sym_tf->kind = SYM_FUNCTION;
    sym_tf->name = decl_tf->name;
    sym_tf->type = decl_tf->func.return_type;
    sym_tf->decl = decl_tf;
    define_symbol(sema->global_scope, sym_tf);

    SonDecl *decl_slp = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_slp, 0, sizeof(SonDecl));
    decl_slp->kind = DECL_FUNCTION;
    decl_slp->name = son_slice_from_cstr("son_sleep_ms");
    decl_slp->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_slp->func.return_type->kind = TYPE_VOID;
    SonSymbol *sym_slp = alloc_symbol(arena);
    sym_slp->kind = SYM_FUNCTION;
    sym_slp->name = decl_slp->name;
    sym_slp->type = decl_slp->func.return_type;
    sym_slp->decl = decl_slp;
    define_symbol(sema->global_scope, sym_slp);

    SonDecl *decl_ha = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_ha, 0, sizeof(SonDecl));
    decl_ha->kind = DECL_FUNCTION;
    decl_ha->name = son_slice_from_cstr("son_heap_alloc");
    decl_ha->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_ha->func.return_type->kind = TYPE_USIZE;
    SonSymbol *sym_ha = alloc_symbol(arena);
    sym_ha->kind = SYM_FUNCTION;
    sym_ha->name = decl_ha->name;
    sym_ha->type = decl_ha->func.return_type;
    sym_ha->decl = decl_ha;
    define_symbol(sema->global_scope, sym_ha);

    SonDecl *decl_hf = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_hf, 0, sizeof(SonDecl));
    decl_hf->kind = DECL_FUNCTION;
    decl_hf->name = son_slice_from_cstr("son_heap_free");
    decl_hf->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_hf->func.return_type->kind = TYPE_VOID;
    SonSymbol *sym_hf = alloc_symbol(arena);
    sym_hf->kind = SYM_FUNCTION;
    sym_hf->name = decl_hf->name;
    sym_hf->type = decl_hf->func.return_type;
    sym_hf->decl = decl_hf;
    define_symbol(sema->global_scope, sym_hf);

    // Socket PAL Runtime symbols
    SonDecl *decl_ninit = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_ninit, 0, sizeof(SonDecl));
    decl_ninit->kind = DECL_FUNCTION;
    decl_ninit->name = son_slice_from_cstr("son_net_init");
    decl_ninit->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_ninit->func.return_type->kind = TYPE_I32;
    SonSymbol *sym_ninit = alloc_symbol(arena);
    sym_ninit->kind = SYM_FUNCTION;
    sym_ninit->name = decl_ninit->name;
    sym_ninit->type = decl_ninit->func.return_type;
    sym_ninit->decl = decl_ninit;
    define_symbol(sema->global_scope, sym_ninit);

    SonDecl *decl_nlist = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_nlist, 0, sizeof(SonDecl));
    decl_nlist->kind = DECL_FUNCTION;
    decl_nlist->name = son_slice_from_cstr("son_net_listen");
    decl_nlist->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_nlist->func.return_type->kind = TYPE_I64;
    SonSymbol *sym_nlist = alloc_symbol(arena);
    sym_nlist->kind = SYM_FUNCTION;
    sym_nlist->name = decl_nlist->name;
    sym_nlist->type = decl_nlist->func.return_type;
    sym_nlist->decl = decl_nlist;
    define_symbol(sema->global_scope, sym_nlist);

    SonDecl *decl_nacc = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_nacc, 0, sizeof(SonDecl));
    decl_nacc->kind = DECL_FUNCTION;
    decl_nacc->name = son_slice_from_cstr("son_net_accept");
    decl_nacc->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_nacc->func.return_type->kind = TYPE_I64;
    SonSymbol *sym_nacc = alloc_symbol(arena);
    sym_nacc->kind = SYM_FUNCTION;
    sym_nacc->name = decl_nacc->name;
    sym_nacc->type = decl_nacc->func.return_type;
    sym_nacc->decl = decl_nacc;
    define_symbol(sema->global_scope, sym_nacc);

    SonDecl *decl_nsend = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_nsend, 0, sizeof(SonDecl));
    decl_nsend->kind = DECL_FUNCTION;
    decl_nsend->name = son_slice_from_cstr("son_net_send");
    decl_nsend->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_nsend->func.return_type->kind = TYPE_I32;
    SonSymbol *sym_nsend = alloc_symbol(arena);
    sym_nsend->kind = SYM_FUNCTION;
    sym_nsend->name = decl_nsend->name;
    sym_nsend->type = decl_nsend->func.return_type;
    sym_nsend->decl = decl_nsend;
    define_symbol(sema->global_scope, sym_nsend);

    SonDecl *decl_nrecv = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_nrecv, 0, sizeof(SonDecl));
    decl_nrecv->kind = DECL_FUNCTION;
    decl_nrecv->name = son_slice_from_cstr("son_net_recv");
    decl_nrecv->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_nrecv->func.return_type->kind = TYPE_I32;
    SonSymbol *sym_nrecv = alloc_symbol(arena);
    sym_nrecv->kind = SYM_FUNCTION;
    sym_nrecv->name = decl_nrecv->name;
    sym_nrecv->type = decl_nrecv->func.return_type;
    sym_nrecv->decl = decl_nrecv;
    define_symbol(sema->global_scope, sym_nrecv);

    SonDecl *decl_ncls = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
    memset(decl_ncls, 0, sizeof(SonDecl));
    decl_ncls->kind = DECL_FUNCTION;
    decl_ncls->name = son_slice_from_cstr("son_net_close");
    decl_ncls->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
    decl_ncls->func.return_type->kind = TYPE_VOID;
    SonSymbol *sym_ncls = alloc_symbol(arena);
    sym_ncls->kind = SYM_FUNCTION;
    sym_ncls->name = decl_ncls->name;
    sym_ncls->type = decl_ncls->func.return_type;
    sym_ncls->decl = decl_ncls;
    define_symbol(sema->global_scope, sym_ncls);

    // Nonblocking socket symbols
    const char *names_bool[] = {"son_net_set_nonblocking", "son_net_set_nodelay", "son_thread_join", "son_file_seek"};
    for (size_t i = 0; i < sizeof(names_bool)/sizeof(names_bool[0]); i++) {
        SonDecl *d = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
        memset(d, 0, sizeof(SonDecl));
        d->kind = DECL_FUNCTION;
        d->name = son_slice_from_cstr(names_bool[i]);
        d->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
        d->func.return_type->kind = TYPE_BOOL;
        SonSymbol *s = alloc_symbol(arena);
        s->kind = SYM_FUNCTION; s->name = d->name; s->type = d->func.return_type; s->decl = d;
        define_symbol(sema->global_scope, s);
    }

    const char *names_i32[] = {"son_net_poll"};
    for (size_t i = 0; i < sizeof(names_i32)/sizeof(names_i32[0]); i++) {
        SonDecl *d = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
        memset(d, 0, sizeof(SonDecl));
        d->kind = DECL_FUNCTION;
        d->name = son_slice_from_cstr(names_i32[i]);
        d->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
        d->func.return_type->kind = TYPE_I32;
        SonSymbol *s = alloc_symbol(arena);
        s->kind = SYM_FUNCTION; s->name = d->name; s->type = d->func.return_type; s->decl = d;
        define_symbol(sema->global_scope, s);
    }

    const char *names_i64[] = {"son_thread_spawn", "son_mutex_init", "son_file_open", "son_file_tell", "son_file_read_exact", "son_file_write_exact", "son_time_nanos"};
    for (size_t i = 0; i < sizeof(names_i64)/sizeof(names_i64[0]); i++) {
        SonDecl *d = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
        memset(d, 0, sizeof(SonDecl));
        d->kind = DECL_FUNCTION;
        d->name = son_slice_from_cstr(names_i64[i]);
        d->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
        d->func.return_type->kind = TYPE_I64;
        SonSymbol *s = alloc_symbol(arena);
        s->kind = SYM_FUNCTION; s->name = d->name; s->type = d->func.return_type; s->decl = d;
        define_symbol(sema->global_scope, s);
    }

    const char *names_void[] = {"son_mutex_lock", "son_mutex_unlock", "son_mutex_destroy", "son_file_close_handle", "son_sleep_us"};
    for (size_t i = 0; i < sizeof(names_void)/sizeof(names_void[0]); i++) {
        SonDecl *d = (SonDecl*)son_arena_alloc(arena, sizeof(SonDecl), sizeof(void*));
        memset(d, 0, sizeof(SonDecl));
        d->kind = DECL_FUNCTION;
        d->name = son_slice_from_cstr(names_void[i]);
        d->func.return_type = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
        d->func.return_type->kind = TYPE_VOID;
        SonSymbol *s = alloc_symbol(arena);
        s->kind = SYM_FUNCTION; s->name = d->name; s->type = d->func.return_type; s->decl = d;
        define_symbol(sema->global_scope, s);
    }
}

static SonScope* push_scope(SonSema *sema, bool is_fn, SonType *ret_type) {
    SonScope *scope = (SonScope*)son_arena_alloc(sema->arena, sizeof(SonScope), sizeof(void*));
    memset(scope, 0, sizeof(SonScope));
    scope->parent = sema->current_scope;
    scope->arena = sema->arena;
    scope->is_function_scope = is_fn;
    scope->expected_return_type = ret_type ? ret_type : (sema->current_scope ? sema->current_scope->expected_return_type : NULL);
    sema->current_scope = scope;
    return scope;
}

static SonSymbol* alloc_symbol(SonArena *arena) {
    SonSymbol *sym = (SonSymbol*)son_arena_alloc(arena, sizeof(SonSymbol), sizeof(void*));
    memset(sym, 0, sizeof(SonSymbol));
    return sym;
}

static void pop_scope(SonSema *sema) {
    if (sema->current_scope) {
        for (SonSymbol *s = sema->current_scope->symbols; s; s = s->next) {
            if (s->borrowed_from) {
                if (s->type && s->type->kind == TYPE_REFERENCE && s->type->is_mut) {
                    s->borrowed_from->is_mutably_borrowed = false;
                } else {
                    if (s->borrowed_from->borrow_count > 0) {
                        s->borrowed_from->borrow_count--;
                    }
                }
                s->borrowed_from = NULL;
            }
        }
        if (sema->current_scope->parent) {
            sema->current_scope = sema->current_scope->parent;
        }
    }
}

static bool define_symbol(SonScope *scope, SonSymbol *sym) {
    // Check redeclaration in current scope
    SonSymbol *curr = scope->symbols;
    while (curr) {
        if (son_slice_eq(curr->name, sym->name)) {
            return false;
        }
        curr = curr->next;
    }
    sym->next = scope->symbols;
    scope->symbols = sym;
    return true;
}

static SonSymbol* lookup_symbol(SonScope *scope, SonStringSlice name) {
    SonScope *curr_scope = scope;
    while (curr_scope) {
        SonSymbol *sym = curr_scope->symbols;
        while (sym) {
            if (son_slice_eq(sym->name, name)) {
                return sym;
            }
            sym = sym->next;
        }
        curr_scope = curr_scope->parent;
    }
    return NULL;
}

static bool is_integer_type(SonTypeKind k) {
    return k == TYPE_I8 || k == TYPE_I16 || k == TYPE_I32 || k == TYPE_I64 || k == TYPE_ISIZE ||
           k == TYPE_U8 || k == TYPE_U16 || k == TYPE_U32 || k == TYPE_U64 || k == TYPE_USIZE;
}

bool son_type_equals(SonType *a, SonType *b) {
    if (a == b) return true;
    if (!a || !b) return false;

    // Both are user-defined types (struct or union)
    if ((a->kind == TYPE_STRUCT || a->kind == TYPE_UNION) &&
        (b->kind == TYPE_STRUCT || b->kind == TYPE_UNION)) {
        return son_slice_eq(a->name, b->name);
    }

    // Allow integer literals to match expected integer types
    if (is_integer_type(a->kind) && is_integer_type(b->kind)) {
        return true;
    }

    if (a->kind != b->kind) {
        // Allow passing Array [T; N] to Slice [T] parameter or variable
        if ((a->kind == TYPE_SLICE && b->kind == TYPE_ARRAY) ||
            (a->kind == TYPE_ARRAY && b->kind == TYPE_SLICE)) {
            return son_type_equals(a->sub_type, b->sub_type);
        }
        return false;
    }

    switch (a->kind) {
        case TYPE_REFERENCE:
        case TYPE_POINTER:
        case TYPE_SLICE:
        case TYPE_ARRAY:
            return son_type_equals(a->sub_type, b->sub_type);
        default:
            return true;
    }
}

const char* son_type_to_string(SonType *type) {
    if (!type) return "unknown";
    switch (type->kind) {
        case TYPE_VOID: return "void";
        case TYPE_BOOL: return "bool";
        case TYPE_CHAR: return "char";
        case TYPE_I8: return "i8";
        case TYPE_I16: return "i16";
        case TYPE_I32: return "i32";
        case TYPE_I64: return "i64";
        case TYPE_ISIZE: return "isize";
        case TYPE_U8: return "u8";
        case TYPE_U16: return "u16";
        case TYPE_U32: return "u32";
        case TYPE_U64: return "u64";
        case TYPE_USIZE: return "usize";
        case TYPE_F32: return "f32";
        case TYPE_F64: return "f64";
        case TYPE_REFERENCE: return "ref";
        case TYPE_SLICE: return "slice";
        case TYPE_STRUCT:
        case TYPE_UNION: {
            static char buf[128];
            snprintf(buf, sizeof(buf), "%.*s", (int)type->name.length, type->name.data);
            return buf;
        }
        default: return "type";
    }
}

static SonType* substitute_type_params(SonArena *arena, SonType *t, SonStringSlice *params, SonType **args, size_t count) {
    if (!t) return NULL;
    if (t->kind == TYPE_GENERIC_PARAM) {
        for (size_t p = 0; p < count; p++) {
            if (son_slice_eq(params[p], t->name)) {
                return args[p];
            }
        }
        return t;
    }
    if (t->kind == TYPE_SLICE || t->kind == TYPE_REFERENCE || t->kind == TYPE_POINTER) {
        SonType *sub = substitute_type_params(arena, t->sub_type, params, args, count);
        SonType *nt = (SonType*)son_arena_alloc(arena, sizeof(SonType), sizeof(void*));
        *nt = *t;
        nt->sub_type = sub;
        return nt;
    }
    return t;
}

static SonStringSlice son_mangle_generic_name(SonArena *arena, SonStringSlice base, SonType **args, size_t count) {
    char buf[256];
    int off = snprintf(buf, sizeof(buf), "%.*s", (int)base.length, base.data);
    for (size_t i = 0; i < count; i++) {
        off += snprintf(buf + off, sizeof(buf) - off, "_%s", son_type_to_string(args[i]));
    }
    size_t len = strlen(buf);
    char *copy = (char*)son_arena_alloc(arena, len + 1, 1);
    memcpy(copy, buf, len + 1);
    SonStringSlice s = { .data = copy, .length = len };
    return s;
}

static SonType* monomorphize_type(SonSema *sema, SonType *type) {
    if (!type) return NULL;
    if (type->kind == TYPE_REFERENCE || type->kind == TYPE_POINTER || type->kind == TYPE_SLICE) {
        if (type->sub_type) {
            type->sub_type = monomorphize_type(sema, type->sub_type);
        }
        return type;
    }
    if (type->type_arg_count == 0) return type;
    if (!sema->current_module) return type;

    SonSymbol *template_sym = lookup_symbol(sema->global_scope, type->name);
    if (!template_sym || !template_sym->decl || template_sym->decl->type_param_count != type->type_arg_count) {
        return type;
    }

    SonDecl *tmpl_decl = template_sym->decl;
    SonStringSlice spec_name = son_mangle_generic_name(sema->arena, tmpl_decl->name, type->type_args, type->type_arg_count);

    SonSymbol *existing = lookup_symbol(sema->global_scope, spec_name);
    if (existing) {
        type->name = spec_name;
        type->kind = existing->type->kind;
        type->decl = existing->decl;
        return existing->type;
    }

    SonDecl *spec_decl = (SonDecl*)son_arena_alloc(sema->arena, sizeof(SonDecl), sizeof(void*));
    memset(spec_decl, 0, sizeof(SonDecl));
    spec_decl->kind = tmpl_decl->kind;
    spec_decl->loc = tmpl_decl->loc;
    spec_decl->name = spec_name;
    spec_decl->type_param_count = 0;

    if (tmpl_decl->kind == DECL_UNION) {
        spec_decl->union_decl.variant_count = tmpl_decl->union_decl.variant_count;
        spec_decl->union_decl.variants = (SonVariantDecl*)son_arena_alloc(sema->arena, sizeof(SonVariantDecl) * spec_decl->union_decl.variant_count, sizeof(void*));

        for (size_t v = 0; v < tmpl_decl->union_decl.variant_count; v++) {
            SonVariantDecl *tmpl_var = &tmpl_decl->union_decl.variants[v];
            SonVariantDecl *spec_var = &spec_decl->union_decl.variants[v];
            spec_var->name = tmpl_var->name;
            spec_var->tag_value = tmpl_var->tag_value;
            spec_var->payload_type = substitute_type_params(sema->arena, tmpl_var->payload_type, tmpl_decl->type_params, type->type_args, tmpl_decl->type_param_count);
        }
    } else if (tmpl_decl->kind == DECL_STRUCT) {
        spec_decl->struct_decl.field_count = tmpl_decl->struct_decl.field_count;
        spec_decl->struct_decl.fields = (SonFieldDecl*)son_arena_alloc(sema->arena, sizeof(SonFieldDecl) * spec_decl->struct_decl.field_count, sizeof(void*));

        for (size_t f = 0; f < tmpl_decl->struct_decl.field_count; f++) {
            SonFieldDecl *tmpl_field = &tmpl_decl->struct_decl.fields[f];
            SonFieldDecl *spec_field = &spec_decl->struct_decl.fields[f];
            spec_field->name = tmpl_field->name;
            spec_field->type = substitute_type_params(sema->arena, tmpl_field->type, tmpl_decl->type_params, type->type_args, tmpl_decl->type_param_count);
        }
    }

    SonSymbol *spec_sym = alloc_symbol(sema->arena);
    spec_sym->name = spec_name;
    spec_sym->loc = tmpl_decl->loc;
    spec_sym->decl = spec_decl;
    spec_sym->kind = (spec_decl->kind == DECL_UNION) ? SYM_UNION : SYM_STRUCT;

    SonType *spec_type = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
    spec_type->kind = (spec_decl->kind == DECL_UNION) ? TYPE_UNION : TYPE_STRUCT;
    spec_type->name = spec_name;
    spec_type->decl = spec_decl;
    spec_sym->type = spec_type;

    define_symbol(sema->global_scope, spec_sym);
    if (sema->current_module->decl_count < 256) {
        sema->current_module->decls[sema->current_module->decl_count++] = spec_decl;
    }

    type->name = spec_name;
    type->kind = spec_type->kind;
    type->decl = spec_decl;
    return spec_type;
}

static SonType* analyze_expr(SonSema *sema, SonExpr *expr);

static void analyze_stmt(SonSema *sema, SonStmt *stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
        case STMT_VAR_DECL: {
            SonType *annot_type = stmt->var_decl.type_annot;
            if (annot_type && annot_type->type_arg_count > 0) {
                annot_type = monomorphize_type(sema, annot_type);
                stmt->var_decl.type_annot = annot_type;
            }
            sema->current_expected_type = annot_type;
            SonType *init_type = NULL;
            if (stmt->var_decl.init_expr) {
                init_type = analyze_expr(sema, stmt->var_decl.init_expr);
            }
            sema->current_expected_type = NULL;

            SonType *final_type = annot_type ? annot_type : init_type;
            if (!final_type) {
                son_error_report(stmt->loc, "Cannot infer type for variable '%.*s'",
                                 (int)stmt->var_decl.name.length, stmt->var_decl.name.data);
                sema->has_error = true;
                sema->error_count++;
                return;
            }
            stmt->var_decl.type_annot = final_type;

            if (annot_type && init_type) {
                if (!son_type_equals(annot_type, init_type)) {
                    son_error_report(stmt->loc, "Type mismatch in variable '%.*s': declared as '%s' but assigned '%s'",
                                     (int)stmt->var_decl.name.length, stmt->var_decl.name.data,
                                     son_type_to_string(annot_type), son_type_to_string(init_type));
                    sema->has_error = true;
                    sema->error_count++;
                }
            }

            SonSymbol *sym = alloc_symbol(sema->arena);
            sym->kind = SYM_VARIABLE;
            sym->name = stmt->var_decl.name;
            sym->type = final_type;
            sym->loc = stmt->loc;
            sym->is_mut = stmt->var_decl.is_var;
            sym->borrow_count = 0;
            sym->is_mutably_borrowed = false;
            sym->borrowed_from = NULL;
            sym->next = NULL;

            // Strict Borrow Checking for reference variables
            if (stmt->var_decl.init_expr && stmt->var_decl.init_expr->kind == EXPR_UNARY &&
                (stmt->var_decl.init_expr->unary.op == TOKEN_KW_REF || stmt->var_decl.init_expr->unary.op == TOKEN_KW_MUT)) {
                SonExpr *target_expr = stmt->var_decl.init_expr->unary.operand;
                if (target_expr->kind == EXPR_IDENTIFIER) {
                    SonSymbol *target_sym = lookup_symbol(sema->current_scope, target_expr->ident_name);
                    if (target_sym) {
                        bool is_mut_borrow = (stmt->var_decl.init_expr->unary.op == TOKEN_KW_MUT);
                        if (is_mut_borrow) {
                            if (!target_sym->is_mut) {
                                son_error_report(stmt->loc, "Cannot borrow immutable variable '%.*s' as mutable (declared with 'val')",
                                                 (int)target_sym->name.length, target_sym->name.data);
                                sema->has_error = true;
                                sema->error_count++;
                            } else if (target_sym->is_mutably_borrowed) {
                                son_error_report(stmt->loc, "Cannot borrow '%.*s' as mutable more than once at a time",
                                                 (int)target_sym->name.length, target_sym->name.data);
                                sema->has_error = true;
                                sema->error_count++;
                            } else if (target_sym->borrow_count > 0) {
                                son_error_report(stmt->loc, "Cannot borrow '%.*s' as mutable because it is already borrowed as immutable",
                                                 (int)target_sym->name.length, target_sym->name.data);
                                sema->has_error = true;
                                sema->error_count++;
                            } else {
                                target_sym->is_mutably_borrowed = true;
                                sym->borrowed_from = target_sym;
                            }
                        } else {
                            if (target_sym->is_mutably_borrowed) {
                                son_error_report(stmt->loc, "Cannot borrow '%.*s' as immutable because it is currently borrowed as mutable",
                                                 (int)target_sym->name.length, target_sym->name.data);
                                sema->has_error = true;
                                sema->error_count++;
                            } else {
                                target_sym->borrow_count++;
                                sym->borrowed_from = target_sym;
                            }
                        }
                    }
                }
            }

            if (!define_symbol(sema->current_scope, sym)) {
                son_error_report(stmt->loc, "Redeclaration of variable '%.*s' in the same scope",
                                 (int)stmt->var_decl.name.length, stmt->var_decl.name.data);
                sema->has_error = true;
                sema->error_count++;
            }
            break;
        }

        case STMT_ASSIGN: {
            SonType *lhs_type = analyze_expr(sema, stmt->assign.lvalue);
            SonType *rhs_type = analyze_expr(sema, stmt->assign.rvalue);

            if (stmt->assign.lvalue->kind == EXPR_IDENTIFIER) {
                SonSymbol *sym = lookup_symbol(sema->current_scope, stmt->assign.lvalue->ident_name);
                if (sym) {
                    if (!sym->is_mut) {
                        son_error_report(stmt->loc, "Cannot assign to immutable value '%.*s' (declared with 'val')",
                                         (int)sym->name.length, sym->name.data);
                        sema->has_error = true;
                        sema->error_count++;
                    } else if (sym->is_mutably_borrowed || sym->borrow_count > 0) {
                        son_error_report(stmt->loc, "Cannot assign to '%.*s' because it is currently borrowed",
                                         (int)sym->name.length, sym->name.data);
                        sema->has_error = true;
                        sema->error_count++;
                    }
                }
            }

            if (lhs_type && rhs_type && !son_type_equals(lhs_type, rhs_type)) {
                son_error_report(stmt->loc, "Cannot assign expression of type '%s' to target of type '%s'",
                                 son_type_to_string(rhs_type), son_type_to_string(lhs_type));
                sema->has_error = true;
                sema->error_count++;
            }
            break;
        }

        case STMT_EXPR:
            analyze_expr(sema, stmt->expr);
            break;

        case STMT_RETURN: {
            SonType *ret_type = NULL;
            SonType *expected = sema->current_scope->expected_return_type;
            sema->current_expected_type = expected;
            if (stmt->ret.expr) {
                ret_type = analyze_expr(sema, stmt->ret.expr);

                // Prevent returning local reference pointing to current scope (dangling pointer prevention)
                if (ret_type && (ret_type->kind == TYPE_REFERENCE || ret_type->kind == TYPE_POINTER)) {
                    if (stmt->ret.expr->kind == EXPR_UNARY &&
                        (stmt->ret.expr->unary.op == TOKEN_KW_REF || stmt->ret.expr->unary.op == TOKEN_KW_MUT)) {
                        SonExpr *target = stmt->ret.expr->unary.operand;
                        if (target->kind == EXPR_IDENTIFIER) {
                            SonSymbol *target_sym = lookup_symbol(sema->current_scope, target->ident_name);
                            if (target_sym && sema->current_scope->parent != NULL) {
                                son_error_report(stmt->loc, "Cannot return reference to local variable '%.*s' (escapes local stack frame)",
                                                 (int)target->ident_name.length, target->ident_name.data);
                                sema->has_error = true;
                                sema->error_count++;
                            }
                        }
                    }
                }
            }
            sema->current_expected_type = NULL;
            if (expected) {
                if (expected->kind == TYPE_VOID && ret_type != NULL) {
                    son_error_report(stmt->loc, "Function returning void cannot return a value");
                    sema->has_error = true;
                    sema->error_count++;
                } else if (expected->kind != TYPE_VOID && !son_type_equals(expected, ret_type)) {
                    son_error_report(stmt->loc, "Return type mismatch: expected '%s', got '%s'",
                                     son_type_to_string(expected), son_type_to_string(ret_type));
                    sema->has_error = true;
                    sema->error_count++;
                }
            }
            break;
        }

        case STMT_IF: {
            SonType *cond_type = analyze_expr(sema, stmt->if_stmt.cond);
            if (cond_type && cond_type->kind != TYPE_BOOL) {
                son_error_report(stmt->loc, "Condition of 'if' statement must be 'bool', got '%s'",
                                 son_type_to_string(cond_type));
                sema->has_error = true;
                sema->error_count++;
            }
            push_scope(sema, false, NULL);
            for (size_t i = 0; i < stmt->if_stmt.then_block.stmt_count; i++) {
                analyze_stmt(sema, stmt->if_stmt.then_block.stmts[i]);
            }
            pop_scope(sema);

            if (stmt->if_stmt.has_else) {
                push_scope(sema, false, NULL);
                for (size_t i = 0; i < stmt->if_stmt.else_block.stmt_count; i++) {
                    analyze_stmt(sema, stmt->if_stmt.else_block.stmts[i]);
                }
                pop_scope(sema);
            }
            break;
        }

        case STMT_WHILE: {
            SonType *cond_type = analyze_expr(sema, stmt->while_stmt.cond);
            if (cond_type && cond_type->kind != TYPE_BOOL) {
                son_error_report(stmt->loc, "Condition of 'while' statement must be 'bool', got '%s'",
                                 son_type_to_string(cond_type));
                sema->has_error = true;
                sema->error_count++;
            }
            push_scope(sema, false, NULL);
            for (size_t i = 0; i < stmt->while_stmt.body.stmt_count; i++) {
                analyze_stmt(sema, stmt->while_stmt.body.stmts[i]);
            }
            pop_scope(sema);
            break;
        }

        case STMT_MATCH: {
            SonType *target_type = analyze_expr(sema, stmt->match_stmt.expr);
            SonDecl *union_decl = NULL;
            if (target_type) {
                SonSymbol *sym = lookup_symbol(sema->global_scope, target_type->name);
                if (sym && sym->kind == SYM_UNION) {
                    union_decl = sym->decl;
                }
            }

            for (size_t a = 0; a < stmt->match_stmt.arm_count; a++) {
                SonMatchArm *arm = &stmt->match_stmt.arms[a];
                push_scope(sema, false, NULL);

                if (!arm->is_default && union_decl) {
                    // Check that variant exists in union
                    SonVariantDecl *matched_var = NULL;
                    for (size_t v = 0; v < union_decl->union_decl.variant_count; v++) {
                        if (son_slice_eq(union_decl->union_decl.variants[v].name, arm->variant_name)) {
                            matched_var = &union_decl->union_decl.variants[v];
                            break;
                        }
                    }

                    if (!matched_var) {
                        son_error_report(stmt->loc, "Variant '%.*s' does not exist in union '%.*s'",
                                         (int)arm->variant_name.length, arm->variant_name.data,
                                         (int)union_decl->name.length, union_decl->name.data);
                        sema->has_error = true;
                        sema->error_count++;
                    } else if (arm->has_payload && matched_var->payload_type) {
                        // Bind payload variable
                        SonSymbol *b_sym = alloc_symbol(sema->arena);
                        b_sym->kind = SYM_VARIABLE;
                        b_sym->name = arm->payload_var;
                        b_sym->type = matched_var->payload_type;
                        b_sym->loc = stmt->loc;
                        b_sym->is_mut = false;
                        define_symbol(sema->current_scope, b_sym);
                    }
                }

                for (size_t s = 0; s < arm->body.stmt_count; s++) {
                    analyze_stmt(sema, arm->body.stmts[s]);
                }
                pop_scope(sema);
            }
            break;
        }

        case STMT_BLOCK: {
            push_scope(sema, false, NULL);
            for (size_t i = 0; i < stmt->block.stmt_count; i++) {
                analyze_stmt(sema, stmt->block.stmts[i]);
            }
            pop_scope(sema);
            break;
        }

        default:
            break;
    }
}

static SonType* analyze_expr(SonSema *sema, SonExpr *expr) {
    if (!expr) return NULL;

    switch (expr->kind) {
        case EXPR_INT_LITERAL: {
            SonType *t = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
            t->kind = TYPE_I32;
            expr->inferred_type = t;
            return t;
        }
        case EXPR_FLOAT_LITERAL: {
            SonType *t = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
            t->kind = TYPE_F64;
            expr->inferred_type = t;
            return t;
        }
        case EXPR_BOOL_LITERAL: {
            SonType *t = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
            t->kind = TYPE_BOOL;
            expr->inferred_type = t;
            return t;
        }
        case EXPR_CHAR_LITERAL: {
            SonType *t = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
            t->kind = TYPE_CHAR;
            expr->inferred_type = t;
            return t;
        }
        case EXPR_STRING_LITERAL: {
            SonType *slice_t = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
            slice_t->kind = TYPE_SLICE;
            SonType *u8_t = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
            u8_t->kind = TYPE_U8;
            slice_t->sub_type = u8_t;
            expr->inferred_type = slice_t;
            return slice_t;
        }
        case EXPR_IDENTIFIER: {
            SonSymbol *sym = lookup_symbol(sema->current_scope, expr->ident_name);
            if (!sym) {
                son_error_report(expr->loc, "Undefined identifier '%.*s'",
                                 (int)expr->ident_name.length, expr->ident_name.data);
                sema->has_error = true;
                sema->error_count++;
                return NULL;
            }
            expr->inferred_type = sym->type;
            return sym->type;
        }
        case EXPR_BINARY: {
            SonType *lt = analyze_expr(sema, expr->binary.left);
            SonType *rt = analyze_expr(sema, expr->binary.right);

            switch (expr->binary.op) {
                case TOKEN_PLUS:
                case TOKEN_MINUS:
                case TOKEN_STAR:
                case TOKEN_SLASH:
                case TOKEN_PERCENT:
                case TOKEN_CARET:
                case TOKEN_AMP:
                case TOKEN_PIPE:
                    if (lt && rt && !son_type_equals(lt, rt)) {
                        son_error_report(expr->loc, "Binary operator requires matching operand types, got '%s' and '%s'",
                                         son_type_to_string(lt), son_type_to_string(rt));
                        sema->has_error = true;
                        sema->error_count++;
                    }
                    expr->inferred_type = lt;
                    return lt;

                case TOKEN_SHL:
                case TOKEN_SHR:
                    expr->inferred_type = lt;
                    return lt;

                case TOKEN_EQ_EQ:
                case TOKEN_BANG_EQ:
                case TOKEN_LESS:
                case TOKEN_LESS_EQ:
                case TOKEN_GREATER:
                case TOKEN_GREATER_EQ: {
                    if (lt && rt && !son_type_equals(lt, rt)) {
                        son_error_report(expr->loc, "Comparison operator requires matching operand types, got '%s' and '%s'",
                                         son_type_to_string(lt), son_type_to_string(rt));
                        sema->has_error = true;
                        sema->error_count++;
                    }
                    SonType *b = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
                    b->kind = TYPE_BOOL;
                    expr->inferred_type = b;
                    return b;
                }

                case TOKEN_AMP_AMP:
                case TOKEN_PIPE_PIPE: {
                    if ((lt && lt->kind != TYPE_BOOL) || (rt && rt->kind != TYPE_BOOL)) {
                        son_error_report(expr->loc, "Logical operators require 'bool' operands");
                        sema->has_error = true;
                        sema->error_count++;
                    }
                    SonType *b = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
                    b->kind = TYPE_BOOL;
                    expr->inferred_type = b;
                    return b;
                }

                default:
                    expr->inferred_type = lt;
                    return lt;
            }
        }
        case EXPR_UNARY: {
            SonType *ot = analyze_expr(sema, expr->unary.operand);
            if (expr->unary.op == TOKEN_KW_MUT || expr->unary.op == TOKEN_KW_REF) {
                SonType *ref_t = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
                ref_t->kind = TYPE_REFERENCE;
                ref_t->is_mut = (expr->unary.op == TOKEN_KW_MUT);
                ref_t->sub_type = ot;
                expr->inferred_type = ref_t;
                return ref_t;
            }
            expr->inferred_type = ot;
            return ot;
        }
        case EXPR_CALL: {
            if (expr->call.callee->kind == EXPR_MEMBER) {
                SonExpr *target = expr->call.callee->member.target;
                if (target->kind == EXPR_IDENTIFIER) {
                    SonSymbol *sym = lookup_symbol(sema->global_scope, target->ident_name);
                    if (sym && sym->kind == SYM_UNION && sym->decl && sym->decl->type_param_count == 1 && expr->call.arg_count == 1) {
                        SonType *arg_t = analyze_expr(sema, expr->call.args[0]);
                        SonType *inst = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
                        inst->kind = TYPE_UNION;
                        inst->name = sym->name;
                        inst->type_args = (SonType**)son_arena_alloc(sema->arena, sizeof(SonType*), sizeof(void*));
                        inst->type_args[0] = arg_t;
                        inst->type_arg_count = 1;
                        SonType *monomorphized = monomorphize_type(sema, inst);
                        target->inferred_type = monomorphized;
                        expr->inferred_type = monomorphized;
                        return monomorphized;
                    }
                }
            }
            SonType *callee_t = analyze_expr(sema, expr->call.callee);
            for (size_t i = 0; i < expr->call.arg_count; i++) {
                analyze_expr(sema, expr->call.args[i]);
            }
            // Disallow multiple conflicting borrows to the same symbol in argument list
            for (size_t i = 0; i < expr->call.arg_count; i++) {
                SonExpr *arg1 = expr->call.args[i];
                if (arg1->kind == EXPR_UNARY && (arg1->unary.op == TOKEN_KW_MUT || arg1->unary.op == TOKEN_KW_REF)) {
                    if (arg1->unary.operand->kind == EXPR_IDENTIFIER) {
                        SonStringSlice name1 = arg1->unary.operand->ident_name;
                        for (size_t j = i + 1; j < expr->call.arg_count; j++) {
                            SonExpr *arg2 = expr->call.args[j];
                            if (arg2->kind == EXPR_UNARY && (arg2->unary.op == TOKEN_KW_MUT || arg2->unary.op == TOKEN_KW_REF)) {
                                if (arg2->unary.operand->kind == EXPR_IDENTIFIER) {
                                    SonStringSlice name2 = arg2->unary.operand->ident_name;
                                    if (son_slice_eq(name1, name2) && (arg1->unary.op == TOKEN_KW_MUT || arg2->unary.op == TOKEN_KW_MUT)) {
                                        son_error_report(arg2->loc, "Cannot pass conflicting borrows of '%.*s' in function arguments",
                                                         (int)name1.length, name1.data);
                                        sema->has_error = true;
                                        sema->error_count++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (expr->call.callee->kind == EXPR_IDENTIFIER) {
                SonSymbol *sym = lookup_symbol(sema->global_scope, expr->call.callee->ident_name);
                if (sym && sym->kind == SYM_FUNCTION && sym->decl) {
                    expr->inferred_type = sym->decl->func.return_type;
                    return expr->inferred_type;
                }
            }
            expr->inferred_type = callee_t;
            return callee_t;
        }
        case EXPR_MEMBER: {
            if (expr->member.target->kind == EXPR_IDENTIFIER) {
                SonSymbol *sym = lookup_symbol(sema->global_scope, expr->member.target->ident_name);
                if (sym && sym->kind == SYM_UNION && sym->decl && sym->decl->type_param_count > 0) {
                    if (sema->current_expected_type) {
                        expr->member.target->inferred_type = sema->current_expected_type;
                        expr->inferred_type = sema->current_expected_type;
                        return sema->current_expected_type;
                    }
                }
            }
            SonType *tt = analyze_expr(sema, expr->member.target);
            if (tt) {
                SonType *struct_type = tt;
                if (tt->kind == TYPE_REFERENCE || tt->kind == TYPE_POINTER) {
                    struct_type = tt->sub_type;
                }
                if (struct_type && struct_type->kind == TYPE_UNION) {
                    // Variant access on union type: Union.Variant
                    expr->inferred_type = struct_type;
                    return struct_type;
                }
                if (struct_type && struct_type->kind == TYPE_SLICE && son_slice_eq_cstr(expr->member.member, "len")) {
                    SonType *usize_t = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
                    usize_t->kind = TYPE_USIZE;
                    expr->inferred_type = usize_t;
                    return usize_t;
                }
                if (struct_type && struct_type->kind == TYPE_STRUCT) {
                    SonSymbol *sym = lookup_symbol(sema->global_scope, struct_type->name);
                    if (sym && sym->kind == SYM_STRUCT && sym->decl) {
                        for (size_t f = 0; f < sym->decl->struct_decl.field_count; f++) {
                            if (son_slice_eq(sym->decl->struct_decl.fields[f].name, expr->member.member)) {
                                expr->inferred_type = sym->decl->struct_decl.fields[f].type;
                                return expr->inferred_type;
                            }
                        }
                        son_error_report(expr->loc, "Struct '%.*s' has no field '%.*s'",
                                         (int)struct_type->name.length, struct_type->name.data,
                                         (int)expr->member.member.length, expr->member.member.data);
                        sema->has_error = true;
                        sema->error_count++;
                    }
                }
            }
            return NULL;
        }
        case EXPR_INDEX: {
            SonType *tt = analyze_expr(sema, expr->index.target);
            SonType *it = analyze_expr(sema, expr->index.index);
            if (it && it->kind != TYPE_I32 && it->kind != TYPE_I64 && it->kind != TYPE_USIZE) {
                son_error_report(expr->loc, "Index must be an integer, got '%s'", son_type_to_string(it));
                sema->has_error = true;
                sema->error_count++;
            }
            if (tt && (tt->kind == TYPE_SLICE || tt->kind == TYPE_ARRAY || tt->kind == TYPE_POINTER)) {
                expr->inferred_type = tt->sub_type;
                return tt->sub_type;
            }
            return NULL;
        }
        case EXPR_CAST: {
            analyze_expr(sema, expr->cast.expr);
            expr->inferred_type = expr->cast.target_type;
            return expr->cast.target_type;
        }
        case EXPR_STRUCT_INIT: {
            SonType *st = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
            st->kind = TYPE_STRUCT;
            st->name = expr->struct_init.struct_name;

            SonSymbol *sym = lookup_symbol(sema->global_scope, st->name);
            if (sym && sym->kind == SYM_STRUCT && sym->decl && sym->decl->type_param_count > 0) {
                if (sema->current_expected_type) {
                    st = sema->current_expected_type;
                    expr->struct_init.struct_name = sema->current_expected_type->name;
                    sym = lookup_symbol(sema->global_scope, st->name);
                }
            }

            if (!sym || sym->kind != SYM_STRUCT) {
                son_error_report(expr->loc, "Unknown struct type '%.*s'",
                                 (int)st->name.length, st->name.data);
                sema->has_error = true;
                sema->error_count++;
            }

            for (size_t i = 0; i < expr->struct_init.field_count; i++) {
                analyze_expr(sema, expr->struct_init.fields[i].value);
            }
            expr->inferred_type = st;
            return st;
        }
        case EXPR_MATCH: {
            SonType *target_type = analyze_expr(sema, expr->match_expr.target);
            SonDecl *union_decl = NULL;
            if (target_type) {
                SonSymbol *sym = lookup_symbol(sema->global_scope, target_type->name);
                if (sym && sym->kind == SYM_UNION) {
                    union_decl = sym->decl;
                }
            }

            SonType *common_type = NULL;
            for (size_t a = 0; a < expr->match_expr.arm_count; a++) {
                SonMatchArm *arm = &expr->match_expr.arms[a];
                push_scope(sema, false, NULL);

                if (!arm->is_default && union_decl) {
                    SonVariantDecl *matched_var = NULL;
                    for (size_t v = 0; v < union_decl->union_decl.variant_count; v++) {
                        if (son_slice_eq(union_decl->union_decl.variants[v].name, arm->variant_name)) {
                            matched_var = &union_decl->union_decl.variants[v];
                            break;
                        }
                    }

                    if (!matched_var) {
                        son_error_report(expr->loc, "Variant '%.*s' does not exist in union '%.*s'",
                                         (int)arm->variant_name.length, arm->variant_name.data,
                                         (int)union_decl->name.length, union_decl->name.data);
                        sema->has_error = true;
                        sema->error_count++;
                    } else if (arm->has_payload && matched_var->payload_type) {
                        SonSymbol *b_sym = alloc_symbol(sema->arena);
                        b_sym->kind = SYM_VARIABLE;
                        b_sym->name = arm->payload_var;
                        b_sym->type = matched_var->payload_type;
                        b_sym->loc = expr->loc;
                        b_sym->is_mut = false;
                        define_symbol(sema->current_scope, b_sym);
                    }
                }

                if (arm->is_expr) {
                    SonType *arm_t = analyze_expr(sema, arm->expr);
                    if (!common_type) {
                        common_type = arm_t;
                    } else if (arm_t && !son_type_equals(common_type, arm_t)) {
                        son_error_report(expr->loc, "Match arm type mismatch: expected '%s', got '%s'",
                                         son_type_to_string(common_type), son_type_to_string(arm_t));
                        sema->has_error = true;
                        sema->error_count++;
                    }
                } else {
                    for (size_t s = 0; s < arm->body.stmt_count; s++) {
                        analyze_stmt(sema, arm->body.stmts[s]);
                    }
                }

                pop_scope(sema);
            }

            expr->inferred_type = common_type;
            return common_type;
        }
        case EXPR_ARRAY_LITERAL: {
            SonType *elem_t = NULL;
            if (sema->current_expected_type) {
                if (sema->current_expected_type->kind == TYPE_ARRAY ||
                    sema->current_expected_type->kind == TYPE_SLICE) {
                    elem_t = sema->current_expected_type->sub_type;
                }
            }

            for (size_t i = 0; i < expr->array_literal.element_count; i++) {
                SonType *t = analyze_expr(sema, expr->array_literal.elements[i]);
                if (!elem_t) {
                    elem_t = t;
                }
            }

            SonType *arr_t = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
            arr_t->kind = TYPE_ARRAY;
            arr_t->sub_type = elem_t;
            arr_t->array_len = expr->array_literal.element_count;
            expr->inferred_type = arr_t;
            return arr_t;
        }
        default:
            return NULL;
    }
}

bool son_sema_analyze_module(SonSema *sema, SonModule *module) {
    sema->current_module = module;
    sema->current_expected_type = NULL;

    // Pass 1: Register top-level types and functions
    for (size_t i = 0; i < module->decl_count; i++) {
        SonDecl *decl = module->decls[i];
        SonSymbol *sym = alloc_symbol(sema->arena);
        sym->name = decl->name;
        sym->loc = decl->loc;
        sym->decl = decl;
        sym->next = NULL;

        switch (decl->kind) {
            case DECL_STRUCT: {
                sym->kind = SYM_STRUCT;
                SonType *t = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
                t->kind = TYPE_STRUCT;
                t->name = decl->name;
                t->decl = decl;
                sym->type = t;
                define_symbol(sema->global_scope, sym);
                break;
            }
            case DECL_UNION: {
                sym->kind = SYM_UNION;
                SonType *t = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
                t->kind = TYPE_UNION;
                t->name = decl->name;
                t->decl = decl;
                sym->type = t;
                define_symbol(sema->global_scope, sym);
                break;
            }
            case DECL_FUNCTION: {
                sym->kind = SYM_FUNCTION;
                SonType *t = (SonType*)son_arena_alloc(sema->arena, sizeof(SonType), sizeof(void*));
                t->kind = TYPE_FUNCTION;
                t->name = decl->name;
                sym->type = t;
                define_symbol(sema->global_scope, sym);
                break;
            }
            default:
                break;
        }
    }

    // Pass 2: Check function bodies and expressions
    for (size_t i = 0; i < module->decl_count; i++) {
        SonDecl *decl = module->decls[i];
        if (decl->kind == DECL_FUNCTION) {
            if (decl->is_extern) {
                continue;
            }
            if (decl->func.return_type) {
                decl->func.return_type = monomorphize_type(sema, decl->func.return_type);
            }
            for (size_t p = 0; p < decl->func.param_count; p++) {
                if (decl->func.params[p].type) {
                    decl->func.params[p].type = monomorphize_type(sema, decl->func.params[p].type);
                }
            }

            push_scope(sema, true, decl->func.return_type);

            // Add parameters to scope
            for (size_t p = 0; p < decl->func.param_count; p++) {
                SonParam *param = &decl->func.params[p];
                SonSymbol *p_sym = alloc_symbol(sema->arena);
                p_sym->kind = SYM_VARIABLE;
                p_sym->name = param->name;
                p_sym->type = param->type;
                p_sym->loc = decl->loc;
                p_sym->is_mut = false;
                p_sym->next = NULL;
                define_symbol(sema->current_scope, p_sym);
            }

            // Analyze function body statements
            for (size_t s = 0; s < decl->func.body.stmt_count; s++) {
                analyze_stmt(sema, decl->func.body.stmts[s]);
            }

            pop_scope(sema);
        }
    }

    return !sema->has_error;
}
