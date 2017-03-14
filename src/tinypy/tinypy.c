/*
The tinypy License

Copyright (c) 2008 Phil Hassey

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef TINYPY_H
#define TINYPY_H
#ifndef TP_H
#define TP_H

#include <setjmp.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#define tp_malloc(x) calloc((x),1)
#define tp_realloc(x,y) realloc(x,y)
#define tp_free(x) free(x)

// #include <gc/gc.h>
// #define tp_malloc(x) GC_MALLOC(x)
// #define tp_realloc(x,y) GC_REALLOC(x,y)
// #define tp_free(x)

enum {
    TP_NONE,TP_NUMBER,TP_STRING,TP_DICT,
    TP_LIST,TP_FNC,TP_DATA,
};

typedef double tp_num;

typedef struct tp_number_ {
    int type;
    tp_num val;
} tp_number_;
typedef struct tp_string_ {
    int type;
    struct _tp_string *info;
    char *val;
    int len;
} tp_string_;
typedef struct tp_list_ {
    int type;
    struct _tp_list *val;
} tp_list_;
typedef struct tp_dict_ {
    int type;
    struct _tp_dict *val;
} tp_dict_;
typedef struct tp_fnc_ {
    int type;
    struct _tp_fnc *val;
    int ftype;
    void *fval;
} tp_fnc_;
typedef struct tp_data_ {
    int type;
    struct _tp_data *info;
    void *val;
    struct tp_meta *meta;
} tp_data_;

typedef union tp_obj {
    int type;
    tp_number_ number;
    struct { int type; int *data; } gci;
    tp_string_ string;
    tp_dict_ dict;
    tp_list_ list;
    tp_fnc_ fnc;
    tp_data_ data;
} tp_obj;

typedef struct _tp_string {
    int gci;
    char s[1];
} _tp_string;
typedef struct _tp_list {
    int gci;
    tp_obj *items;
    int len;
    int alloc;
} _tp_list;
typedef struct tp_item {
    int used;
    int hash;
    tp_obj key;
    tp_obj val;
} tp_item;
typedef struct _tp_dict {
    int gci;
    tp_item *items;
    int len;
    int alloc;
    int cur;
    int mask;
    int used;
} _tp_dict;
typedef struct _tp_fnc {
    int gci;
    tp_obj self;
    tp_obj globals;
} _tp_fnc;


typedef union tp_code {
    unsigned char i;
    struct { unsigned char i,a,b,c; } regs;
    struct { char val[4]; } string;
    struct { float val; } number;
} tp_code;

typedef struct tp_frame_ {
    tp_code *codes;
    tp_code *cur;
    tp_code *jmp;
    tp_obj *regs;
    tp_obj *ret_dest;
    tp_obj fname;
    tp_obj name;
    tp_obj line;
    tp_obj globals;
    int lineno;
    int cregs;
} tp_frame_;

#define TP_GCMAX 4096
#define TP_FRAMES 256
// #define TP_REGS_PER_FRAME 256
#define TP_REGS 16384
typedef struct tp_vm {
    tp_obj builtins;
    tp_obj modules;
    tp_frame_ frames[TP_FRAMES];
    tp_obj _params;
    tp_obj params;
    tp_obj _regs;
    tp_obj *regs;
    tp_obj root;
    jmp_buf buf;
    int jmp;
    tp_obj ex;
    char chars[256][2];
    int cur;
    // gc
    _tp_list *white;
    _tp_list *grey;
    _tp_list *black;
    _tp_dict *strings;
    int steps;
} tp_vm;

#define TP tp_vm *tp
typedef struct tp_meta {
    int type;
    tp_obj (*get)(TP,tp_obj,tp_obj);
    void (*set)(TP,tp_obj,tp_obj,tp_obj);
    void (*free)(TP,tp_obj);
//     tp_obj (*del)(TP,tp_obj,tp_obj);
//     tp_obj (*has)(TP,tp_obj,tp_obj);
//     tp_obj (*len)(TP,tp_obj);
} tp_meta;
typedef struct _tp_data {
    int gci;
    tp_meta meta;
} _tp_data;

// NOTE: these are the few out of namespace items for convenience
#define None ((tp_obj){TP_NONE})
#define True tp_number(1)
#define False tp_number(0)
#define STR(v) ((tp_str(tp,(v))).string.val)

void tp_set(TP,tp_obj,tp_obj,tp_obj);
tp_obj tp_get(TP,tp_obj,tp_obj);
tp_obj tp_len(TP,tp_obj);
tp_obj tp_str(TP,tp_obj);
int tp_cmp(TP,tp_obj,tp_obj);
void _tp_raise(TP,tp_obj);
tp_obj tp_printf(TP,char *fmt,...);
tp_obj tp_track(TP,tp_obj);
void tp_grey(TP,tp_obj);

// __func__ __VA_ARGS__ __FILE__ __LINE__
#define tp_raise(r,fmt,...) { \
    _tp_raise(tp,tp_printf(tp,fmt,__VA_ARGS__)); \
    return r; \
}
#define __params (tp->params)
#define TP_OBJ() (tp_get(tp,__params,None))
inline static tp_obj tp_type(TP,int t,tp_obj v) {
    if (v.type != t) { tp_raise(None,"_tp_type(%d,%s)",t,STR(v)); }
    return v;
}
#define TP_TYPE(t) tp_type(tp,t,TP_OBJ())
#define TP_NUM() (TP_TYPE(TP_NUMBER).number.val)
#define TP_STR() (STR(TP_TYPE(TP_STRING)))
#define TP_DEFAULT(d) (__params.list.val->len?tp_get(tp,__params,None):(d))
#define TP_LOOP(e) \
    int __l = __params.list.val->len; \
    int __i; for (__i=0; __i<__l; __i++) { \
    (e) = _tp_list_get(tp,__params.list.val,__i,"TP_LOOP");
#define TP_END \
    }

inline static int _tp_min(int a, int b) { return (a<b?a:b); }
inline static int _tp_max(int a, int b) { return (a>b?a:b); }
inline static int _tp_sign(tp_num v) { return (v<0?-1:(v>0?1:0)); }
inline static tp_obj tp_number(tp_num v) { return (tp_obj)(tp_number_){TP_NUMBER,v}; }
inline static tp_obj tp_string(char *v) { return (tp_obj)(tp_string_){TP_STRING,0,v,strlen(v)}; }
inline static tp_obj tp_string_n(char *v,int n) {
    return (tp_obj)(tp_string_){TP_STRING,0,v,n};
}

#endif
//
void _tp_list_realloc(_tp_list *self,int len) ;
void _tp_list_set(TP,_tp_list *self,int k, tp_obj v, char *error) ;
void _tp_list_free(_tp_list *self) ;
tp_obj _tp_list_get(TP,_tp_list *self,int k,char *error) ;
void _tp_list_insertx(TP,_tp_list *self, int n, tp_obj v) ;
void _tp_list_appendx(TP,_tp_list *self, tp_obj v) ;
void _tp_list_insert(TP,_tp_list *self, int n, tp_obj v) ;
void _tp_list_append(TP,_tp_list *self, tp_obj v) ;
tp_obj _tp_list_pop(TP,_tp_list *self, int n, char *error) ;
int _tp_list_find(TP,_tp_list *self, tp_obj v) ;
tp_obj tp_index(TP) ;
_tp_list *_tp_list_new(void) ;
tp_obj _tp_list_copy(TP, tp_obj rr) ;
tp_obj tp_append(TP) ;
tp_obj tp_pop(TP) ;
tp_obj tp_insert(TP) ;
tp_obj tp_extend(TP) ;
tp_obj tp_list(TP) ;
tp_obj tp_list_n(TP,int n,tp_obj *argv) ;
int _tp_sort_cmp(tp_obj *a,tp_obj *b) ;
tp_obj tp_sort(TP) ;
int tp_lua_hash(void *v,int l) ;
void _tp_dict_free(_tp_dict *self) ;
// void _tp_dict_reset(_tp_dict *self) ;
int tp_hash(TP,tp_obj v) ;
void _tp_dict_hash_set(TP,_tp_dict *self, int hash, tp_obj k, tp_obj v) ;
void _tp_dict_tp_realloc(TP,_tp_dict *self,int len) ;
int _tp_dict_hash_find(TP,_tp_dict *self, int hash, tp_obj k) ;
int _tp_dict_find(TP,_tp_dict *self,tp_obj k) ;
void _tp_dict_setx(TP,_tp_dict *self,tp_obj k, tp_obj v) ;
void _tp_dict_set(TP,_tp_dict *self,tp_obj k, tp_obj v) ;
tp_obj _tp_dict_get(TP,_tp_dict *self,tp_obj k, char *error) ;
void _tp_dict_del(TP,_tp_dict *self,tp_obj k, char *error) ;
_tp_dict *_tp_dict_new(void) ;
tp_obj _tp_dict_copy(TP,tp_obj rr) ;
int _tp_dict_next(TP,_tp_dict *self) ;
tp_obj tp_merge(TP) ;
tp_obj tp_dict(TP) ;
tp_obj tp_dict_n(TP,int n, tp_obj* argv) ;
tp_obj *tp_ptr(tp_obj o) ;
tp_obj _tp_dcall(TP,tp_obj fnc(TP)) ;
tp_obj _tp_tcall(TP,tp_obj fnc) ;
tp_obj tp_fnc_new(TP,int t, void *v, tp_obj s, tp_obj g) ;
tp_obj tp_def(TP,void *v, tp_obj g) ;
tp_obj tp_fnc(TP,tp_obj v(TP)) ;
tp_obj tp_method(TP,tp_obj self,tp_obj v(TP)) ;
tp_obj tp_data(TP,void *v) ;
tp_obj tp_params(TP) ;
tp_obj tp_params_n(TP,int n, tp_obj argv[]) ;
tp_obj tp_params_v(TP,int n,...) ;
tp_obj tp_string_t(TP, int n) ;
tp_obj tp_printf(TP,char *fmt,...) ;
int _tp_str_index(tp_obj s, tp_obj k) ;
tp_obj tp_join(TP) ;
tp_obj tp_string_slice(TP,tp_obj s, int a, int b) ;
tp_obj tp_split(TP) ;
tp_obj tp_find(TP) ;
tp_obj tp_str_index(TP) ;
tp_obj tp_str2(TP) ;
tp_obj tp_chr(TP) ;
tp_obj tp_ord(TP) ;
tp_obj tp_strip(TP) ;
tp_obj tp_replace(TP) ;
tp_obj tp_print(TP) ;
tp_obj tp_bind(TP) ;
tp_obj tp_min(TP) ;
tp_obj tp_max(TP) ;
tp_obj tp_copy(TP) ;
tp_obj tp_len_(TP) ;
tp_obj tp_assert(TP) ;
tp_obj tp_range(TP) ;
tp_obj tp_system(TP) ;
tp_obj tp_istype(TP) ;
tp_obj tp_float(TP) ;
tp_obj tp_save(TP) ;
tp_obj tp_load(TP) ;
tp_obj tp_fpack(TP) ;
tp_obj tp_abs(TP) ;
tp_obj tp_int(TP) ;
tp_num _roundf(tp_num v) ;
tp_obj tp_round(TP) ;
tp_obj tp_exists(TP) ;
tp_obj tp_mtime(TP) ;
void tp_grey(TP,tp_obj v) ;
void tp_follow(TP,tp_obj v) ;
void tp_reset(TP) ;
void tp_gc_init(TP) ;
void tp_gc_deinit(TP) ;
void tp_delete(TP,tp_obj v) ;
void tp_collect(TP) ;
void _tp_gcinc(TP) ;
void tp_full(TP) ;
void tp_gcinc(TP) ;
tp_obj tp_track(TP,tp_obj v) ;
tp_obj tp_str(TP,tp_obj self) ;
int tp_bool(TP,tp_obj v) ;
tp_obj tp_has(TP,tp_obj self, tp_obj k) ;
void tp_del(TP,tp_obj self, tp_obj k) ;
tp_obj tp_iter(TP,tp_obj self, tp_obj k) ;
tp_obj tp_get(TP,tp_obj self, tp_obj k) ;
int tp_iget(TP,tp_obj *r, tp_obj self, tp_obj k) ;
void tp_set(TP,tp_obj self, tp_obj k, tp_obj v) ;
tp_obj tp_add(TP,tp_obj a, tp_obj b) ;
tp_obj tp_mul(TP,tp_obj a, tp_obj b) ;
tp_obj tp_len(TP,tp_obj self) ;
int tp_cmp(TP,tp_obj a, tp_obj b) ;
tp_vm *_tp_init(void) ;
void tp_deinit(TP) ;
void tp_frame(TP,tp_obj globals,tp_code *codes,tp_obj *ret_dest) ;
void _tp_raise(TP,tp_obj e) ;
void tp_print_stack(TP) ;
void tp_handle(TP) ;
void _tp_call(TP,tp_obj *dest, tp_obj fnc, tp_obj params) ;
void tp_return(TP, tp_obj v) ;
int tp_step(TP) ;
void tp_run(TP,int cur) ;
tp_obj tp_call(TP, char *mod, char *fnc, tp_obj params) ;
tp_obj tp_import(TP,char *fname, char *name, void *codes) ;
tp_obj tp_exec_(TP) ;
tp_obj tp_import_(TP) ;
tp_obj tp_builtins(TP) ;
void tp_args(TP,int argc, char *argv[]) ;
tp_obj tp_main(TP,char *fname, void *code) ;
tp_obj tp_compile(TP, tp_obj text, tp_obj fname) ;
tp_obj tp_exec(TP,tp_obj code, tp_obj globals) ;
tp_obj tp_eval(TP, char *text, tp_obj globals) ;
tp_vm *tp_init(int argc, char *argv[]) ;
void tp_compiler(TP) ;
#endif

extern unsigned char tp_tokenize[];
extern unsigned char tp_parse[];
extern unsigned char tp_encode[];
extern unsigned char tp_py2bc[];
void _tp_list_realloc(_tp_list *self,int len) {
    if (!len) { len=1; }
    self->items = tp_realloc(self->items,len*sizeof(tp_obj));
    self->alloc = len;
}

void _tp_list_set(TP,_tp_list *self,int k, tp_obj v, char *error) {
    if (k >= self->len) { tp_raise(,"%s: KeyError: %d\n",error,k); }
    self->items[k] = v;
    tp_grey(tp,v);
}
void _tp_list_free(_tp_list *self) {
    tp_free(self->items);
    tp_free(self);
}

tp_obj _tp_list_get(TP,_tp_list *self,int k,char *error) {
    if (k >= self->len) { tp_raise(None,"%s: KeyError: %d\n",error,k); }
    return self->items[k];
}
void _tp_list_insertx(TP,_tp_list *self, int n, tp_obj v) {
    if (self->len >= self->alloc) {
        _tp_list_realloc(self,self->alloc*2);
    }
    if (n < self->len) { memmove(&self->items[n+1],&self->items[n],sizeof(tp_obj)*(self->len-n)); }
    self->items[n] = v;
    self->len += 1;
}
void _tp_list_appendx(TP,_tp_list *self, tp_obj v) {
    _tp_list_insertx(tp,self,self->len,v);
}
void _tp_list_insert(TP,_tp_list *self, int n, tp_obj v) {
    _tp_list_insertx(tp,self,n,v);
    tp_grey(tp,v);
}
void _tp_list_append(TP,_tp_list *self, tp_obj v) {
    _tp_list_insert(tp,self,self->len,v);
}
tp_obj _tp_list_pop(TP,_tp_list *self, int n, char *error) {
    tp_obj r = _tp_list_get(tp,self,n,error);
    if (n != self->len-1) { memmove(&self->items[n],&self->items[n+1],sizeof(tp_obj)*(self->len-(n+1))); }
    self->len -= 1;
    return r;
}

int _tp_list_find(TP,_tp_list *self, tp_obj v) {
    int n;
    for (n=0; n<self->len; n++) {
        if (tp_cmp(tp,v,self->items[n]) == 0) {
            return n;
        }
    }
    return -1;
}

tp_obj tp_index(TP) {
    tp_obj self = TP_OBJ();
    tp_obj v = TP_OBJ();
    int i = _tp_list_find(tp,self.list.val,v);
    if (i < 0) { tp_raise(None,"tp_index(%s,%s) - item not found",STR(self),STR(v)); }
    return tp_number(i);
}

_tp_list *_tp_list_new(void) {
    return tp_malloc(sizeof(_tp_list));
}

tp_obj _tp_list_copy(TP, tp_obj rr) {
    _tp_list *o = rr.list.val;
    _tp_list *r = _tp_list_new(); *r = *o;
    r->items = tp_malloc(sizeof(tp_obj)*o->alloc);
    memcpy(r->items,o->items,sizeof(tp_obj)*o->alloc);
    return tp_track(tp,(tp_obj)(tp_list_){TP_LIST,r});
}

tp_obj tp_append(TP) {
    tp_obj self = TP_OBJ();
    tp_obj v = TP_OBJ();
    _tp_list_append(tp,self.list.val,v);
    return None;
}

tp_obj tp_pop(TP) {
    tp_obj self = TP_OBJ();
    return _tp_list_pop(tp,self.list.val,self.list.val->len-1,"pop");
}

tp_obj tp_insert(TP) {
    tp_obj self = TP_OBJ();
    int n = TP_NUM();
    tp_obj v = TP_OBJ();
    _tp_list_insert(tp,self.list.val,n,v);
    return None;
}

tp_obj tp_extend(TP) {
    tp_obj self = TP_OBJ();
    tp_obj v = TP_OBJ();
    int i;
    for (i=0; i<v.list.val->len; i++) {
        _tp_list_append(tp,self.list.val,v.list.val->items[i]);
    }
    return None;
}

tp_obj tp_list(TP) {
    tp_obj r = (tp_obj)(tp_list_){TP_LIST,_tp_list_new()};
    return tp?tp_track(tp,r):r;
}

tp_obj tp_list_n(TP,int n,tp_obj *argv) {
    tp_obj r = tp_list(tp); _tp_list_realloc(r.list.val,n);
    int i; for (i=0; i<n; i++) { _tp_list_append(tp,r.list.val,argv[i]); }
    return r;
}

int _tp_sort_cmp(tp_obj *a,tp_obj *b) {
    return tp_cmp(0,*a,*b);
}
tp_obj tp_sort(TP) {
    tp_obj self = TP_OBJ();
    qsort(self.list.val->items, self.list.val->len, sizeof(tp_obj), (int(*)(const void*,const void*))_tp_sort_cmp);
    return None;
}



//


int tp_lua_hash(void *v,int l) {
    int i,step = (l>>5)+1;
    int h = l + (l >= 4?*(int*)v:0);
    for (i=l; i>=step; i-=step) {
        h = h^((h<<5)+(h>>2)+((unsigned char *)v)[i-1]);
    }
    return h;
}
void _tp_dict_free(_tp_dict *self) {
    tp_free(self->items);
    tp_free(self);
}

// void _tp_dict_reset(_tp_dict *self) {
//     memset(self->items,0,self->alloc*sizeof(tp_item));
//     self->len = 0;
//     self->used = 0;
//     self->cur = 0;
// }

int tp_hash(TP,tp_obj v) {
    switch (v.type) {
        case TP_NONE: return 0;
        case TP_NUMBER: return tp_lua_hash(&v.number.val,sizeof(tp_num));
        case TP_STRING: return tp_lua_hash(v.string.val,v.string.len);
        case TP_DICT: return tp_lua_hash(&v.dict.val,sizeof(void*));
        case TP_LIST: {
            int r = v.list.val->len; int n; for(n=0; n<v.list.val->len; n++) {
            tp_obj vv = v.list.val->items[n]; r += vv.type != TP_LIST?tp_hash(tp,v.list.val->items[n]):tp_lua_hash(&vv.list.val,sizeof(void*)); } return r;
        }
        case TP_FNC: return tp_lua_hash(&v.fnc.val,sizeof(void*));
        case TP_DATA: return tp_lua_hash(&v.data.val,sizeof(void*));
    }
    tp_raise(0,"tp_hash(%s)",STR(v));
}

void _tp_dict_hash_set(TP,_tp_dict *self, int hash, tp_obj k, tp_obj v) {
    int i,idx = hash&self->mask;
    for (i=idx; i<idx+self->alloc; i++) {
        int n = i&self->mask;
        if (self->items[n].used > 0) { continue; }
        if (self->items[n].used == 0) { self->used += 1; }
        self->items[n] = (tp_item){1,hash,k,v};
        self->len += 1;
        return;
    }
    tp_raise(,"_tp_dict_hash_set(%d,%d,%s,%s)",self,hash,STR(k),STR(v));
}

void _tp_dict_tp_realloc(TP,_tp_dict *self,int len) {
    len = _tp_max(8,len);
    tp_item *items = self->items;
    int i,alloc = self->alloc;

    self->items = tp_malloc(len*sizeof(tp_item));
    self->alloc = len; self->mask = len-1;
    self->len = 0; self->used = 0;

    for (i=0; i<alloc; i++) {
        if (items[i].used != 1) { continue; }
        _tp_dict_hash_set(tp,self,items[i].hash,items[i].key,items[i].val);
    }
    tp_free(items);
}

int _tp_dict_hash_find(TP,_tp_dict *self, int hash, tp_obj k) {
    int i,idx = hash&self->mask;
    for (i=idx; i<idx+self->alloc; i++) {
        int n = i&self->mask;
        if (self->items[n].used == 0) { break; }
        if (self->items[n].used < 0) { continue; }
        if (self->items[n].hash != hash) { continue; }
        if (tp_cmp(tp,self->items[n].key,k) != 0) { continue; }
        return n;
    }
    return -1;
}
int _tp_dict_find(TP,_tp_dict *self,tp_obj k) {
    return _tp_dict_hash_find(tp,self,tp_hash(tp,k),k);
}

void _tp_dict_setx(TP,_tp_dict *self,tp_obj k, tp_obj v) {
    int hash = tp_hash(tp,k); int n = _tp_dict_hash_find(tp,self,hash,k);
    if (n == -1) {
        if (self->len >= (self->alloc/2)) {
            _tp_dict_tp_realloc(tp,self,self->alloc*2);
        } else if (self->used >= (self->alloc*3/4)) {
            _tp_dict_tp_realloc(tp,self,self->alloc);
        }
        _tp_dict_hash_set(tp,self,hash,k,v);
    } else {
        self->items[n].val = v;
    }
}

void _tp_dict_set(TP,_tp_dict *self,tp_obj k, tp_obj v) {
    _tp_dict_setx(tp,self,k,v);
    tp_grey(tp,k); tp_grey(tp,v);
}

tp_obj _tp_dict_get(TP,_tp_dict *self,tp_obj k, char *error) {
    int n = _tp_dict_find(tp,self,k);
    if (n < 0) {
        tp_raise(None,"%s: KeyError: %s\n",error,STR(k));
    }
    return self->items[n].val;
}

void _tp_dict_del(TP,_tp_dict *self,tp_obj k, char *error) {
    int n = _tp_dict_find(tp,self,k);
    if (n < 0) { tp_raise(,"%s: KeyError: %s\n",error,STR(k)); }
    self->items[n].used = -1;
    self->len -= 1;
}

_tp_dict *_tp_dict_new(void) {
    _tp_dict *self = tp_malloc(sizeof(_tp_dict));
    return self;
}
tp_obj _tp_dict_copy(TP,tp_obj rr) {
    _tp_dict *o = rr.dict.val;
    _tp_dict *r = _tp_dict_new(); *r = *o;
    r->items = tp_malloc(sizeof(tp_item)*o->alloc);
    memcpy(r->items,o->items,sizeof(tp_item)*o->alloc);
    return tp_track(tp,(tp_obj)(tp_dict_){TP_DICT,r});
}

int _tp_dict_next(TP,_tp_dict *self) {
    if (!self->len) { tp_raise(0,"_tp_dict_next(...)",0); }
    while (1) {
        self->cur = ((self->cur + 1) & self->mask);
        if (self->items[self->cur].used > 0) {
            return self->cur;
        }
    }
}

tp_obj tp_merge(TP) {
    tp_obj self = TP_OBJ();
    tp_obj v = TP_OBJ();
    int i; for (i=0; i<v.dict.val->len; i++) {
        int n = _tp_dict_next(tp,v.dict.val);
        _tp_dict_set(tp,self.dict.val,
            v.dict.val->items[n].key,v.dict.val->items[n].val);
    }
    return None;
}

tp_obj tp_dict(TP) {
    tp_obj r = (tp_obj)(tp_dict_){TP_DICT,_tp_dict_new()};
    return tp?tp_track(tp,r):r;
}

tp_obj tp_dict_n(TP,int n, tp_obj* argv) {
    tp_obj r = tp_dict(tp);
    int i; for (i=0; i<n; i++) { tp_set(tp,r,argv[i*2],argv[i*2+1]); }
    return r;
}


tp_obj *tp_ptr(tp_obj o) {
    tp_obj *ptr = tp_malloc(sizeof(tp_obj)); *ptr = o;
    return ptr;
}

tp_obj _tp_dcall(TP,tp_obj fnc(TP)) {
    return fnc(tp);
}
tp_obj _tp_tcall(TP,tp_obj fnc) {
    if (fnc.fnc.ftype&2) {
        _tp_list_insert(tp,tp->params.list.val,0,fnc.fnc.val->self);
    }
    return _tp_dcall(tp,fnc.fnc.fval);
}

tp_obj tp_fnc_new(TP,int t, void *v, tp_obj s, tp_obj g) {
    tp_obj r;
    r.type = TP_FNC;
    _tp_fnc *self = tp_malloc(sizeof(_tp_fnc));
    self->self = s;
    self->globals = g;
    r.fnc.ftype = t;
    r.fnc.val = self;
    r.fnc.fval = v;
    return tp_track(tp,r);
}

tp_obj tp_def(TP,void *v, tp_obj g) {
    return tp_fnc_new(tp,1,v,None,g);
}

tp_obj tp_fnc(TP,tp_obj v(TP)) {
    return tp_fnc_new(tp,0,v,None,None);
}
tp_obj tp_method(TP,tp_obj self,tp_obj v(TP)) {
    return tp_fnc_new(tp,2,v,self,None);
}

tp_obj tp_data(TP,void *v) {
    tp_obj r = (tp_obj)(tp_data_){TP_DATA,tp_malloc(sizeof(_tp_data)),v,0};
    r.data.meta = &r.data.info->meta;
    return tp_track(tp,r);
}

tp_obj tp_params(TP) {
    tp->params = tp->_params.list.val->items[tp->cur];
    tp_obj r = tp->_params.list.val->items[tp->cur];
    r.list.val->len = 0;
    return r;
}
tp_obj tp_params_n(TP,int n, tp_obj argv[]) {
    tp_obj r = tp_params(tp);
    int i; for (i=0; i<n; i++) { _tp_list_append(tp,r.list.val,argv[i]); }
    return r;
}
tp_obj tp_params_v(TP,int n,...) {
    tp_obj r = tp_params(tp);
    va_list a; va_start(a,n);
    int i; for(i=0; i<n; i++) { _tp_list_append(tp,r.list.val,va_arg(a,tp_obj)); }
    va_end(a);
    return r;
}

//

tp_obj tp_string_t(TP, int n) {
    tp_obj r = tp_string_n(0,n);
    r.string.info = tp_malloc(sizeof(_tp_string)+n);
    r.string.val = r.string.info->s;
    return r;
}

tp_obj tp_printf(TP,char *fmt,...) {
    va_list arg;
    va_start(arg, fmt);
    int l = vsnprintf(NULL, 0, fmt,arg);
    tp_obj r = tp_string_t(tp,l);
    char *s = r.string.val;
    va_end(arg);
    va_start(arg, fmt);
    vsprintf(s,fmt,arg);
    va_end(arg);
    return tp_track(tp,r);
}

int _tp_str_index(tp_obj s, tp_obj k) {
    int i=0;
    while ((s.string.len - i) >= k.string.len) {
        if (memcmp(s.string.val+i,k.string.val,k.string.len) == 0) {
            return i;
        }
        i += 1;
    }
    return -1;
}

tp_obj tp_join(TP) {
    tp_obj delim = TP_OBJ();
    tp_obj val = TP_OBJ();
    int l=0,i;
    for (i=0; i<val.list.val->len; i++) {
        if (i!=0) { l += delim.string.len; }
        l += tp_str(tp,val.list.val->items[i]).string.len;
    }
    tp_obj r = tp_string_t(tp,l);
    char *s = r.string.val;
    l = 0;
    for (i=0; i<val.list.val->len; i++) {
        if (i!=0) {
            memcpy(s+l,delim.string.val,delim.string.len); l += delim.string.len;
        }
        tp_obj e = tp_str(tp,val.list.val->items[i]);
        memcpy(s+l,e.string.val,e.string.len); l += e.string.len;
    }
    return tp_track(tp,r);
}

tp_obj tp_string_slice(TP,tp_obj s, int a, int b) {
    tp_obj r = tp_string_t(tp,b-a);
    char *m = r.string.val;
    memcpy(m,s.string.val+a,b-a);
    return tp_track(tp,r);
}

tp_obj tp_split(TP) {
    tp_obj v = TP_OBJ();
    tp_obj d = TP_OBJ();
    tp_obj r = tp_list(tp);

    int i;
    while ((i=_tp_str_index(v,d))!=-1) {
        _tp_list_append(tp,r.list.val,tp_string_slice(tp,v,0,i));
        v.string.val += i + d.string.len; v.string.len -= i + d.string.len;
//         tp_grey(tp,r); // should stop gc or something instead
    }
    _tp_list_append(tp,r.list.val,tp_string_slice(tp,v,0,v.string.len));
//     tp_grey(tp,r); // should stop gc or something instead
    return r;
}


tp_obj tp_find(TP) {
    tp_obj s = TP_OBJ();
    tp_obj v = TP_OBJ();
    return tp_number(_tp_str_index(s,v));
}

tp_obj tp_str_index(TP) {
    tp_obj s = TP_OBJ();
    tp_obj v = TP_OBJ();
    int n = _tp_str_index(s,v);
    if (n >= 0) { return tp_number(n); }
    tp_raise(None,"tp_str_index(%s,%s)",s,v);
}

tp_obj tp_str2(TP) {
    tp_obj v = TP_OBJ();
    return tp_str(tp,v);
}

tp_obj tp_chr(TP) {
    int v = TP_NUM();
    return tp_string_n(tp->chars[(unsigned char)v],1);
}
tp_obj tp_ord(TP) {
    char *s = TP_STR();
    return tp_number(s[0]);
}

tp_obj tp_strip(TP) {
    char *v = TP_STR();
    int i, l = strlen(v); int a = l, b = 0;
    for (i=0; i<l; i++) {
        if (v[i] != ' ' && v[i] != '\n' && v[i] != '\t' && v[i] != '\r') {
            a = _tp_min(a,i); b = _tp_max(b,i+1);
        }
    }
    if ((b-a) < 0) { return tp_string(""); }
    tp_obj r = tp_string_t(tp,b-a);
    char *s = r.string.val;
    memcpy(s,v+a,b-a);
    return tp_track(tp,r);
}


tp_obj tp_replace(TP) {
    tp_obj s = TP_OBJ();
    tp_obj k = TP_OBJ();
    tp_obj v = TP_OBJ();
    tp_obj p = s;
    int i,n = 0;
    while ((i = _tp_str_index(p,k)) != -1) {
        n += 1;
        p.string.val += i + k.string.len; p.string.len -= i + k.string.len;
    }
//     fprintf(stderr,"ns: %d\n",n);
    int l = s.string.len + n * (v.string.len-k.string.len);
    int c;
    tp_obj rr = tp_string_t(tp,l);
    char *r = rr.string.val;
    char *d = r;
    tp_obj z; z = p = s;
    while ((i = _tp_str_index(p,k)) != -1) {
        p.string.val += i; p.string.len -= i;
        memcpy(d,z.string.val,c=(p.string.val-z.string.val)); d += c;
        p.string.val += k.string.len; p.string.len -= k.string.len;
        memcpy(d,v.string.val,v.string.len); d += v.string.len;
        z = p;
    }
    memcpy(d,z.string.val,(s.string.val + s.string.len) - z.string.val);

    return tp_track(tp,rr);
}

tp_obj tp_print(TP) {
    int n = 0;
    tp_obj e;
    TP_LOOP(e)
        if (n) { printf(" "); }
        printf("%s",STR(e));
        n += 1;
    TP_END;
    printf("\n");
    return None;
}

tp_obj tp_bind(TP) {
    tp_obj r = TP_OBJ();
    tp_obj self = TP_OBJ();
    return tp_fnc_new(tp,r.fnc.ftype|2,r.fnc.fval,self,r.fnc.val->globals);
}

tp_obj tp_min(TP) {
    tp_obj r = TP_OBJ();
    tp_obj e;
    TP_LOOP(e)
        if (tp_cmp(tp,r,e) > 0) { r = e; }
    TP_END;
    return r;
}

tp_obj tp_max(TP) {
    tp_obj r = TP_OBJ();
    tp_obj e;
    TP_LOOP(e)
        if (tp_cmp(tp,r,e) < 0) { r = e; }
    TP_END;
    return r;
}

tp_obj tp_copy(TP) {
    tp_obj r = TP_OBJ();
    int type = r.type;
    if (type == TP_LIST) {
        return _tp_list_copy(tp,r);
    } else if (type == TP_DICT) {
        return _tp_dict_copy(tp,r);
    }
    tp_raise(None,"tp_copy(%s)",STR(r));
}


tp_obj tp_len_(TP) {
    tp_obj e = TP_OBJ();
    return tp_len(tp,e);
}


tp_obj tp_assert(TP) {
    int a = TP_NUM();
    if (a) { return None; }
    tp_raise(None,"%s","assert failed");
}

tp_obj tp_range(TP) {
    int a = TP_NUM();
    int b = TP_NUM();
    int c = TP_DEFAULT(tp_number(1)).number.val;
    tp_obj r = tp_list(tp);
    int i;
    for (i=a; i!=b; i+=c) { _tp_list_append(tp,r.list.val,tp_number(i)); }
    return r;
}


tp_obj tp_system(TP) {
    char *s = TP_STR();
    int r = system(s);
    return tp_number(r);
}

tp_obj tp_istype(TP) {
    tp_obj v = TP_OBJ();
    char *t = TP_STR();
    if (strcmp("string",t) == 0) { return tp_number(v.type == TP_STRING); }
    if (strcmp("list",t) == 0) { return tp_number(v.type == TP_LIST); }
    if (strcmp("dict",t) == 0) { return tp_number(v.type == TP_DICT); }
    if (strcmp("number",t) == 0) { return tp_number(v.type == TP_NUMBER); }
    tp_raise(None,"is_type(%s,%s)",STR(v),t);
}


tp_obj tp_float(TP) {
    tp_obj v = TP_OBJ();
    int ord = TP_DEFAULT(tp_number(0)).number.val;
    int type = v.type;
    if (type == TP_NUMBER) { return v; }
    if (type == TP_STRING) {
        if (strchr(STR(v),'.')) { return tp_number(atof(STR(v))); }
        return(tp_number(strtol(STR(v),0,ord)));
    }
    tp_raise(None,"tp_float(%s)",STR(v));
}


tp_obj tp_save(TP) {
    char *fname = TP_STR();
    tp_obj v = TP_OBJ();
    FILE *f;
    f = fopen(fname,"wb");
    if (!f) { tp_raise(None,"tp_save(%s,...)",fname); }
    fwrite(v.string.val,v.string.len,1,f);
    fclose(f);
    return None;
}

tp_obj tp_load(TP) {
    char *fname = TP_STR();
    struct stat stbuf; stat(fname, &stbuf); long l = stbuf.st_size;
    FILE *f;
    f = fopen(fname,"rb");
    if (!f) { tp_raise(None,"tp_load(%s)",fname); }
    tp_obj r = tp_string_t(tp,l);
    char *s = r.string.val;
    fread(s,1,l,f);
    fclose(f);
    return tp_track(tp,r);
}


tp_obj tp_fpack(TP) {
    tp_num v = TP_NUM();
    tp_obj r = tp_string_t(tp,sizeof(tp_num));
    *(tp_num*)r.string.val = v;
    return tp_track(tp,r);
}

tp_obj tp_abs(TP) {
    return tp_number(fabs(tp_float(tp).number.val));
}
tp_obj tp_int(TP) {
    return tp_number((long)tp_float(tp).number.val);
}
tp_num _roundf(tp_num v) {
    tp_num av = fabs(v); tp_num iv = (long)av;
    av = (av-iv < 0.5?iv:iv+1);
    return (v<0?-av:av);
}
tp_obj tp_round(TP) {
    return tp_number(_roundf(tp_float(tp).number.val));
}

tp_obj tp_exists(TP) {
    char *s = TP_STR();
    struct stat stbuf;
    return tp_number(!stat(s,&stbuf));
}
tp_obj tp_mtime(TP) {
    char *s = TP_STR();
    struct stat stbuf;
    if (!stat(s,&stbuf)) { return tp_number(stbuf.st_mtime); }
    tp_raise(None,"tp_mtime(%s)",s);
}
// tp_obj tp_track(TP,tp_obj v) { return v; }
// void tp_grey(TP,tp_obj v) { }
// void tp_full(TP) { }
// void tp_gc_init(TP) { }
// void tp_gc_deinit(TP) { }
// void tp_delete(TP,tp_obj v) { }

void tp_grey(TP,tp_obj v) {
    if (v.type < TP_STRING || (!v.gci.data) || *v.gci.data) { return; }
    *v.gci.data = 1;
    if (v.type == TP_STRING || v.type == TP_DATA) {
        _tp_list_appendx(tp,tp->black,v);
        return;
    }
    _tp_list_appendx(tp,tp->grey,v);
}

void tp_follow(TP,tp_obj v) {
    int type = v.type;
    if (type == TP_LIST) {
        int n;
        for (n=0; n<v.list.val->len; n++) {
            tp_grey(tp,v.list.val->items[n]);
        }
    }
    if (type == TP_DICT) {
        int i;
        for (i=0; i<v.dict.val->len; i++) {
            int n = _tp_dict_next(tp,v.dict.val);
            tp_grey(tp,v.dict.val->items[n].key);
            tp_grey(tp,v.dict.val->items[n].val);
        }
    }
    if (type == TP_FNC) {
        tp_grey(tp,v.fnc.val->self);
        tp_grey(tp,v.fnc.val->globals);
    }
}

void tp_reset(TP) {
    int n;
    for (n=0; n<tp->black->len; n++) {
        *tp->black->items[n].gci.data = 0;
    }
    _tp_list *tmp = tp->white; tp->white = tp->black; tp->black = tmp;
}

void tp_gc_init(TP) {
    tp->white = _tp_list_new();
    tp->strings = _tp_dict_new();
    tp->grey = _tp_list_new();
    tp->black = _tp_list_new();
    tp->steps = 0;
}

void tp_gc_deinit(TP) {
    _tp_list_free(tp->white);
    _tp_dict_free(tp->strings);
    _tp_list_free(tp->grey);
    _tp_list_free(tp->black);
}

void tp_delete(TP,tp_obj v) {
    int type = v.type;
    if (type == TP_LIST) {
        _tp_list_free(v.list.val);
        return;
    } else if (type == TP_DICT) {
        _tp_dict_free(v.dict.val);
        return;
    } else if (type == TP_STRING) {
        tp_free(v.string.info);
        return;
    } else if (type == TP_DATA) {
        if (v.data.meta && v.data.meta->free) {
            v.data.meta->free(tp,v);
        }
        tp_free(v.data.info);
        return;
    } else if (type == TP_FNC) {
        tp_free(v.fnc.val);
        return;
    }
    tp_raise(,"tp_delete(%s)",STR(v));
}

void tp_collect(TP) {
    int n;
    for (n=0; n<tp->white->len; n++) {
        tp_obj r = tp->white->items[n];
        if (*r.gci.data) { continue; }
        if (r.type == TP_STRING) {
            //this can't be moved into tp_delete, because tp_delete is
            // also used by tp_track_s to delete redundant strings
            _tp_dict_del(tp,tp->strings,r,"tp_collect");
        }
        tp_delete(tp,r);
    }
    tp->white->len = 0;
    tp_reset(tp);
}

void _tp_gcinc(TP) {
    if (!tp->grey->len) { return; }
    tp_obj v = _tp_list_pop(tp,tp->grey,tp->grey->len-1,"_tp_gcinc");
    tp_follow(tp,v);
    _tp_list_appendx(tp,tp->black,v);
}

void tp_full(TP) {
    while (tp->grey->len) {
        _tp_gcinc(tp);
    }
    tp_collect(tp);
    tp_follow(tp,tp->root);
}

void tp_gcinc(TP) {
    tp->steps += 1;
    if (tp->steps < TP_GCMAX || tp->grey->len > 0) {
        _tp_gcinc(tp); _tp_gcinc(tp);
    }
    if (tp->steps < TP_GCMAX || tp->grey->len > 0) { return; }
    tp->steps = 0;
    tp_full(tp);
    return;
}

tp_obj tp_track(TP,tp_obj v) {
    if (v.type == TP_STRING) {
        int i = _tp_dict_find(tp,tp->strings,v);
        if (i != -1) {
            tp_delete(tp,v);
            v = tp->strings->items[i].key;
            tp_grey(tp,v);
            return v;
        }
        _tp_dict_setx(tp,tp->strings,v,True);
    }
    tp_gcinc(tp);
    tp_grey(tp,v);
    return v;
}

//


tp_obj tp_str(TP,tp_obj self) {
    int type = self.type;
    if (type == TP_STRING) { return self; }
    if (type == TP_NUMBER) {
        tp_num v = self.number.val;
        if ((fabs(v)-fabs((long)v)) < 0.000001) { return tp_printf(tp,"%ld",(long)v); }
        return tp_printf(tp,"%f",v);
    } else if(type == TP_DICT) {
        return tp_printf(tp,"<dict 0x%x>",self.dict.val);
    } else if(type == TP_LIST) {
        return tp_printf(tp,"<list 0x%x>",self.list.val);
    } else if (type == TP_NONE) {
        return tp_string("None");
    } else if (type == TP_DATA) {
        return tp_printf(tp,"<data 0x%x>",self.data.val);
    } else if (type == TP_FNC) {
        return tp_printf(tp,"<fnc 0x%x>",self.fnc.val);
    }
    return tp_string("<?>");
}

int tp_bool(TP,tp_obj v) {
    switch(v.type) {
        case TP_NUMBER: return v.number.val != 0;
        case TP_NONE: return 0;
        case TP_STRING: return v.string.len != 0;
        case TP_LIST: return v.list.val->len != 0;
        case TP_DICT: return v.dict.val->len != 0;
    }
    return 1;
}


tp_obj tp_has(TP,tp_obj self, tp_obj k) {
    int type = self.type;
    if (type == TP_DICT) {
        if (_tp_dict_find(tp,self.dict.val,k) != -1) { return True; }
        return False;
    } else if (type == TP_STRING && k.type == TP_STRING) {
        char *p = strstr(STR(self),STR(k));
        return tp_number(p != 0);
    } else if (type == TP_LIST) {
        return tp_number(_tp_list_find(tp,self.list.val,k)!=-1);
    }
    tp_raise(None,"tp_has(%s,%s)",STR(self),STR(k));
}

void tp_del(TP,tp_obj self, tp_obj k) {
    int type = self.type;
    if (type == TP_DICT) {
        _tp_dict_del(tp,self.dict.val,k,"tp_del");
        return;
    }
    tp_raise(,"tp_del(%s,%s)",STR(self),STR(k));
}


tp_obj tp_iter(TP,tp_obj self, tp_obj k) {
    int type = self.type;
    if (type == TP_LIST || type == TP_STRING) { return tp_get(tp,self,k); }
    if (type == TP_DICT && k.type == TP_NUMBER) {
        return self.dict.val->items[_tp_dict_next(tp,self.dict.val)].key;
    }
    tp_raise(None,"tp_iter(%s,%s)",STR(self),STR(k));
}

tp_obj tp_get(TP,tp_obj self, tp_obj k) {
    int type = self.type;
    if (type == TP_DICT) {
        return _tp_dict_get(tp,self.dict.val,k,"tp_get");
    } else if (type == TP_LIST) {
        if (k.type == TP_NUMBER) {
            int l = tp_len(tp,self).number.val;
            int n = k.number.val;
            n = (n<0?l+n:n);
            return _tp_list_get(tp,self.list.val,n,"tp_get");
        } else if (k.type == TP_STRING) {
            if (strcmp("append",STR(k)) == 0) {
                return tp_method(tp,self,tp_append);
            } else if (strcmp("pop",STR(k)) == 0) {
                return tp_method(tp,self,tp_pop);
            } else if (strcmp("index",STR(k)) == 0) {
                return tp_method(tp,self,tp_index);
            } else if (strcmp("sort",STR(k)) == 0) {
                return tp_method(tp,self,tp_sort);
            } else if (strcmp("extend",STR(k)) == 0) {
                return tp_method(tp,self,tp_extend);
            } else if (strcmp("*",STR(k)) == 0) {
                tp_params_v(tp,1,self); tp_obj r = tp_copy(tp);
                self.list.val->len=0;
                return r;
            }
        } else if (k.type == TP_NONE) {
            return _tp_list_pop(tp,self.list.val,0,"tp_get");
        }
    } else if (type == TP_STRING) {
        if (k.type == TP_NUMBER) {
            int l = self.string.len;
            int n = k.number.val;
            n = (n<0?l+n:n);
            if (n >= 0 && n < l) { return tp_string_n(tp->chars[(unsigned char)self.string.val[n]],1); }
        } else if (k.type == TP_STRING) {
            if (strcmp("join",STR(k)) == 0) {
                return tp_method(tp,self,tp_join);
            } else if (strcmp("split",STR(k)) == 0) {
                return tp_method(tp,self,tp_split);
            } else if (strcmp("index",STR(k)) == 0) {
                return tp_method(tp,self,tp_str_index);
            } else if (strcmp("strip",STR(k)) == 0) {
                return tp_method(tp,self,tp_strip);
            } else if (strcmp("replace",STR(k)) == 0) {
                return tp_method(tp,self,tp_replace);
            }
        }
    } else if (type == TP_DATA) {
        return self.data.meta->get(tp,self,k);
    }

    if (k.type == TP_LIST) {
        int a,b,l;
        tp_obj tmp;
        l = tp_len(tp,self).number.val;
        tmp = tp_get(tp,k,tp_number(0));
        if (tmp.type == TP_NUMBER) { a = tmp.number.val; }
        else if(tmp.type == TP_NONE) { a = 0; }
        else { tp_raise(None,"%s is not a number",STR(tmp)); }
        tmp = tp_get(tp,k,tp_number(1));
        if (tmp.type == TP_NUMBER) { b = tmp.number.val; }
        else if(tmp.type == TP_NONE) { b = l; }
        else { tp_raise(None,"%s is not a number",STR(tmp)); }
        a = _tp_max(0,(a<0?l+a:a)); b = _tp_min(l,(b<0?l+b:b));
        if (type == TP_LIST) {
            return tp_list_n(tp,b-a,&self.list.val->items[a]);
        } else if (type == TP_STRING) {
            tp_obj r = tp_string_t(tp,b-a);
            char *ptr = r.string.val;
            memcpy(ptr,self.string.val+a,b-a); ptr[b-a]=0;
            return tp_track(tp,r);
        }
    }



    tp_raise(None,"tp_get(%s,%s)",STR(self),STR(k));
}

int tp_iget(TP,tp_obj *r, tp_obj self, tp_obj k) {
    if (self.type == TP_DICT) {
        int n = _tp_dict_find(tp,self.dict.val,k);
        if (n == -1) { return 0; }
        *r = self.dict.val->items[n].val;
        tp_grey(tp,*r);
        return 1;
    }
    if (self.type == TP_LIST && !self.list.val->len) { return 0; }
    *r = tp_get(tp,self,k); tp_grey(tp,*r);
    return 1;
}

void tp_set(TP,tp_obj self, tp_obj k, tp_obj v) {
    int type = self.type;

    if (type == TP_DICT) {
        _tp_dict_set(tp,self.dict.val,k,v);
        return;
    } else if (type == TP_LIST) {
        if (k.type == TP_NUMBER) {
            _tp_list_set(tp,self.list.val,k.number.val,v,"tp_set");
            return;
        } else if (k.type == TP_NONE) {
            _tp_list_append(tp,self.list.val,v);
            return;
        } else if (k.type == TP_STRING) {
            if (strcmp("*",STR(k)) == 0) {
                tp_params_v(tp,2,self,v); tp_extend(tp);
                return;
            }
        }
    } else if (type == TP_DATA) {
        self.data.meta->set(tp,self,k,v);
        return;
    }
    tp_raise(,"tp_set(%s,%s,%s)",STR(self),STR(k),STR(v));
}

tp_obj tp_add(TP,tp_obj a, tp_obj b) {
    if (a.type == TP_NUMBER && a.type == b.type) {
        return tp_number(a.number.val+b.number.val);
    } else if (a.type == TP_STRING && a.type == b.type) {
        int al = a.string.len, bl = b.string.len;
        tp_obj r = tp_string_t(tp,al+bl);
        char *s = r.string.val;
        memcpy(s,a.string.val,al); memcpy(s+al,b.string.val,bl);
        return tp_track(tp,r);
    } else if (a.type == TP_LIST && a.type == b.type) {
        tp_params_v(tp,1,a); tp_obj r = tp_copy(tp);
        tp_params_v(tp,2,r,b); tp_extend(tp);
        return r;
    }
    tp_raise(None,"tp_add(%s,%s)",STR(a),STR(b));
}

tp_obj tp_mul(TP,tp_obj a, tp_obj b) {
    if (a.type == TP_NUMBER && a.type == b.type) {
        return tp_number(a.number.val*b.number.val);
    } else if (a.type == TP_STRING && b.type == TP_NUMBER) {
        int al = a.string.len; int n = b.number.val;
        tp_obj r = tp_string_t(tp,al*n);
        char *s = r.string.val;
        int i; for (i=0; i<n; i++) { memcpy(s+al*i,a.string.val,al); }
        return tp_track(tp,r);
    }
    tp_raise(None,"tp_mul(%s,%s)",STR(a),STR(b));
}


tp_obj tp_len(TP,tp_obj self) {
    int type = self.type;
    if (type == TP_STRING) {
        return tp_number(self.string.len);
    } else if (type == TP_DICT) {
        return tp_number(self.dict.val->len);
    } else if (type == TP_LIST) {
        return tp_number(self.list.val->len);
    }
    tp_raise(None,"tp_len(%s)",STR(self));
}

int tp_cmp(TP,tp_obj a, tp_obj b) {
    if (a.type != b.type) { return a.type-b.type; }
    switch(a.type) {
        case TP_NONE: return 0;
        case TP_NUMBER: return _tp_sign(a.number.val-b.number.val);
        case TP_STRING: {
            int v = memcmp(a.string.val,b.string.val,_tp_min(a.string.len,b.string.len));
            if (v == 0) { v = a.string.len-b.string.len; }
            return v;
        }
        case TP_LIST: {
            int n,v; for(n=0;n<_tp_min(a.list.val->len,b.list.val->len);n++) {
        tp_obj aa = a.list.val->items[n]; tp_obj bb = b.list.val->items[n];
            if (aa.type == TP_LIST && bb.type == TP_LIST) { v = aa.list.val-bb.list.val; } else { v = tp_cmp(tp,aa,bb); }
            if (v) { return v; } }
            return a.list.val->len-b.list.val->len;
        }
        case TP_DICT: return a.dict.val - b.dict.val;
        case TP_FNC: return a.fnc.val - b.fnc.val;
        case TP_DATA: return a.data.val - b.data.val;
    }
    tp_raise(0,"tp_cmp(%s,%s)",STR(a),STR(b));
}

#define TP_OP(name,expr) \
    tp_obj name(TP,tp_obj _a,tp_obj _b) { \
    if (_a.type == TP_NUMBER && _a.type == _b.type) { \
        tp_num a = _a.number.val; tp_num b = _b.number.val; \
        return tp_number(expr); \
    } \
    tp_raise(None,"%s(%s,%s)",#name,STR(_a),STR(_b)); \
}

TP_OP(tp_and,((long)a)&((long)b));
TP_OP(tp_or,((long)a)|((long)b));
TP_OP(tp_mod,((long)a)%((long)b));
TP_OP(tp_lsh,((long)a)<<((long)b));
TP_OP(tp_rsh,((long)a)>>((long)b));
TP_OP(tp_sub,a-b);
TP_OP(tp_div,a/b);
TP_OP(tp_pow,pow(a,b));


//

tp_vm *_tp_init(void) {
    int i;
    tp_vm *tp = tp_malloc(sizeof(tp_vm));
    tp->cur = 0;
    tp->jmp = 0;
    tp->ex = None;
    tp->root = tp_list(0);
    for (i=0; i<256; i++) { tp->chars[i][0]=i; }
    tp_gc_init(tp);
    tp->_regs = tp_list(tp);
    for (i=0; i<TP_REGS; i++) { tp_set(tp,tp->_regs,None,None); }
    tp->builtins = tp_dict(tp);
    tp->modules = tp_dict(tp);
    tp->_params = tp_list(tp);
    for (i=0; i<TP_FRAMES; i++) { tp_set(tp,tp->_params,None,tp_list(tp)); }
    tp_set(tp,tp->root,None,tp->builtins);
    tp_set(tp,tp->root,None,tp->modules);
    tp_set(tp,tp->root,None,tp->_regs);
    tp_set(tp,tp->root,None,tp->_params);
    tp_set(tp,tp->builtins,tp_string("MODULES"),tp->modules);
    tp_set(tp,tp->modules,tp_string("BUILTINS"),tp->builtins);
    tp_set(tp,tp->builtins,tp_string("BUILTINS"),tp->builtins);
    tp->regs = tp->_regs.list.val->items;
    tp_full(tp);
    return tp;
}

void tp_deinit(TP) {
    while (tp->root.list.val->len) {
        _tp_list_pop(tp,tp->root.list.val,0,"tp_deinit");
    }
    tp_full(tp); tp_full(tp);
    tp_delete(tp,tp->root);
    tp_gc_deinit(tp);
    tp_free(tp);
}


// tp_frame_
void tp_frame(TP,tp_obj globals,tp_code *codes,tp_obj *ret_dest) {
    tp_frame_ f;
    f.globals = globals;
    f.codes = codes;
    f.cur = f.codes;
    f.jmp = 0;
//     fprintf(stderr,"tp->cur: %d\n",tp->cur);
    f.regs = (tp->cur <= 0?tp->regs:tp->frames[tp->cur].regs+tp->frames[tp->cur].cregs);
    f.ret_dest = ret_dest;
    f.lineno = 0;
    f.line = tp_string("");
    f.name = tp_string("?");
    f.fname = tp_string("?");
    f.cregs = 0;
//     return f;
    if (f.regs+256 >= tp->regs+TP_REGS || tp->cur >= TP_FRAMES-1) { tp_raise(,"tp_frame: stack overflow %d",tp->cur); }
    tp->cur += 1;
    tp->frames[tp->cur] = f;
}

void _tp_raise(TP,tp_obj e) {
    if (!tp || !tp->jmp) {
        printf("\nException:\n%s\n",STR(e));
        exit(-1);
        return;
    }
    if (e.type != TP_NONE) { tp->ex = e; }
    tp_grey(tp,e);
    longjmp(tp->buf,1);
}

void tp_print_stack(TP) {
    printf("\n");
    int i;
    for (i=0; i<=tp->cur; i++) {
        if (!tp->frames[i].lineno) { continue; }
        printf("File \"%s\", line %d, in %s\n  %s\n",
            STR(tp->frames[i].fname),tp->frames[i].lineno,STR(tp->frames[i].name),STR(tp->frames[i].line));
    }
    printf("\nException:\n%s\n",STR(tp->ex));
}



void tp_handle(TP) {
    int i;
    for (i=tp->cur; i>=0; i--) {
        if (tp->frames[i].jmp) { break; }
    }
    if (i >= 0) {
        tp->cur = i;
        tp->frames[i].cur = tp->frames[i].jmp;
        tp->frames[i].jmp = 0;
        return;
    }
    tp_print_stack(tp);
    exit(-1);
}

void _tp_call(TP,tp_obj *dest, tp_obj fnc, tp_obj params) {
    if (fnc.type == TP_DICT) {
        return _tp_call(tp,dest,tp_get(tp,fnc,tp_string("__call__")),params);
    }
    if (fnc.type == TP_FNC && !(fnc.fnc.ftype&1)) {
        *dest = _tp_tcall(tp,fnc);
        tp_grey(tp,*dest);
        return;
    }
    if (fnc.type == TP_FNC) {
        tp_frame(tp,fnc.fnc.val->globals,fnc.fnc.fval,dest);
        if ((fnc.fnc.ftype&2)) {
            tp->frames[tp->cur].regs[0] = params;
            _tp_list_insert(tp,params.list.val,0,fnc.fnc.val->self);
        } else {
            tp->frames[tp->cur].regs[0] = params;
        }
        return;
    }
    tp_params_v(tp,1,fnc); tp_print(tp);
    tp_raise(,"tp_call: %s is not callable",STR(fnc));
}


void tp_return(TP, tp_obj v) {
    tp_obj *dest = tp->frames[tp->cur].ret_dest;
    if (dest) { *dest = v; tp_grey(tp,v); }
//     memset(tp->frames[tp->cur].regs,0,TP_REGS_PER_FRAME*sizeof(tp_obj));
//     fprintf(stderr,"regs:%d\n",(tp->frames[tp->cur].cregs+1));
    memset(tp->frames[tp->cur].regs,0,tp->frames[tp->cur].cregs*sizeof(tp_obj));
    tp->cur -= 1;
}

enum {
    TP_IEOF,TP_IADD,TP_ISUB,TP_IMUL,TP_IDIV,TP_IPOW,TP_IAND,TP_IOR,TP_ICMP,TP_IGET,TP_ISET,
    TP_INUMBER,TP_ISTRING,TP_IGGET,TP_IGSET,TP_IMOVE,TP_IDEF,TP_IPASS,TP_IJUMP,TP_ICALL,
    TP_IRETURN,TP_IIF,TP_IDEBUG,TP_IEQ,TP_ILE,TP_ILT,TP_IDICT,TP_ILIST,TP_INONE,TP_ILEN,
    TP_ILINE,TP_IPARAMS,TP_IIGET,TP_IFILE,TP_INAME,TP_INE,TP_IHAS,TP_IRAISE,TP_ISETJMP,
    TP_IMOD,TP_ILSH,TP_IRSH,TP_IITER,TP_IDEL,TP_IREGS,
    TP_ITOTAL
};

// char *tp_strings[TP_ITOTAL] = {
//     "EOF","ADD","SUB","MUL","DIV","POW","AND","OR","CMP","GET","SET","NUM",
//     "STR","GGET","GSET","MOVE","DEF","PASS","JUMP","CALL","RETURN","IF","DEBUG",
//     "EQ","LE","LT","DICT","LIST","NONE","LEN","LINE","PARAMS","IGET","FILE",
//     "NAME","NE","HAS","RAISE","SETJMP","MOD","LSH","RSH","ITER","DEL","REGS",
// };

#define VA ((int)e.regs.a)
#define VB ((int)e.regs.b)
#define VC ((int)e.regs.c)
#define RA regs[e.regs.a]
#define RB regs[e.regs.b]
#define RC regs[e.regs.c]
#define UVBC (unsigned short)(((VB<<8)+VC))
#define SVBC (short)(((VB<<8)+VC))
#define GA tp_grey(tp,RA)
#define SR(v) f->cur = cur; return(v);

int tp_step(TP) {
    tp_frame_ *f = &tp->frames[tp->cur];
    tp_obj *regs = f->regs;
    tp_code *cur = f->cur;
    while(1) {
    tp_code e = *cur;
//     fprintf(stderr,"%2d.%4d: %-6s %3d %3d %3d\n",tp->cur,cur-f->codes,tp_strings[e.i],VA,VB,VC);
//     int i; for(i=0;i<16;i++) { fprintf(stderr,"%d: %s\n",i,STR(regs[i])); }
    switch (e.i) {
        case TP_IEOF: tp_return(tp,None); SR(0); break;
        case TP_IADD: RA = tp_add(tp,RB,RC); break;
        case TP_ISUB: RA = tp_sub(tp,RB,RC); break;
        case TP_IMUL: RA = tp_mul(tp,RB,RC); break;
        case TP_IDIV: RA = tp_div(tp,RB,RC); break;
        case TP_IPOW: RA = tp_pow(tp,RB,RC); break;
        case TP_IAND: RA = tp_and(tp,RB,RC); break;
        case TP_IOR:  RA = tp_or(tp,RB,RC); break;
        case TP_IMOD:  RA = tp_mod(tp,RB,RC); break;
        case TP_ILSH:  RA = tp_lsh(tp,RB,RC); break;
        case TP_IRSH:  RA = tp_rsh(tp,RB,RC); break;
        case TP_ICMP: RA = tp_number(tp_cmp(tp,RB,RC)); break;
        case TP_INE: RA = tp_number(tp_cmp(tp,RB,RC)!=0); break;
        case TP_IEQ: RA = tp_number(tp_cmp(tp,RB,RC)==0); break;
        case TP_ILE: RA = tp_number(tp_cmp(tp,RB,RC)<=0); break;
        case TP_ILT: RA = tp_number(tp_cmp(tp,RB,RC)<0); break;
        case TP_IPASS: break;
        case TP_IIF: if (tp_bool(tp,RA)) { cur += 1; } break;
        case TP_IGET: RA = tp_get(tp,RB,RC); GA; break;
        case TP_IITER:
            if (RC.number.val < tp_len(tp,RB).number.val) {
                RA = tp_iter(tp,RB,RC); GA;
                RC.number.val += 1;
                cur += 1;
            }
            break;
        case TP_IHAS: RA = tp_has(tp,RB,RC); break;
        case TP_IIGET: tp_iget(tp,&RA,RB,RC); break;
        case TP_ISET: tp_set(tp,RA,RB,RC); break;
        case TP_IDEL: tp_del(tp,RA,RB); break;
        case TP_IMOVE: RA = RB; break;
        case TP_INUMBER:
            RA = tp_number(*(tp_num*)(*++cur).string.val);
            cur += sizeof(tp_num)/4;
            continue;
        case TP_ISTRING:
            RA = tp_string_n((*(cur+1)).string.val,UVBC);
            cur += (UVBC/4)+1;
            break;
        case TP_IDICT: RA = tp_dict_n(tp,VC/2,&RB); break;
        case TP_ILIST: RA = tp_list_n(tp,VC,&RB); break;
        case TP_IPARAMS: RA = tp_params_n(tp,VC,&RB); break;
        case TP_ILEN: RA = tp_len(tp,RB); break;
        case TP_IJUMP: cur += SVBC; continue; break;
        case TP_ISETJMP: f->jmp = cur+SVBC; break;
        case TP_ICALL: _tp_call(tp,&RA,RB,RC); cur++; SR(0); break;
        case TP_IGGET:
            if (!tp_iget(tp,&RA,f->globals,RB)) {
                RA = tp_get(tp,tp->builtins,RB); GA;
            }
            break;
        case TP_IGSET: tp_set(tp,f->globals,RA,RB); break;
        case TP_IDEF:
            RA = tp_def(tp,(*(cur+1)).string.val,f->globals);
            cur += SVBC; continue;
            break;
        case TP_IRETURN: tp_return(tp,RA); SR(0); break;
        case TP_IRAISE: _tp_raise(tp,RA); SR(0); break;
        case TP_IDEBUG:
            tp_params_v(tp,3,tp_string("DEBUG:"),tp_number(VA),RA); tp_print(tp);
            break;
        case TP_INONE: RA = None; break;
        case TP_ILINE:
            f->line = tp_string_n((*(cur+1)).string.val,VA*4-1);
//             fprintf(stderr,"%7d: %s\n",UVBC,f->line.string.val);
            cur += VA; f->lineno = UVBC;
            break;
        case TP_IFILE: f->fname = RA; break;
        case TP_INAME: f->name = RA; break;
        case TP_IREGS: f->cregs = VA; break;
        default: tp_raise(0,"tp_step: invalid instruction %d",e.i); break;
    }
    cur += 1;
    }
    SR(0);
}

void tp_run(TP,int cur) {
    if (tp->jmp) { tp_raise(,"tp_run(%d) called recusively",cur); }
    tp->jmp = 1; if (setjmp(tp->buf)) { tp_handle(tp); }
    while (tp->cur >= cur && tp_step(tp) != -1);
    tp->cur = cur-1; tp->jmp = 0;
}


tp_obj tp_call(TP, char *mod, char *fnc, tp_obj params) {
    tp_obj tmp;
    tp_obj r = None;
    tmp = tp_get(tp,tp->modules,tp_string(mod));
    tmp = tp_get(tp,tmp,tp_string(fnc));
    _tp_call(tp,&r,tmp,params);
    tp_run(tp,tp->cur);
    return r;
}

tp_obj tp_import(TP,char *fname, char *name, void *codes) {
    if (!((fname && strstr(fname,".tpc")) || codes)) {
        return tp_call(tp,"py2bc","import_fname",tp_params_v(tp,2,tp_string(fname),tp_string(name)));
    }

    tp_obj code = None;
    if (!codes) {
        tp_params_v(tp,1,tp_string(fname));
        code = tp_load(tp);
        codes = code.string.val;
    } else {
        code = tp_data(tp,codes);
    }

    tp_obj g = tp_dict(tp);
    tp_set(tp,g,tp_string("__name__"),tp_string(name));
    tp_set(tp,g,tp_string("__code__"),code);
    tp_frame(tp,g,codes,0);
    tp_set(tp,tp->modules,tp_string(name),g);

    if (!tp->jmp) { tp_run(tp,tp->cur); }

    return g;
}



tp_obj tp_exec_(TP) {
    tp_obj code = TP_OBJ();
    tp_obj globals = TP_OBJ();
    tp_frame(tp,globals,(void*)code.string.val,0);
    return None;
}


tp_obj tp_import_(TP) {
    tp_obj mod = TP_OBJ();

    if (tp_has(tp,tp->modules,mod).number.val) {
        return tp_get(tp,tp->modules,mod);
    }

    char *s = STR(mod);
    tp_obj r = tp_import(tp,STR(tp_add(tp,mod,tp_string(".tpc"))),s,0);
    return r;
}

#define TP_IB(a,b) tp_set(tp,ctx,tp_string(a),tp_fnc(tp,b))

tp_obj tp_builtins(TP) {
    tp_obj ctx = tp->builtins;
    TP_IB("print",tp_print); TP_IB("range",tp_range); TP_IB("min",tp_min);
    TP_IB("max",tp_max); TP_IB("bind",tp_bind); TP_IB("copy",tp_copy);
    TP_IB("import",tp_import_); TP_IB("len",tp_len_); TP_IB("assert",tp_assert);
    TP_IB("str",tp_str2); TP_IB("float",tp_float); TP_IB("system",tp_system);
    TP_IB("istype",tp_istype); TP_IB("chr",tp_chr); TP_IB("save",tp_save);
    TP_IB("load",tp_load); TP_IB("fpack",tp_fpack); TP_IB("abs",tp_abs);
    TP_IB("int",tp_int); TP_IB("exec",tp_exec_); TP_IB("exists",tp_exists);
    TP_IB("mtime",tp_mtime); TP_IB("number",tp_float); TP_IB("round",tp_round);
    TP_IB("ord",tp_ord); TP_IB("merge",tp_merge);
    return ctx;
}


void tp_args(TP,int argc, char *argv[]) {
    tp_obj self = tp_list(tp);
    int i;
    for (i=1; i<argc; i++) { _tp_list_append(tp,self.list.val,tp_string(argv[i])); }
    tp_set(tp,tp->builtins,tp_string("ARGV"),self);
}


tp_obj tp_main(TP,char *fname, void *code) {
    return tp_import(tp,fname,"__main__",code);
}
tp_obj tp_compile(TP, tp_obj text, tp_obj fname) {
    return tp_call(tp,"BUILTINS","compile",tp_params_v(tp,2,text,fname));
}

tp_obj tp_exec(TP,tp_obj code, tp_obj globals) {
    tp_obj r=None;
    tp_frame(tp,globals,(void*)code.string.val,&r);
    tp_run(tp,tp->cur);
    return r;
}

tp_obj tp_eval(TP, char *text, tp_obj globals) {
    tp_obj code = tp_compile(tp,tp_string(text),tp_string("<eval>"));
    return tp_exec(tp,code,globals);
}

tp_vm *tp_init(int argc, char *argv[]) {
    tp_vm *tp = _tp_init();
    tp_builtins(tp);
    tp_args(tp,argc,argv);
    tp_compiler(tp);
    return tp;
}


//
#ifndef TP_COMPILER
#define TP_COMPILER 1
#endif

void tp_compiler(TP);

#if TP_COMPILER
void tp_compiler(TP) {
    tp_import(tp,0,"tokenize",tp_tokenize);
    tp_import(tp,0,"parse",tp_parse);
    tp_import(tp,0,"encode",tp_encode);
    tp_import(tp,0,"py2bc",tp_py2bc);
    tp_call(tp,"py2bc","_init",None);
}
#else
void tp_compiler(TP) { }
#endif

//
unsigned char tp_tokenize[] = {
44,61,0,0,16,0,0,97,44,16,0,0,28,2,0,0,
9,1,0,2,28,4,0,0,9,3,0,4,28,6,0,0,
9,5,0,6,11,9,0,0,0,0,0,0,0,0,0,0,
9,8,5,9,15,7,8,0,11,10,0,0,0,0,0,0,
0,0,240,63,9,9,5,10,15,8,9,0,12,11,0,5,
115,112,108,105,116,0,0,0,9,10,3,11,12,11,0,1,
10,0,0,0,31,9,11,1,19,9,10,9,11,12,0,0,
0,0,0,0,0,0,240,63,2,11,7,12,9,9,9,11,
15,5,9,0,12,11,0,0,0,0,0,0,15,9,11,0,
11,12,0,0,0,0,0,0,0,0,36,64,25,11,7,12,
21,11,0,0,18,0,0,6,12,12,0,1,32,0,0,0,
1,11,9,12,15,9,11,0,18,0,0,1,11,12,0,0,
0,0,0,0,0,0,89,64,25,11,7,12,21,11,0,0,
18,0,0,6,12,12,0,2,32,32,0,0,1,11,9,12,
15,9,11,0,18,0,0,1,12,15,0,3,115,116,114,0,
13,14,15,0,15,15,7,0,31,13,15,1,19,13,14,13,
1,12,9,13,12,13,0,2,58,32,0,0,1,12,12,13,
1,12,12,5,12,13,0,1,10,0,0,0,1,12,12,13,
15,11,12,0,12,13,0,5,32,32,32,32,32,0,0,0,
12,15,0,1,32,0,0,0,3,15,15,8,1,13,13,15,
12,15,0,1,94,0,0,0,1,13,13,15,12,15,0,1,
10,0,0,0,1,13,13,15,1,12,11,13,15,11,12,0,
12,12,0,7,101,114,114,111,114,58,32,0,1,12,12,1,
12,13,0,1,10,0,0,0,1,12,12,13,1,12,12,11,
37,12,0,0,0,0,0,0,12,1,0,7,117,95,101,114,
114,111,114,0,14,1,0,0,12,1,0,8,73,83,89,77,
66,79,76,83,0,0,0,0,12,2,0,26,96,45,61,91,
93,59,44,46,47,126,33,64,36,37,94,38,42,40,41,43,
123,125,58,60,62,63,0,0,14,1,2,0,12,1,0,7,
83,89,77,66,79,76,83,0,12,3,0,3,100,101,102,0,
12,4,0,5,99,108,97,115,115,0,0,0,12,5,0,5,
121,105,101,108,100,0,0,0,12,6,0,6,114,101,116,117,
114,110,0,0,12,7,0,4,112,97,115,115,0,0,0,0,
12,8,0,3,97,110,100,0,12,9,0,2,111,114,0,0,
12,10,0,3,110,111,116,0,12,11,0,2,105,110,0,0,
12,12,0,6,105,109,112,111,114,116,0,0,12,13,0,2,
105,115,0,0,12,14,0,5,119,104,105,108,101,0,0,0,
12,15,0,5,98,114,101,97,107,0,0,0,12,16,0,3,
102,111,114,0,12,17,0,8,99,111,110,116,105,110,117,101,
0,0,0,0,12,18,0,2,105,102,0,0,12,19,0,4,
101,108,115,101,0,0,0,0,12,20,0,4,101,108,105,102,
0,0,0,0,12,21,0,3,116,114,121,0,12,22,0,6,
101,120,99,101,112,116,0,0,12,23,0,5,114,97,105,115,
101,0,0,0,12,24,0,4,84,114,117,101,0,0,0,0,
12,25,0,5,70,97,108,115,101,0,0,0,12,26,0,4,
78,111,110,101,0,0,0,0,12,27,0,6,103,108,111,98,
97,108,0,0,12,28,0,3,100,101,108,0,12,29,0,1,
45,0,0,0,12,30,0,1,43,0,0,0,12,31,0,1,
42,0,0,0,12,32,0,2,42,42,0,0,12,33,0,1,
47,0,0,0,12,34,0,1,37,0,0,0,12,35,0,2,
60,60,0,0,12,36,0,2,62,62,0,0,12,37,0,2,
45,61,0,0,12,38,0,2,43,61,0,0,12,39,0,2,
42,61,0,0,12,40,0,2,47,61,0,0,12,41,0,1,
61,0,0,0,12,42,0,2,61,61,0,0,12,43,0,2,
33,61,0,0,12,44,0,1,60,0,0,0,12,45,0,1,
62,0,0,0,12,46,0,2,60,61,0,0,12,47,0,2,
62,61,0,0,12,48,0,1,91,0,0,0,12,49,0,1,
93,0,0,0,12,50,0,1,123,0,0,0,12,51,0,1,
125,0,0,0,12,52,0,1,40,0,0,0,12,53,0,1,
41,0,0,0,12,54,0,1,46,0,0,0,12,55,0,1,
58,0,0,0,12,56,0,1,44,0,0,0,12,57,0,1,
59,0,0,0,12,58,0,1,38,0,0,0,12,59,0,1,
124,0,0,0,12,60,0,1,33,0,0,0,27,2,3,58,
14,1,2,0,12,3,0,1,91,0,0,0,12,4,0,1,
40,0,0,0,12,5,0,1,123,0,0,0,27,2,3,3,
15,1,2,0,12,5,0,1,93,0,0,0,12,6,0,1,
41,0,0,0,12,7,0,1,125,0,0,0,27,4,5,3,
15,3,4,0,12,5,0,7,66,95,66,69,71,73,78,0,
14,5,1,0,12,1,0,5,66,95,69,78,68,0,0,0,
14,1,3,0,26,1,0,0,12,3,0,5,84,68,97,116,
97,0,0,0,14,3,1,0,16,5,0,48,44,21,0,0,
28,2,0,0,9,1,0,2,11,4,0,0,0,0,0,0,
0,0,240,63,15,3,4,0,11,6,0,0,0,0,0,0,
0,0,0,0,15,5,6,0,11,8,0,0,0,0,0,0,
0,0,240,63,15,7,8,0,12,9,0,1,121,0,0,0,
10,1,9,3,12,10,0,2,121,105,0,0,10,1,10,5,
12,11,0,2,110,108,0,0,10,1,11,7,27,13,0,0,
15,12,13,0,11,16,0,0,0,0,0,0,0,0,0,0,
27,15,16,1,15,14,15,0,11,17,0,0,0,0,0,0,
0,0,0,0,15,16,17,0,12,18,0,3,114,101,115,0,
10,1,18,12,12,19,0,6,105,110,100,101,110,116,0,0,
10,1,19,14,12,20,0,6,98,114,97,99,101,115,0,0,
10,1,20,16,0,0,0,0,12,6,0,8,95,95,105,110,
105,116,95,95,0,0,0,0,10,1,6,5,16,6,0,32,
44,17,0,0,28,2,0,0,9,1,0,2,28,4,0,0,
9,3,0,4,28,6,0,0,9,5,0,6,12,9,0,3,
114,101,115,0,9,8,1,9,12,9,0,6,97,112,112,101,
110,100,0,0,9,8,8,9,12,10,0,4,102,114,111,109,
0,0,0,0,12,16,0,1,102,0,0,0,9,11,1,16,
12,12,0,4,116,121,112,101,0,0,0,0,15,13,3,0,
12,14,0,3,118,97,108,0,15,15,5,0,26,9,10,6,
31,7,9,1,19,7,8,7,0,0,0,0,12,7,0,3,
97,100,100,0,10,1,7,6,16,7,0,43,44,10,0,0,
28,2,0,0,9,1,0,2,12,5,0,4,98,105,110,100,
0,0,0,0,13,4,5,0,12,7,0,5,84,68,97,116,
97,0,0,0,13,5,7,0,12,7,0,8,95,95,105,110,
105,116,95,95,0,0,0,0,9,5,5,7,15,6,1,0,
31,3,5,2,19,3,4,3,12,5,0,8,95,95,105,110,
105,116,95,95,0,0,0,0,10,1,5,3,12,7,0,4,
98,105,110,100,0,0,0,0,13,6,7,0,12,9,0,5,
84,68,97,116,97,0,0,0,13,7,9,0,12,9,0,3,
97,100,100,0,9,7,7,9,15,8,1,0,31,3,7,2,
19,3,6,3,12,7,0,3,97,100,100,0,10,1,7,3,
0,0,0,0,12,8,0,7,95,95,110,101,119,95,95,0,
10,1,8,7,16,8,0,22,44,7,0,0,26,1,0,0,
12,4,0,5,84,68,97,116,97,0,0,0,13,3,4,0,
12,4,0,7,95,95,110,101,119,95,95,0,9,3,3,4,
15,4,1,0,31,2,4,1,19,2,3,2,12,5,0,8,
95,95,105,110,105,116,95,95,0,0,0,0,9,4,1,5,
19,6,4,0,20,1,0,0,0,0,0,0,12,9,0,8,
95,95,99,97,108,108,95,95,0,0,0,0,10,1,9,8,
16,10,0,28,44,8,0,0,28,2,0,0,9,1,0,2,
12,5,0,7,114,101,112,108,97,99,101,0,9,4,1,5,
12,5,0,2,13,10,0,0,12,6,0,1,10,0,0,0,
31,3,5,2,19,3,4,3,15,1,3,0,12,6,0,7,
114,101,112,108,97,99,101,0,9,5,1,6,12,6,0,1,
13,0,0,0,12,7,0,1,10,0,0,0,31,3,6,2,
19,3,5,3,15,1,3,0,20,1,0,0,0,0,0,0,
12,11,0,5,99,108,101,97,110,0,0,0,14,11,10,0,
16,11,0,41,44,11,0,0,28,2,0,0,9,1,0,2,
12,5,0,5,99,108,101,97,110,0,0,0,13,4,5,0,
15,5,1,0,31,3,5,1,19,3,4,3,15,1,3,0,
38,0,0,11,12,6,0,11,100,111,95,116,111,107,101,110,
105,122,101,0,13,5,6,0,15,6,1,0,31,3,6,1,
19,3,5,3,20,3,0,0,18,0,0,18,12,7,0,7,
117,95,101,114,114,111,114,0,13,6,7,0,12,7,0,8,
116,111,107,101,110,105,122,101,0,0,0,0,15,8,1,0,
12,10,0,1,84,0,0,0,13,9,10,0,12,10,0,1,
102,0,0,0,9,9,9,10,31,3,7,3,19,3,6,3,
0,0,0,0,12,12,0,8,116,111,107,101,110,105,122,101,
0,0,0,0,14,12,11,0,16,12,1,128,44,54,0,0,
28,2,0,0,9,1,0,2,12,6,0,5,84,68,97,116,
97,0,0,0,13,5,6,0,31,4,0,0,19,4,5,4,
15,3,4,0,11,7,0,0,0,0,0,0,0,0,0,0,
15,6,7,0,12,11,0,3,108,101,110,0,13,10,11,0,
15,11,1,0,31,9,11,1,19,9,10,9,15,8,9,0,
12,11,0,1,84,0,0,0,14,11,3,0,15,3,6,0,
15,6,8,0,12,11,0,1,84,0,0,0,13,8,11,0,
12,14,0,1,84,0,0,0,13,12,14,0,12,14,0,1,
121,0,0,0,9,12,12,14,12,15,0,1,84,0,0,0,
13,14,15,0,12,15,0,2,121,105,0,0,9,14,14,15,
2,13,3,14,11,14,0,0,0,0,0,0,0,0,240,63,
1,13,13,14,27,11,12,2,12,12,0,1,102,0,0,0,
10,8,12,11,25,11,3,6,21,11,0,0,18,0,1,53,
9,14,1,3,15,13,14,0,12,15,0,1,84,0,0,0,
13,14,15,0,12,18,0,1,84,0,0,0,13,16,18,0,
12,18,0,1,121,0,0,0,9,16,16,18,12,19,0,1,
84,0,0,0,13,18,19,0,12,19,0,2,121,105,0,0,
9,18,18,19,2,17,3,18,11,18,0,0,0,0,0,0,
0,0,240,63,1,17,17,18,27,15,16,2,12,16,0,1,
102,0,0,0,10,14,16,15,12,17,0,1,84,0,0,0,
13,15,17,0,12,17,0,2,110,108,0,0,9,15,15,17,
21,15,0,0,18,0,0,22,12,17,0,1,84,0,0,0,
13,15,17,0,11,17,0,0,0,0,0,0,0,0,0,0,
12,18,0,2,110,108,0,0,10,15,18,17,12,20,0,9,
100,111,95,105,110,100,101,110,116,0,0,0,13,19,20,0,
15,20,1,0,15,21,3,0,15,22,6,0,31,17,20,3,
19,17,19,17,15,3,17,0,18,0,0,253,12,20,0,1,
10,0,0,0,23,17,13,20,21,17,0,0,18,0,0,12,
12,21,0,5,100,111,95,110,108,0,0,0,13,20,21,0,
15,21,1,0,15,22,3,0,15,23,6,0,31,17,21,3,
19,17,20,17,15,3,17,0,18,0,0,237,12,21,0,8,
73,83,89,77,66,79,76,83,0,0,0,0,13,17,21,0,
36,17,17,13,21,17,0,0,18,0,0,13,12,22,0,9,
100,111,95,115,121,109,98,111,108,0,0,0,13,21,22,0,
15,22,1,0,15,23,3,0,15,24,6,0,31,17,22,3,
19,17,21,17,15,3,17,0,18,0,0,217,11,23,0,0,
0,0,0,0,0,0,0,0,12,17,0,1,48,0,0,0,
24,17,17,13,23,22,17,23,21,22,0,0,18,0,0,2,
18,0,0,4,12,24,0,1,57,0,0,0,24,17,13,24,
21,17,0,0,18,0,0,13,12,25,0,9,100,111,95,110,
117,109,98,101,114,0,0,0,13,24,25,0,15,25,1,0,
15,26,3,0,15,27,6,0,31,17,25,3,19,17,24,17,
15,3,17,0,18,0,0,190,11,26,0,0,0,0,0,0,
0,0,240,63,11,28,0,0,0,0,0,0,0,0,240,63,
11,30,0,0,0,0,0,0,0,0,0,0,12,17,0,1,
97,0,0,0,24,17,17,13,23,29,17,30,21,29,0,0,
18,0,0,2,18,0,0,4,12,31,0,1,122,0,0,0,
24,17,13,31,23,27,17,28,21,27,0,0,18,0,0,2,
18,0,0,14,11,32,0,0,0,0,0,0,0,0,0,0,
12,17,0,1,65,0,0,0,24,17,17,13,23,31,17,32,
21,31,0,0,18,0,0,2,18,0,0,4,12,33,0,1,
90,0,0,0,24,17,13,33,23,25,17,26,21,25,0,0,
18,0,0,2,18,0,0,4,12,33,0,1,95,0,0,0,
23,17,13,33,21,17,0,0,18,0,0,12,12,34,0,7,
100,111,95,110,97,109,101,0,13,33,34,0,15,34,1,0,
15,35,3,0,15,36,6,0,31,17,34,3,19,17,33,17,
15,3,17,0,18,0,0,134,11,35,0,0,0,0,0,0,
0,0,240,63,12,36,0,1,34,0,0,0,23,17,13,36,
23,34,17,35,21,34,0,0,18,0,0,2,18,0,0,4,
12,36,0,1,39,0,0,0,23,17,13,36,21,17,0,0,
18,0,0,13,12,37,0,9,100,111,95,115,116,114,105,110,
103,0,0,0,13,36,37,0,15,37,1,0,15,38,3,0,
15,39,6,0,31,17,37,3,19,17,36,17,15,3,17,0,
18,0,0,107,12,37,0,1,35,0,0,0,23,17,13,37,
21,17,0,0,18,0,0,13,12,38,0,10,100,111,95,99,
111,109,109,101,110,116,0,0,13,37,38,0,15,38,1,0,
15,39,3,0,15,40,6,0,31,17,38,3,19,17,37,17,
15,3,17,0,18,0,0,90,11,39,0,0,0,0,0,0,
0,0,0,0,12,40,0,1,92,0,0,0,23,17,13,40,
23,38,17,39,21,38,0,0,18,0,0,2,18,0,0,9,
11,41,0,0,0,0,0,0,0,0,240,63,1,40,3,41,
9,17,1,40,12,40,0,1,10,0,0,0,23,17,17,40,
21,17,0,0,18,0,0,31,11,40,0,0,0,0,0,0,
0,0,0,64,1,17,3,40,15,3,17,0,12,41,0,1,
84,0,0,0,13,40,41,0,12,41,0,1,121,0,0,0,
9,40,40,41,11,41,0,0,0,0,0,0,0,0,240,63,
1,40,40,41,15,17,40,0,15,41,3,0,12,43,0,1,
84,0,0,0,13,42,43,0,12,43,0,1,121,0,0,0,
10,42,43,17,12,45,0,1,84,0,0,0,13,44,45,0,
12,45,0,2,121,105,0,0,10,44,45,41,18,0,0,40,
11,48,0,0,0,0,0,0,0,0,240,63,12,49,0,1,
32,0,0,0,23,46,13,49,23,47,46,48,21,47,0,0,
18,0,0,2,18,0,0,4,12,49,0,1,9,0,0,0,
23,46,13,49,21,46,0,0,18,0,0,7,11,49,0,0,
0,0,0,0,0,0,240,63,1,46,3,49,15,3,46,0,
18,0,0,19,12,50,0,7,117,95,101,114,114,111,114,0,
13,49,50,0,12,50,0,8,116,111,107,101,110,105,122,101,
0,0,0,0,15,51,1,0,12,53,0,1,84,0,0,0,
13,52,53,0,12,53,0,1,102,0,0,0,9,52,52,53,
31,46,50,3,19,46,49,46,18,0,0,1,18,0,254,202,
12,52,0,6,105,110,100,101,110,116,0,0,13,51,52,0,
11,52,0,0,0,0,0,0,0,0,0,0,31,50,52,1,
19,50,51,50,12,53,0,1,84,0,0,0,13,52,53,0,
12,53,0,3,114,101,115,0,9,52,52,53,15,50,52,0,
12,52,0,1,84,0,0,0,28,53,0,0,14,52,53,0,
20,50,0,0,0,0,0,0,12,13,0,11,100,111,95,116,
111,107,101,110,105,122,101,0,14,13,12,0,16,13,0,75,
44,21,0,0,28,2,0,0,9,1,0,2,28,4,0,0,
9,3,0,4,28,6,0,0,9,5,0,6,11,7,0,0,
0,0,0,0,0,0,0,0,12,9,0,1,84,0,0,0,
13,8,9,0,12,9,0,6,98,114,97,99,101,115,0,0,
9,8,8,9,23,7,7,8,21,7,0,0,18,0,0,13,
12,9,0,1,84,0,0,0,13,8,9,0,12,9,0,3,
97,100,100,0,9,8,8,9,12,9,0,2,110,108,0,0,
28,10,0,0,31,7,9,2,19,7,8,7,18,0,0,1,
11,11,0,0,0,0,0,0,0,0,240,63,1,10,3,11,
15,9,10,0,11,12,0,0,0,0,0,0,0,0,240,63,
15,11,12,0,15,3,9,0,12,13,0,1,84,0,0,0,
13,9,13,0,12,13,0,2,110,108,0,0,10,9,13,11,
12,16,0,1,84,0,0,0,13,15,16,0,12,16,0,1,
121,0,0,0,9,15,15,16,11,16,0,0,0,0,0,0,
0,0,240,63,1,15,15,16,15,14,15,0,15,16,3,0,
12,18,0,1,84,0,0,0,13,17,18,0,12,18,0,1,
121,0,0,0,10,17,18,14,12,20,0,1,84,0,0,0,
13,19,20,0,12,20,0,2,121,105,0,0,10,19,20,16,
20,3,0,0,0,0,0,0,12,14,0,5,100,111,95,110,
108,0,0,0,14,14,13,0,16,14,0,90,44,21,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
28,6,0,0,9,5,0,6,11,8,0,0,0,0,0,0,
0,0,0,0,15,7,8,0,25,8,3,5,21,8,0,0,
18,0,0,33,9,10,1,3,15,9,10,0,11,12,0,0,
0,0,0,0,0,0,0,0,12,13,0,1,32,0,0,0,
35,10,9,13,23,11,10,12,21,11,0,0,18,0,0,2,
18,0,0,4,12,13,0,1,9,0,0,0,35,10,9,13,
21,10,0,0,18,0,0,3,18,0,0,15,18,0,0,1,
11,14,0,0,0,0,0,0,0,0,240,63,1,13,3,14,
15,10,13,0,11,16,0,0,0,0,0,0,0,0,240,63,
1,15,7,16,15,14,15,0,15,3,10,0,15,7,14,0,
18,0,255,222,11,16,0,0,0,0,0,0,0,0,0,0,
11,18,0,0,0,0,0,0,0,0,0,0,12,19,0,1,
10,0,0,0,35,10,9,19,23,17,10,18,21,17,0,0,
18,0,0,2,18,0,0,4,12,19,0,1,35,0,0,0,
35,10,9,19,23,14,10,16,21,14,0,0,18,0,0,2,
18,0,0,12,11,10,0,0,0,0,0,0,0,0,0,0,
12,20,0,1,84,0,0,0,13,19,20,0,12,20,0,6,
98,114,97,99,101,115,0,0,9,19,19,20,23,10,10,19,
21,10,0,0,18,0,0,9,12,20,0,6,105,110,100,101,
110,116,0,0,13,19,20,0,15,20,7,0,31,10,20,1,
19,10,19,10,18,0,0,1,20,3,0,0,0,0,0,0,
12,15,0,9,100,111,95,105,110,100,101,110,116,0,0,0,
14,15,14,0,16,15,0,137,44,14,0,0,28,2,0,0,
9,1,0,2,12,5,0,1,84,0,0,0,13,4,5,0,
12,5,0,6,105,110,100,101,110,116,0,0,9,4,4,5,
11,5,0,0,0,0,0,0,0,0,240,191,9,4,4,5,
23,3,1,4,21,3,0,0,18,0,0,3,17,0,0,0,
18,0,0,117,12,4,0,1,84,0,0,0,13,3,4,0,
12,4,0,6,105,110,100,101,110,116,0,0,9,3,3,4,
11,4,0,0,0,0,0,0,0,0,240,191,9,3,3,4,
25,3,3,1,21,3,0,0,18,0,0,28,12,5,0,1,
84,0,0,0,13,4,5,0,12,5,0,6,105,110,100,101,
110,116,0,0,9,4,4,5,12,5,0,6,97,112,112,101,
110,100,0,0,9,4,4,5,15,5,1,0,31,3,5,1,
19,3,4,3,12,6,0,1,84,0,0,0,13,5,6,0,
12,6,0,3,97,100,100,0,9,5,5,6,12,6,0,6,
105,110,100,101,110,116,0,0,15,7,1,0,31,3,6,2,
19,3,5,3,18,0,0,76,12,7,0,1,84,0,0,0,
13,6,7,0,12,7,0,6,105,110,100,101,110,116,0,0,
9,6,6,7,11,7,0,0,0,0,0,0,0,0,240,191,
9,6,6,7,25,3,1,6,21,3,0,0,18,0,0,62,
12,8,0,1,84,0,0,0,13,7,8,0,12,8,0,6,
105,110,100,101,110,116,0,0,9,7,7,8,12,8,0,5,
105,110,100,101,120,0,0,0,9,7,7,8,15,8,1,0,
31,6,8,1,19,6,7,6,15,3,6,0,11,8,0,0,
0,0,0,0,0,0,240,63,1,6,3,8,12,10,0,3,
108,101,110,0,13,9,10,0,12,11,0,1,84,0,0,0,
13,10,11,0,12,11,0,6,105,110,100,101,110,116,0,0,
9,10,10,11,31,8,10,1,19,8,9,8,25,6,6,8,
21,6,0,0,18,0,0,27,12,11,0,1,84,0,0,0,
13,10,11,0,12,11,0,6,105,110,100,101,110,116,0,0,
9,10,10,11,12,11,0,3,112,111,112,0,9,10,10,11,
31,8,0,0,19,8,10,8,15,1,8,0,12,12,0,1,
84,0,0,0,13,11,12,0,12,12,0,3,97,100,100,0,
9,11,11,12,12,12,0,6,100,101,100,101,110,116,0,0,
15,13,1,0,31,8,12,2,19,8,11,8,18,0,255,212,
18,0,0,1,0,0,0,0,12,16,0,6,105,110,100,101,
110,116,0,0,14,16,15,0,16,16,0,161,44,30,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
28,6,0,0,9,5,0,6,27,8,0,0,15,7,8,0,
9,9,1,3,15,8,9,0,15,10,3,0,11,13,0,0,
0,0,0,0,0,0,240,63,1,12,3,13,15,11,12,0,
15,13,8,0,15,8,10,0,15,3,11,0,12,11,0,7,
83,89,77,66,79,76,83,0,13,10,11,0,36,10,10,13,
21,10,0,0,18,0,0,9,12,14,0,6,97,112,112,101,
110,100,0,0,9,11,7,14,15,14,13,0,31,10,14,1,
19,10,11,10,18,0,0,1,25,14,3,5,21,14,0,0,
18,0,0,42,9,16,1,3,15,15,16,0,11,16,0,0,
0,0,0,0,0,0,0,0,12,18,0,8,73,83,89,77,
66,79,76,83,0,0,0,0,13,17,18,0,36,17,17,15,
23,16,16,17,21,16,0,0,18,0,0,3,18,0,0,27,
18,0,0,1,1,17,13,15,15,16,17,0,11,20,0,0,
0,0,0,0,0,0,240,63,1,19,3,20,15,18,19,0,
15,13,16,0,15,3,18,0,12,18,0,7,83,89,77,66,
79,76,83,0,13,16,18,0,36,16,16,13,21,16,0,0,
18,0,0,9,12,20,0,6,97,112,112,101,110,100,0,0,
9,18,7,20,15,20,13,0,31,16,20,1,19,16,18,16,
18,0,0,1,18,0,255,213,12,22,0,3,112,111,112,0,
9,21,7,22,31,20,0,0,19,20,21,20,15,13,20,0,
12,24,0,3,108,101,110,0,13,23,24,0,15,24,13,0,
31,22,24,1,19,22,23,22,15,20,22,0,1,22,8,20,
15,3,22,0,12,25,0,1,84,0,0,0,13,24,25,0,
12,25,0,3,97,100,100,0,9,24,24,25,12,25,0,6,
115,121,109,98,111,108,0,0,15,26,13,0,31,22,25,2,
19,22,24,22,12,25,0,7,66,95,66,69,71,73,78,0,
13,22,25,0,36,22,22,13,21,22,0,0,18,0,0,20,
12,25,0,1,84,0,0,0,13,22,25,0,12,26,0,1,
84,0,0,0,13,25,26,0,12,26,0,6,98,114,97,99,
101,115,0,0,9,25,25,26,11,26,0,0,0,0,0,0,
0,0,240,63,1,25,25,26,12,26,0,6,98,114,97,99,
101,115,0,0,10,22,26,25,18,0,0,1,12,28,0,5,
66,95,69,78,68,0,0,0,13,27,28,0,36,27,27,13,
21,27,0,0,18,0,0,20,12,28,0,1,84,0,0,0,
13,27,28,0,12,29,0,1,84,0,0,0,13,28,29,0,
12,29,0,6,98,114,97,99,101,115,0,0,9,28,28,29,
11,29,0,0,0,0,0,0,0,0,240,63,2,28,28,29,
12,29,0,6,98,114,97,99,101,115,0,0,10,27,29,28,
18,0,0,1,20,3,0,0,0,0,0,0,12,17,0,9,
100,111,95,115,121,109,98,111,108,0,0,0,14,17,16,0,
16,17,0,143,44,34,0,0,28,2,0,0,9,1,0,2,
28,4,0,0,9,3,0,4,28,6,0,0,9,5,0,6,
9,8,1,3,15,7,8,0,11,11,0,0,0,0,0,0,
0,0,240,63,1,10,3,11,15,9,10,0,9,12,1,3,
15,11,12,0,15,13,7,0,15,3,9,0,15,7,11,0,
25,9,3,5,21,9,0,0,18,0,0,60,9,11,1,3,
15,7,11,0,11,15,0,0,0,0,0,0,0,0,0,0,
11,17,0,0,0,0,0,0,0,0,0,0,11,19,0,0,
0,0,0,0,0,0,240,63,12,20,0,1,48,0,0,0,
25,11,7,20,23,18,11,19,21,18,0,0,18,0,0,2,
18,0,0,4,12,11,0,1,57,0,0,0,25,11,11,7,
23,16,11,17,21,16,0,0,18,0,0,2,18,0,0,14,
11,21,0,0,0,0,0,0,0,0,240,63,12,22,0,1,
97,0,0,0,25,11,7,22,23,20,11,21,21,20,0,0,
18,0,0,2,18,0,0,4,12,11,0,1,102,0,0,0,
25,11,11,7,23,14,11,15,21,14,0,0,18,0,0,2,
18,0,0,4,12,22,0,1,120,0,0,0,35,11,7,22,
21,11,0,0,18,0,0,3,18,0,0,12,18,0,0,1,
1,22,13,7,15,11,22,0,11,25,0,0,0,0,0,0,
0,0,240,63,1,24,3,25,15,23,24,0,15,13,11,0,
15,3,23,0,18,0,255,195,12,23,0,1,46,0,0,0,
23,11,7,23,21,11,0,0,18,0,0,43,1,23,13,7,
15,11,23,0,11,27,0,0,0,0,0,0,0,0,240,63,
1,26,3,27,15,25,26,0,15,13,11,0,15,3,25,0,
25,11,3,5,21,11,0,0,18,0,0,30,9,25,1,3,
15,7,25,0,11,28,0,0,0,0,0,0,0,0,240,63,
12,29,0,1,48,0,0,0,25,25,7,29,23,27,25,28,
21,27,0,0,18,0,0,2,18,0,0,4,12,25,0,1,
57,0,0,0,25,25,25,7,21,25,0,0,18,0,0,3,
18,0,0,12,18,0,0,1,1,29,13,7,15,25,29,0,
11,32,0,0,0,0,0,0,0,0,240,63,1,31,3,32,
15,30,31,0,15,13,25,0,15,3,30,0,18,0,255,225,
18,0,0,1,12,32,0,1,84,0,0,0,13,30,32,0,
12,32,0,3,97,100,100,0,9,30,30,32,12,32,0,6,
110,117,109,98,101,114,0,0,15,33,13,0,31,25,32,2,
19,25,30,25,20,3,0,0,0,0,0,0,12,18,0,9,
100,111,95,110,117,109,98,101,114,0,0,0,14,18,17,0,
16,18,0,134,44,32,0,0,28,2,0,0,9,1,0,2,
28,4,0,0,9,3,0,4,28,6,0,0,9,5,0,6,
9,8,1,3,15,7,8,0,11,11,0,0,0,0,0,0,
0,0,240,63,1,10,3,11,15,9,10,0,15,11,7,0,
15,3,9,0,25,7,3,5,21,7,0,0,18,0,0,80,
9,12,1,3,15,9,12,0,11,14,0,0,0,0,0,0,
0,0,0,0,11,16,0,0,0,0,0,0,0,0,0,0,
11,18,0,0,0,0,0,0,0,0,0,0,11,20,0,0,
0,0,0,0,0,0,240,63,12,21,0,1,97,0,0,0,
25,12,9,21,23,19,12,20,21,19,0,0,18,0,0,2,
18,0,0,4,12,12,0,1,122,0,0,0,25,12,12,9,
23,17,12,18,21,17,0,0,18,0,0,2,18,0,0,14,
11,22,0,0,0,0,0,0,0,0,240,63,12,23,0,1,
65,0,0,0,25,12,9,23,23,21,12,22,21,21,0,0,
18,0,0,2,18,0,0,4,12,12,0,1,90,0,0,0,
25,12,12,9,23,15,12,16,21,15,0,0,18,0,0,2,
18,0,0,14,11,24,0,0,0,0,0,0,0,0,240,63,
12,25,0,1,48,0,0,0,25,12,9,25,23,23,12,24,
21,23,0,0,18,0,0,2,18,0,0,4,12,12,0,1,
57,0,0,0,25,12,12,9,23,13,12,14,21,13,0,0,
18,0,0,2,18,0,0,4,12,25,0,1,95,0,0,0,
35,12,9,25,21,12,0,0,18,0,0,3,18,0,0,12,
18,0,0,1,1,25,11,9,15,12,25,0,11,28,0,0,
0,0,0,0,0,0,240,63,1,27,3,28,15,26,27,0,
15,11,12,0,15,3,26,0,18,0,255,175,12,26,0,7,
83,89,77,66,79,76,83,0,13,12,26,0,36,12,12,11,
21,12,0,0,18,0,0,14,12,28,0,1,84,0,0,0,
13,26,28,0,12,28,0,3,97,100,100,0,9,26,26,28,
12,28,0,6,115,121,109,98,111,108,0,0,15,29,11,0,
31,12,28,2,19,12,26,12,18,0,0,14,12,30,0,1,
84,0,0,0,13,29,30,0,12,30,0,3,97,100,100,0,
9,29,29,30,12,30,0,4,110,97,109,101,0,0,0,0,
15,31,11,0,31,28,30,2,19,28,29,28,18,0,0,1,
20,3,0,0,0,0,0,0,12,19,0,7,100,111,95,110,
97,109,101,0,14,19,18,0,16,19,1,3,44,42,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
28,6,0,0,9,5,0,6,12,8,0,0,0,0,0,0,
15,7,8,0,9,10,1,3,15,9,10,0,11,13,0,0,
0,0,0,0,0,0,240,63,1,12,3,13,15,11,12,0,
15,13,7,0,15,7,9,0,15,3,11,0,11,14,0,0,
0,0,0,0,0,0,0,0,11,16,0,0,0,0,0,0,
0,0,0,0,11,9,0,0,0,0,0,0,0,0,20,64,
2,17,5,3,24,9,9,17,23,15,9,16,21,15,0,0,
18,0,0,2,18,0,0,3,9,9,1,3,23,9,9,7,
23,11,9,14,21,11,0,0,18,0,0,2,18,0,0,7,
11,18,0,0,0,0,0,0,0,0,240,63,1,17,3,18,
9,9,1,17,23,9,9,7,21,9,0,0,18,0,0,105,
11,17,0,0,0,0,0,0,0,0,0,64,1,9,3,17,
15,3,9,0,11,18,0,0,0,0,0,0,0,0,0,64,
2,17,5,18,25,9,3,17,21,9,0,0,18,0,0,92,
9,18,1,3,15,17,18,0,11,20,0,0,0,0,0,0,
0,0,0,0,11,22,0,0,0,0,0,0,0,0,0,0,
23,18,17,7,23,21,18,22,21,21,0,0,18,0,0,2,
18,0,0,7,11,24,0,0,0,0,0,0,0,0,240,63,
1,23,3,24,9,18,1,23,23,18,18,7,23,19,18,20,
21,19,0,0,18,0,0,2,18,0,0,7,11,24,0,0,
0,0,0,0,0,0,0,64,1,23,3,24,9,18,1,23,
23,18,18,7,21,18,0,0,18,0,0,20,11,23,0,0,
0,0,0,0,0,0,8,64,1,18,3,23,15,3,18,0,
12,24,0,1,84,0,0,0,13,23,24,0,12,24,0,3,
97,100,100,0,9,23,23,24,12,24,0,6,115,116,114,105,
110,103,0,0,15,25,13,0,31,18,24,2,19,18,23,18,
18,0,0,43,18,0,0,41,1,24,13,17,15,18,24,0,
11,27,0,0,0,0,0,0,0,0,240,63,1,26,3,27,
15,25,26,0,15,13,18,0,15,3,25,0,12,25,0,1,
10,0,0,0,23,18,17,25,21,18,0,0,18,0,0,26,
12,27,0,1,84,0,0,0,13,25,27,0,12,27,0,1,
121,0,0,0,9,25,25,27,11,27,0,0,0,0,0,0,
0,0,240,63,1,25,25,27,15,18,25,0,15,27,3,0,
12,29,0,1,84,0,0,0,13,28,29,0,12,29,0,1,
121,0,0,0,10,28,29,18,12,31,0,1,84,0,0,0,
13,30,31,0,12,31,0,2,121,105,0,0,10,30,31,27,
18,0,0,1,18,0,0,1,18,0,255,159,18,0,0,104,
25,32,3,5,21,32,0,0,18,0,0,100,9,33,1,3,
15,17,33,0,12,34,0,1,92,0,0,0,23,33,17,34,
21,33,0,0,18,0,0,60,11,34,0,0,0,0,0,0,
0,0,240,63,1,33,3,34,15,3,33,0,9,33,1,3,
15,17,33,0,12,34,0,1,110,0,0,0,23,33,17,34,
21,33,0,0,18,0,0,5,12,33,0,1,10,0,0,0,
15,17,33,0,18,0,0,1,12,34,0,1,114,0,0,0,
23,33,17,34,21,33,0,0,18,0,0,11,12,35,0,3,
99,104,114,0,13,34,35,0,11,35,0,0,0,0,0,0,
0,0,42,64,31,33,35,1,19,33,34,33,15,17,33,0,
18,0,0,1,12,35,0,1,116,0,0,0,23,33,17,35,
21,33,0,0,18,0,0,5,12,33,0,1,9,0,0,0,
15,17,33,0,18,0,0,1,12,35,0,1,48,0,0,0,
23,33,17,35,21,33,0,0,18,0,0,5,12,33,0,1,
0,0,0,0,15,17,33,0,18,0,0,1,1,35,13,17,
15,33,35,0,11,38,0,0,0,0,0,0,0,0,240,63,
1,37,3,38,15,36,37,0,15,13,33,0,15,3,36,0,
18,0,0,33,23,33,17,7,21,33,0,0,18,0,0,20,
11,36,0,0,0,0,0,0,0,0,240,63,1,33,3,36,
15,3,33,0,12,38,0,1,84,0,0,0,13,36,38,0,
12,38,0,3,97,100,100,0,9,36,36,38,12,38,0,6,
115,116,114,105,110,103,0,0,15,39,13,0,31,33,38,2,
19,33,36,33,18,0,0,13,18,0,0,11,1,38,13,17,
15,33,38,0,11,41,0,0,0,0,0,0,0,0,240,63,
1,40,3,41,15,39,40,0,15,13,33,0,15,3,39,0,
18,0,0,1,18,0,255,155,18,0,0,1,20,3,0,0,
0,0,0,0,12,20,0,9,100,111,95,115,116,114,105,110,
103,0,0,0,14,20,19,0,16,20,0,33,44,11,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
28,6,0,0,9,5,0,6,11,8,0,0,0,0,0,0,
0,0,240,63,1,7,3,8,15,3,7,0,25,7,3,5,
21,7,0,0,18,0,0,16,9,9,1,3,15,8,9,0,
12,10,0,1,10,0,0,0,23,9,8,10,21,9,0,0,
18,0,0,3,18,0,0,8,18,0,0,1,11,10,0,0,
0,0,0,0,0,0,240,63,1,9,3,10,15,3,9,0,
18,0,255,239,20,3,0,0,0,0,0,0,12,21,0,10,
100,111,95,99,111,109,109,101,110,116,0,0,14,21,20,0,
0,0,0,0,
};
unsigned char tp_parse[] = {
44,116,0,0,12,2,0,6,105,109,112,111,114,116,0,0,
13,1,2,0,12,2,0,8,116,111,107,101,110,105,122,101,
0,0,0,0,31,0,2,1,19,0,1,0,12,2,0,8,
116,111,107,101,110,105,122,101,0,0,0,0,14,2,0,0,
16,0,0,38,44,18,0,0,28,2,0,0,9,1,0,2,
28,4,0,0,9,3,0,4,28,6,0,0,9,5,0,6,
28,7,0,0,28,8,0,0,32,7,0,8,12,11,0,4,
102,114,111,109,0,0,0,0,12,17,0,4,102,114,111,109,
0,0,0,0,9,12,1,17,12,13,0,4,116,121,112,101,
0,0,0,0,15,14,3,0,12,15,0,3,118,97,108,0,
15,16,5,0,26,10,11,6,15,9,10,0,28,11,0,0,
35,10,7,11,21,10,0,0,18,0,0,6,12,10,0,5,
105,116,101,109,115,0,0,0,10,9,10,7,18,0,0,1,
20,9,0,0,0,0,0,0,12,2,0,5,109,107,116,111,
107,0,0,0,14,2,0,0,16,2,0,63,44,10,0,0,
28,2,0,0,9,1,0,2,12,4,0,1,42,0,0,0,
9,3,0,4,11,6,0,0,0,0,0,0,0,0,0,0,
9,5,3,6,28,6,0,0,23,5,5,6,21,5,0,0,
18,0,0,6,11,5,0,0,0,0,0,0,0,0,240,63,
20,5,0,0,18,0,0,1,12,7,0,4,116,121,112,101,
0,0,0,0,9,6,1,7,36,5,3,6,21,5,0,0,
18,0,0,6,11,5,0,0,0,0,0,0,0,0,240,63,
20,5,0,0,18,0,0,1,11,7,0,0,0,0,0,0,
0,0,0,0,12,8,0,4,116,121,112,101,0,0,0,0,
9,5,1,8,12,8,0,6,115,121,109,98,111,108,0,0,
23,5,5,8,23,6,5,7,21,6,0,0,18,0,0,2,
18,0,0,5,12,9,0,3,118,97,108,0,9,8,1,9,
36,5,3,8,21,5,0,0,18,0,0,6,11,5,0,0,
0,0,0,0,0,0,240,63,20,5,0,0,18,0,0,1,
11,5,0,0,0,0,0,0,0,0,0,0,20,5,0,0,
0,0,0,0,12,3,0,5,99,104,101,99,107,0,0,0,
14,3,2,0,16,3,0,58,44,15,0,0,28,2,0,0,
9,1,0,2,28,4,0,0,9,3,0,4,12,7,0,1,
80,0,0,0,13,6,7,0,12,7,0,5,115,116,97,99,
107,0,0,0,9,6,6,7,12,7,0,6,97,112,112,101,
110,100,0,0,9,6,6,7,15,8,1,0,12,10,0,4,
100,109,97,112,0,0,0,0,13,9,10,0,9,9,9,1,
27,7,8,2,31,5,7,1,19,5,6,5,21,3,0,0,
18,0,0,12,12,7,0,4,100,109,97,112,0,0,0,0,
13,5,7,0,12,8,0,4,111,109,97,112,0,0,0,0,
13,7,8,0,9,7,7,1,10,5,1,7,18,0,0,19,
12,9,0,4,100,109,97,112,0,0,0,0,13,8,9,0,
12,10,0,3,108,98,112,0,11,11,0,0,0,0,0,0,
0,0,0,0,12,12,0,3,110,117,100,0,12,14,0,6,
105,116,115,101,108,102,0,0,13,13,14,0,26,9,10,4,
10,8,1,9,18,0,0,1,0,0,0,0,12,4,0,5,
116,119,101,97,107,0,0,0,14,4,3,0,16,4,0,30,
44,7,0,0,12,3,0,1,80,0,0,0,13,2,3,0,
12,3,0,5,115,116,97,99,107,0,0,0,9,2,2,3,
12,3,0,3,112,111,112,0,9,2,2,3,31,1,0,0,
19,1,2,1,11,5,0,0,0,0,0,0,0,0,0,0,
9,4,1,5,15,3,4,0,11,6,0,0,0,0,0,0,
0,0,240,63,9,5,1,6,15,4,5,0,12,5,0,4,
100,109,97,112,0,0,0,0,13,1,5,0,10,1,3,4,
0,0,0,0,12,5,0,7,114,101,115,116,111,114,101,0,
14,5,4,0,16,5,0,16,44,7,0,0,28,2,0,0,
9,1,0,2,26,4,0,0,15,3,4,0,11,5,0,0,
0,0,0,0,0,0,0,0,42,4,1,5,18,0,0,4,
9,6,1,4,10,3,4,6,18,0,255,252,20,3,0,0,
0,0,0,0,12,6,0,3,99,112,121,0,14,6,5,0,
26,6,0,0,12,7,0,5,80,68,97,116,97,0,0,0,
14,7,6,0,16,8,0,32,44,13,0,0,28,2,0,0,
9,1,0,2,28,4,0,0,9,3,0,4,28,6,0,0,
9,5,0,6,12,7,0,1,115,0,0,0,10,1,7,3,
12,8,0,6,116,111,107,101,110,115,0,0,10,1,8,5,
11,9,0,0,0,0,0,0,0,0,0,0,12,10,0,3,
112,111,115,0,10,1,10,9,28,9,0,0,12,11,0,5,
116,111,107,101,110,0,0,0,10,1,11,9,27,9,0,0,
12,12,0,5,115,116,97,99,107,0,0,0,10,1,12,9,
0,0,0,0,12,9,0,8,95,95,105,110,105,116,95,95,
0,0,0,0,10,6,9,8,16,9,0,39,44,9,0,0,
28,2,0,0,9,1,0,2,12,3,0,4,111,109,97,112,
0,0,0,0,12,6,0,3,99,112,121,0,13,5,6,0,
12,7,0,9,98,97,115,101,95,100,109,97,112,0,0,0,
13,6,7,0,31,4,6,1,19,4,5,4,14,3,4,0,
12,3,0,4,100,109,97,112,0,0,0,0,12,7,0,3,
99,112,121,0,13,6,7,0,12,8,0,9,98,97,115,101,
95,100,109,97,112,0,0,0,13,7,8,0,31,4,7,1,
19,4,6,4,14,3,4,0,12,7,0,7,97,100,118,97,
110,99,101,0,9,4,1,7,31,3,0,0,19,3,4,3,
0,0,0,0,12,10,0,4,105,110,105,116,0,0,0,0,
10,6,10,9,16,10,0,109,44,20,0,0,28,2,0,0,
9,1,0,2,28,3,0,0,28,4,0,0,32,3,0,4,
11,5,0,0,0,0,0,0,0,0,0,0,12,8,0,5,
99,104,101,99,107,0,0,0,13,7,8,0,12,10,0,5,
116,111,107,101,110,0,0,0,9,8,1,10,15,9,3,0,
31,6,8,2,19,6,7,6,23,5,5,6,21,5,0,0,
18,0,0,17,12,8,0,5,101,114,114,111,114,0,0,0,
13,6,8,0,12,8,0,9,101,120,112,101,99,116,101,100,
32,0,0,0,1,8,8,3,12,10,0,5,116,111,107,101,
110,0,0,0,9,9,1,10,31,5,8,2,19,5,6,5,
18,0,0,1,12,9,0,3,112,111,115,0,9,8,1,9,
12,11,0,3,108,101,110,0,13,10,11,0,12,12,0,6,
116,111,107,101,110,115,0,0,9,11,1,12,31,9,11,1,
19,9,10,9,25,8,8,9,21,8,0,0,18,0,0,21,
12,11,0,6,116,111,107,101,110,115,0,0,9,9,1,11,
12,12,0,3,112,111,115,0,9,11,1,12,9,9,9,11,
15,8,9,0,12,11,0,3,112,111,115,0,9,9,1,11,
11,11,0,0,0,0,0,0,0,0,240,63,1,9,9,11,
12,11,0,3,112,111,115,0,10,1,11,9,18,0,0,23,
12,12,0,4,102,114,111,109,0,0,0,0,11,18,0,0,
0,0,0,0,0,0,0,0,11,19,0,0,0,0,0,0,
0,0,0,0,27,13,18,2,12,14,0,4,116,121,112,101,
0,0,0,0,12,15,0,3,101,111,102,0,12,16,0,3,
118,97,108,0,12,17,0,3,101,111,102,0,26,9,12,6,
15,8,9,0,18,0,0,1,12,13,0,2,100,111,0,0,
13,12,13,0,15,13,8,0,31,9,13,1,19,9,12,9,
12,13,0,5,116,111,107,101,110,0,0,0,10,1,13,9,
20,8,0,0,0,0,0,0,12,11,0,7,97,100,118,97,
110,99,101,0,10,6,11,10,16,11,0,64,44,12,0,0,
28,2,0,0,9,1,0,2,12,5,0,4,98,105,110,100,
0,0,0,0,13,4,5,0,12,7,0,5,80,68,97,116,
97,0,0,0,13,5,7,0,12,7,0,8,95,95,105,110,
105,116,95,95,0,0,0,0,9,5,5,7,15,6,1,0,
31,3,5,2,19,3,4,3,12,5,0,8,95,95,105,110,
105,116,95,95,0,0,0,0,10,1,5,3,12,7,0,4,
98,105,110,100,0,0,0,0,13,6,7,0,12,9,0,5,
80,68,97,116,97,0,0,0,13,7,9,0,12,9,0,4,
105,110,105,116,0,0,0,0,9,7,7,9,15,8,1,0,
31,3,7,2,19,3,6,3,12,7,0,4,105,110,105,116,
0,0,0,0,10,1,7,3,12,9,0,4,98,105,110,100,
0,0,0,0,13,8,9,0,12,11,0,5,80,68,97,116,
97,0,0,0,13,9,11,0,12,11,0,7,97,100,118,97,
110,99,101,0,9,9,9,11,15,10,1,0,31,3,9,2,
19,3,8,3,12,9,0,7,97,100,118,97,110,99,101,0,
10,1,9,3,0,0,0,0,12,12,0,7,95,95,110,101,
119,95,95,0,10,6,12,11,16,12,0,22,44,7,0,0,
26,1,0,0,12,4,0,5,80,68,97,116,97,0,0,0,
13,3,4,0,12,4,0,7,95,95,110,101,119,95,95,0,
9,3,3,4,15,4,1,0,31,2,4,1,19,2,3,2,
12,5,0,8,95,95,105,110,105,116,95,95,0,0,0,0,
9,4,1,5,19,6,4,0,20,1,0,0,0,0,0,0,
12,13,0,8,95,95,99,97,108,108,95,95,0,0,0,0,
10,6,13,12,16,14,0,33,44,11,0,0,28,2,0,0,
9,1,0,2,28,4,0,0,9,3,0,4,12,6,0,5,
112,114,105,110,116,0,0,0,13,5,6,0,12,7,0,8,
116,111,107,101,110,105,122,101,0,0,0,0,13,6,7,0,
12,7,0,7,117,95,101,114,114,111,114,0,9,6,6,7,
15,7,1,0,12,10,0,1,80,0,0,0,13,8,10,0,
12,10,0,1,115,0,0,0,9,8,8,10,12,10,0,4,
102,114,111,109,0,0,0,0,9,9,3,10,31,5,7,3,
19,5,6,5,0,0,0,0,12,15,0,5,101,114,114,111,
114,0,0,0,14,15,14,0,16,15,0,32,44,8,0,0,
28,2,0,0,9,1,0,2,12,4,0,3,110,117,100,0,
36,3,1,4,11,4,0,0,0,0,0,0,0,0,0,0,
23,3,3,4,21,3,0,0,18,0,0,12,12,5,0,5,
101,114,114,111,114,0,0,0,13,4,5,0,12,5,0,6,
110,111,32,110,117,100,0,0,15,6,1,0,31,3,5,2,
19,3,4,3,18,0,0,1,12,7,0,3,110,117,100,0,
9,6,1,7,15,7,1,0,31,5,7,1,19,5,6,5,
20,5,0,0,0,0,0,0,12,16,0,3,110,117,100,0,
14,16,15,0,16,16,0,35,44,11,0,0,28,2,0,0,
9,1,0,2,28,4,0,0,9,3,0,4,12,6,0,3,
108,101,100,0,36,5,1,6,11,6,0,0,0,0,0,0,
0,0,0,0,23,5,5,6,21,5,0,0,18,0,0,12,
12,7,0,5,101,114,114,111,114,0,0,0,13,6,7,0,
12,7,0,6,110,111,32,108,101,100,0,0,15,8,1,0,
31,5,7,2,19,5,6,5,18,0,0,1,12,9,0,3,
108,101,100,0,9,8,1,9,15,9,1,0,15,10,3,0,
31,7,9,2,19,7,8,7,20,7,0,0,0,0,0,0,
12,17,0,3,108,101,100,0,14,17,16,0,16,17,0,29,
44,7,0,0,28,2,0,0,9,1,0,2,12,4,0,3,
108,98,112,0,36,3,1,4,11,4,0,0,0,0,0,0,
0,0,0,0,23,3,3,4,21,3,0,0,18,0,0,12,
12,5,0,5,101,114,114,111,114,0,0,0,13,4,5,0,
12,5,0,6,110,111,32,108,98,112,0,0,15,6,1,0,
31,3,5,2,19,3,4,3,18,0,0,1,12,6,0,3,
108,98,112,0,9,5,1,6,20,5,0,0,0,0,0,0,
12,18,0,7,103,101,116,95,108,98,112,0,14,18,17,0,
16,18,0,32,44,7,0,0,28,2,0,0,9,1,0,2,
12,4,0,5,105,116,101,109,115,0,0,0,36,3,1,4,
11,4,0,0,0,0,0,0,0,0,0,0,23,3,3,4,
21,3,0,0,18,0,0,13,12,5,0,5,101,114,114,111,
114,0,0,0,13,4,5,0,12,5,0,8,110,111,32,105,
116,101,109,115,0,0,0,0,15,6,1,0,31,3,5,2,
19,3,4,3,18,0,0,1,12,6,0,5,105,116,101,109,
115,0,0,0,9,5,1,6,20,5,0,0,0,0,0,0,
12,19,0,9,103,101,116,95,105,116,101,109,115,0,0,0,
14,19,18,0,16,19,0,66,44,14,0,0,28,2,0,0,
9,1,0,2,12,5,0,1,80,0,0,0,13,4,5,0,
12,5,0,5,116,111,107,101,110,0,0,0,9,4,4,5,
15,3,4,0,12,6,0,7,97,100,118,97,110,99,101,0,
13,5,6,0,31,4,0,0,19,4,5,4,12,8,0,3,
110,117,100,0,13,7,8,0,15,8,3,0,31,6,8,1,
19,6,7,6,15,4,6,0,12,10,0,7,103,101,116,95,
108,98,112,0,13,9,10,0,12,11,0,1,80,0,0,0,
13,10,11,0,12,11,0,5,116,111,107,101,110,0,0,0,
9,10,10,11,31,8,10,1,19,8,9,8,25,6,1,8,
21,6,0,0,18,0,0,24,12,10,0,1,80,0,0,0,
13,8,10,0,12,10,0,5,116,111,107,101,110,0,0,0,
9,8,8,10,15,3,8,0,12,11,0,7,97,100,118,97,
110,99,101,0,13,10,11,0,31,8,0,0,19,8,10,8,
12,12,0,3,108,101,100,0,13,11,12,0,15,12,3,0,
15,13,4,0,31,8,12,2,19,8,11,8,15,4,8,0,
18,0,255,218,20,4,0,0,0,0,0,0,12,20,0,10,
101,120,112,114,101,115,115,105,111,110,0,0,14,20,19,0,
16,20,0,24,44,11,0,0,28,2,0,0,9,1,0,2,
28,4,0,0,9,3,0,4,15,6,3,0,12,9,0,10,
101,120,112,114,101,115,115,105,111,110,0,0,13,8,9,0,
12,10,0,2,98,112,0,0,9,9,1,10,31,7,9,1,
19,7,8,7,27,5,6,2,12,6,0,5,105,116,101,109,
115,0,0,0,10,1,6,5,20,1,0,0,0,0,0,0,
12,21,0,9,105,110,102,105,120,95,108,101,100,0,0,0,
14,21,20,0,16,21,0,56,44,14,0,0,28,2,0,0,
9,1,0,2,28,4,0,0,9,3,0,4,12,7,0,5,
99,104,101,99,107,0,0,0,13,6,7,0,12,9,0,1,
80,0,0,0,13,7,9,0,12,9,0,5,116,111,107,101,
110,0,0,0,9,7,7,9,12,8,0,3,110,111,116,0,
31,5,7,2,19,5,6,5,21,5,0,0,18,0,0,16,
12,5,0,5,105,115,110,111,116,0,0,0,12,7,0,3,
118,97,108,0,10,1,7,5,12,9,0,7,97,100,118,97,
110,99,101,0,13,8,9,0,12,9,0,3,110,111,116,0,
31,5,9,1,19,5,8,5,18,0,0,1,15,9,3,0,
12,12,0,10,101,120,112,114,101,115,115,105,111,110,0,0,
13,11,12,0,12,13,0,2,98,112,0,0,9,12,1,13,
31,10,12,1,19,10,11,10,27,5,9,2,12,9,0,5,
105,116,101,109,115,0,0,0,10,1,9,5,20,1,0,0,
0,0,0,0,12,22,0,8,105,110,102,105,120,95,105,115,
0,0,0,0,14,22,21,0,16,22,0,38,44,13,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
12,7,0,7,97,100,118,97,110,99,101,0,13,6,7,0,
12,7,0,2,105,110,0,0,31,5,7,1,19,5,6,5,
12,5,0,5,110,111,116,105,110,0,0,0,12,7,0,3,
118,97,108,0,10,1,7,5,15,8,3,0,12,11,0,10,
101,120,112,114,101,115,115,105,111,110,0,0,13,10,11,0,
12,12,0,2,98,112,0,0,9,11,1,12,31,9,11,1,
19,9,10,9,27,5,8,2,12,8,0,5,105,116,101,109,
115,0,0,0,10,1,8,5,20,1,0,0,0,0,0,0,
12,23,0,9,105,110,102,105,120,95,110,111,116,0,0,0,
14,23,22,0,16,23,0,54,44,11,0,0,28,2,0,0,
9,1,0,2,28,4,0,0,9,3,0,4,12,8,0,10,
101,120,112,114,101,115,115,105,111,110,0,0,13,7,8,0,
12,9,0,2,98,112,0,0,9,8,1,9,31,6,8,1,
19,6,7,6,15,5,6,0,12,8,0,3,118,97,108,0,
9,6,3,8,12,8,0,1,44,0,0,0,23,6,6,8,
21,6,0,0,18,0,0,14,12,9,0,5,105,116,101,109,
115,0,0,0,9,8,3,9,12,9,0,6,97,112,112,101,
110,100,0,0,9,8,8,9,15,9,5,0,31,6,9,1,
19,6,8,6,20,3,0,0,18,0,0,1,15,9,3,0,
15,10,5,0,27,6,9,2,12,9,0,5,105,116,101,109,
115,0,0,0,10,1,9,6,12,6,0,5,116,117,112,108,
101,0,0,0,12,10,0,4,116,121,112,101,0,0,0,0,
10,1,10,6,20,1,0,0,0,0,0,0,12,24,0,11,
105,110,102,105,120,95,116,117,112,108,101,0,14,24,23,0,
16,24,0,43,44,9,0,0,28,2,0,0,9,1,0,2,
28,4,0,0,23,3,1,4,21,3,0,0,18,0,0,4,
27,3,0,0,20,3,0,0,18,0,0,1,12,5,0,5,
99,104,101,99,107,0,0,0,13,4,5,0,15,5,1,0,
12,6,0,1,44,0,0,0,12,7,0,5,116,117,112,108,
101,0,0,0,12,8,0,10,115,116,97,116,101,109,101,110,
116,115,0,0,31,3,5,4,19,3,4,3,21,3,0,0,
18,0,0,11,12,6,0,9,103,101,116,95,105,116,101,109,
115,0,0,0,13,5,6,0,15,6,1,0,31,3,6,1,
19,3,5,3,20,3,0,0,18,0,0,1,15,6,1,0,
27,3,6,1,20,3,0,0,0,0,0,0,12,25,0,3,
108,115,116,0,14,25,24,0,16,25,0,23,44,13,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
12,7,0,5,109,107,116,111,107,0,0,0,13,6,7,0,
15,7,3,0,15,8,1,0,15,9,1,0,12,12,0,3,
108,115,116,0,13,11,12,0,15,12,3,0,31,10,12,1,
19,10,11,10,31,5,7,4,19,5,6,5,20,5,0,0,
0,0,0,0,12,26,0,4,105,108,115,116,0,0,0,0,
14,26,25,0,16,26,0,114,44,18,0,0,28,2,0,0,
9,1,0,2,28,4,0,0,9,3,0,4,12,8,0,5,
109,107,116,111,107,0,0,0,13,7,8,0,15,8,1,0,
12,9,0,4,99,97,108,108,0,0,0,0,12,10,0,1,
36,0,0,0,15,12,3,0,27,11,12,1,31,6,8,4,
19,6,7,6,15,5,6,0,11,6,0,0,0,0,0,0,
0,0,0,0,12,10,0,5,99,104,101,99,107,0,0,0,
13,9,10,0,12,12,0,1,80,0,0,0,13,10,12,0,
12,12,0,5,116,111,107,101,110,0,0,0,9,10,10,12,
12,11,0,1,41,0,0,0,31,8,10,2,19,8,9,8,
23,6,6,8,21,6,0,0,18,0,0,63,12,11,0,5,
116,119,101,97,107,0,0,0,13,10,11,0,12,11,0,1,
44,0,0,0,11,12,0,0,0,0,0,0,0,0,0,0,
31,8,11,2,19,8,10,8,12,12,0,5,105,116,101,109,
115,0,0,0,9,11,5,12,12,12,0,6,97,112,112,101,
110,100,0,0,9,11,11,12,12,14,0,10,101,120,112,114,
101,115,115,105,111,110,0,0,13,13,14,0,11,14,0,0,
0,0,0,0,0,0,0,0,31,12,14,1,19,12,13,12,
31,8,12,1,19,8,11,8,12,12,0,1,80,0,0,0,
13,8,12,0,12,12,0,5,116,111,107,101,110,0,0,0,
9,8,8,12,12,12,0,3,118,97,108,0,9,8,8,12,
12,12,0,1,44,0,0,0,23,8,8,12,21,8,0,0,
18,0,0,10,12,14,0,7,97,100,118,97,110,99,101,0,
13,12,14,0,12,14,0,1,44,0,0,0,31,8,14,1,
19,8,12,8,18,0,0,1,12,16,0,7,114,101,115,116,
111,114,101,0,13,15,16,0,31,14,0,0,19,14,15,14,
18,0,255,174,12,17,0,7,97,100,118,97,110,99,101,0,
13,16,17,0,12,17,0,1,41,0,0,0,31,14,17,1,
19,14,16,14,20,5,0,0,0,0,0,0,12,27,0,8,
99,97,108,108,95,108,101,100,0,0,0,0,14,27,26,0,
16,27,0,219,44,35,0,0,28,2,0,0,9,1,0,2,
28,4,0,0,9,3,0,4,12,8,0,5,109,107,116,111,
107,0,0,0,13,7,8,0,15,8,1,0,12,9,0,3,
103,101,116,0,12,10,0,1,46,0,0,0,15,12,3,0,
27,11,12,1,31,6,8,4,19,6,7,6,15,5,6,0,
15,9,3,0,27,8,9,1,15,6,8,0,11,9,0,0,
0,0,0,0,0,0,0,0,15,8,9,0,11,9,0,0,
0,0,0,0,0,0,0,0,12,12,0,5,99,104,101,99,
107,0,0,0,13,11,12,0,12,14,0,1,80,0,0,0,
13,12,14,0,12,14,0,5,116,111,107,101,110,0,0,0,
9,12,12,14,12,13,0,1,93,0,0,0,31,10,12,2,
19,10,11,10,23,9,9,10,21,9,0,0,18,0,0,96,
11,10,0,0,0,0,0,0,0,0,0,0,15,8,10,0,
12,13,0,5,99,104,101,99,107,0,0,0,13,12,13,0,
12,15,0,1,80,0,0,0,13,13,15,0,12,15,0,5,
116,111,107,101,110,0,0,0,9,13,13,15,12,14,0,1,
58,0,0,0,31,10,13,2,19,10,12,10,21,10,0,0,
18,0,0,27,12,14,0,6,97,112,112,101,110,100,0,0,
9,13,6,14,12,16,0,5,109,107,116,111,107,0,0,0,
13,15,16,0,12,19,0,1,80,0,0,0,13,16,19,0,
12,19,0,5,116,111,107,101,110,0,0,0,9,16,16,19,
12,17,0,6,115,121,109,98,111,108,0,0,12,18,0,4,
78,111,110,101,0,0,0,0,31,14,16,3,19,14,15,14,
31,10,14,1,19,10,13,10,18,0,0,18,12,17,0,6,
97,112,112,101,110,100,0,0,9,16,6,17,12,19,0,10,
101,120,112,114,101,115,115,105,111,110,0,0,13,18,19,0,
11,19,0,0,0,0,0,0,0,0,0,0,31,17,19,1,
19,17,18,17,31,14,17,1,19,14,16,14,18,0,0,1,
12,20,0,5,99,104,101,99,107,0,0,0,13,19,20,0,
12,22,0,1,80,0,0,0,13,20,22,0,12,22,0,5,
116,111,107,101,110,0,0,0,9,20,20,22,12,21,0,1,
58,0,0,0,31,17,20,2,19,17,19,17,21,17,0,0,
18,0,0,14,12,21,0,7,97,100,118,97,110,99,101,0,
13,20,21,0,12,21,0,1,58,0,0,0,31,17,21,1,
19,17,20,17,11,17,0,0,0,0,0,0,0,0,240,63,
15,8,17,0,18,0,0,1,18,0,255,141,21,8,0,0,
18,0,0,27,12,22,0,6,97,112,112,101,110,100,0,0,
9,21,6,22,12,24,0,5,109,107,116,111,107,0,0,0,
13,23,24,0,12,27,0,1,80,0,0,0,13,24,27,0,
12,27,0,5,116,111,107,101,110,0,0,0,9,24,24,27,
12,25,0,6,115,121,109,98,111,108,0,0,12,26,0,4,
78,111,110,101,0,0,0,0,31,22,24,3,19,22,23,22,
31,17,22,1,19,17,21,17,18,0,0,1,11,22,0,0,
0,0,0,0,0,0,0,64,12,26,0,3,108,101,110,0,
13,25,26,0,15,26,6,0,31,24,26,1,19,24,25,24,
25,22,22,24,21,22,0,0,18,0,0,23,15,26,3,0,
12,28,0,5,109,107,116,111,107,0,0,0,13,24,28,0,
15,28,1,0,12,29,0,5,115,108,105,99,101,0,0,0,
12,30,0,1,58,0,0,0,11,33,0,0,0,0,0,0,
0,0,240,63,28,34,0,0,27,32,33,2,9,31,6,32,
31,27,28,4,19,27,24,27,27,22,26,2,15,6,22,0,
18,0,0,1,12,22,0,5,105,116,101,109,115,0,0,0,
10,5,22,6,12,28,0,7,97,100,118,97,110,99,101,0,
13,27,28,0,12,28,0,1,93,0,0,0,31,26,28,1,
19,26,27,26,20,5,0,0,0,0,0,0,12,28,0,7,
103,101,116,95,108,101,100,0,14,28,27,0,16,28,0,33,
44,11,0,0,28,2,0,0,9,1,0,2,28,4,0,0,
9,3,0,4,12,8,0,10,101,120,112,114,101,115,115,105,
111,110,0,0,13,7,8,0,12,9,0,2,98,112,0,0,
9,8,1,9,31,6,8,1,19,6,7,6,15,5,6,0,
12,6,0,6,115,116,114,105,110,103,0,0,12,8,0,4,
116,121,112,101,0,0,0,0,10,5,8,6,15,9,3,0,
15,10,5,0,27,6,9,2,12,9,0,5,105,116,101,109,
115,0,0,0,10,1,9,6,20,1,0,0,0,0,0,0,
12,29,0,7,100,111,116,95,108,101,100,0,14,29,28,0,
16,29,0,6,44,3,0,0,28,2,0,0,9,1,0,2,
20,1,0,0,0,0,0,0,12,30,0,6,105,116,115,101,
108,102,0,0,14,30,29,0,16,30,0,42,44,10,0,0,
28,2,0,0,9,1,0,2,12,5,0,5,116,119,101,97,
107,0,0,0,13,4,5,0,12,5,0,1,44,0,0,0,
11,6,0,0,0,0,0,0,0,0,240,63,31,3,5,2,
19,3,4,3,12,7,0,10,101,120,112,114,101,115,115,105,
111,110,0,0,13,6,7,0,11,7,0,0,0,0,0,0,
0,0,0,0,31,5,7,1,19,5,6,5,15,3,5,0,
12,8,0,7,114,101,115,116,111,114,101,0,13,7,8,0,
31,5,0,0,19,5,7,5,12,9,0,7,97,100,118,97,
110,99,101,0,13,8,9,0,12,9,0,1,41,0,0,0,
31,5,9,1,19,5,8,5,20,3,0,0,0,0,0,0,
12,31,0,9,112,97,114,101,110,95,110,117,100,0,0,0,
14,31,30,0,16,31,0,224,44,29,0,0,28,2,0,0,
9,1,0,2,12,3,0,4,108,105,115,116,0,0,0,0,
12,4,0,4,116,121,112,101,0,0,0,0,10,1,4,3,
12,3,0,2,91,93,0,0,12,5,0,3,118,97,108,0,
10,1,5,3,27,3,0,0,12,6,0,5,105,116,101,109,
115,0,0,0,10,1,6,3,12,8,0,1,80,0,0,0,
13,7,8,0,12,8,0,5,116,111,107,101,110,0,0,0,
9,7,7,8,15,3,7,0,12,9,0,5,116,119,101,97,
107,0,0,0,13,8,9,0,12,9,0,1,44,0,0,0,
11,10,0,0,0,0,0,0,0,0,0,0,31,7,9,2,
19,7,8,7,11,7,0,0,0,0,0,0,0,0,0,0,
12,11,0,5,99,104,101,99,107,0,0,0,13,10,11,0,
12,14,0,1,80,0,0,0,13,11,14,0,12,14,0,5,
116,111,107,101,110,0,0,0,9,11,11,14,12,12,0,3,
102,111,114,0,12,13,0,1,93,0,0,0,31,9,11,3,
19,9,10,9,23,7,7,9,21,7,0,0,18,0,0,48,
12,13,0,10,101,120,112,114,101,115,115,105,111,110,0,0,
13,12,13,0,11,13,0,0,0,0,0,0,0,0,0,0,
31,11,13,1,19,11,12,11,15,9,11,0,12,14,0,5,
105,116,101,109,115,0,0,0,9,13,1,14,12,14,0,6,
97,112,112,101,110,100,0,0,9,13,13,14,15,14,9,0,
31,11,14,1,19,11,13,11,12,14,0,1,80,0,0,0,
13,11,14,0,12,14,0,5,116,111,107,101,110,0,0,0,
9,11,11,14,12,14,0,3,118,97,108,0,9,11,11,14,
12,14,0,1,44,0,0,0,23,11,11,14,21,11,0,0,
18,0,0,10,12,15,0,7,97,100,118,97,110,99,101,0,
13,14,15,0,12,15,0,1,44,0,0,0,31,11,15,1,
19,11,14,11,18,0,0,1,18,0,255,187,12,17,0,5,
99,104,101,99,107,0,0,0,13,16,17,0,12,19,0,1,
80,0,0,0,13,17,19,0,12,19,0,5,116,111,107,101,
110,0,0,0,9,17,17,19,12,18,0,3,102,111,114,0,
31,15,17,2,19,15,16,15,21,15,0,0,18,0,0,82,
12,15,0,4,99,111,109,112,0,0,0,0,12,17,0,4,
116,121,112,101,0,0,0,0,10,1,17,15,12,19,0,7,
97,100,118,97,110,99,101,0,13,18,19,0,12,19,0,3,
102,111,114,0,31,15,19,1,19,15,18,15,12,20,0,5,
116,119,101,97,107,0,0,0,13,19,20,0,12,20,0,2,
105,110,0,0,11,21,0,0,0,0,0,0,0,0,0,0,
31,15,20,2,19,15,19,15,12,21,0,5,105,116,101,109,
115,0,0,0,9,20,1,21,12,21,0,6,97,112,112,101,
110,100,0,0,9,20,20,21,12,23,0,10,101,120,112,114,
101,115,115,105,111,110,0,0,13,22,23,0,11,23,0,0,
0,0,0,0,0,0,0,0,31,21,23,1,19,21,22,21,
31,15,21,1,19,15,20,15,12,23,0,7,97,100,118,97,
110,99,101,0,13,21,23,0,12,23,0,2,105,110,0,0,
31,15,23,1,19,15,21,15,12,24,0,5,105,116,101,109,
115,0,0,0,9,23,1,24,12,24,0,6,97,112,112,101,
110,100,0,0,9,23,23,24,12,26,0,10,101,120,112,114,
101,115,115,105,111,110,0,0,13,25,26,0,11,26,0,0,
0,0,0,0,0,0,0,0,31,24,26,1,19,24,25,24,
31,15,24,1,19,15,23,15,12,26,0,7,114,101,115,116,
111,114,101,0,13,24,26,0,31,15,0,0,19,15,24,15,
18,0,0,1,12,27,0,7,114,101,115,116,111,114,101,0,
13,26,27,0,31,15,0,0,19,15,26,15,12,28,0,7,
97,100,118,97,110,99,101,0,13,27,28,0,12,28,0,1,
93,0,0,0,31,15,28,1,19,15,27,15,20,1,0,0,
0,0,0,0,12,32,0,8,108,105,115,116,95,110,117,100,
0,0,0,0,14,32,31,0,16,32,0,116,44,18,0,0,
28,2,0,0,9,1,0,2,12,3,0,4,100,105,99,116,
0,0,0,0,12,4,0,4,116,121,112,101,0,0,0,0,
10,1,4,3,12,3,0,2,123,125,0,0,12,5,0,3,
118,97,108,0,10,1,5,3,27,3,0,0,12,6,0,5,
105,116,101,109,115,0,0,0,10,1,6,3,12,8,0,5,
116,119,101,97,107,0,0,0,13,7,8,0,12,8,0,1,
44,0,0,0,11,9,0,0,0,0,0,0,0,0,0,0,
31,3,8,2,19,3,7,3,11,3,0,0,0,0,0,0,
0,0,0,0,12,10,0,5,99,104,101,99,107,0,0,0,
13,9,10,0,12,12,0,1,80,0,0,0,13,10,12,0,
12,12,0,5,116,111,107,101,110,0,0,0,9,10,10,12,
12,11,0,1,125,0,0,0,31,8,10,2,19,8,9,8,
23,3,3,8,21,3,0,0,18,0,0,48,12,11,0,5,
105,116,101,109,115,0,0,0,9,10,1,11,12,11,0,6,
97,112,112,101,110,100,0,0,9,10,10,11,12,13,0,10,
101,120,112,114,101,115,115,105,111,110,0,0,13,12,13,0,
11,13,0,0,0,0,0,0,0,0,0,0,31,11,13,1,
19,11,12,11,31,8,11,1,19,8,10,8,12,13,0,5,
99,104,101,99,107,0,0,0,13,11,13,0,12,16,0,1,
80,0,0,0,13,13,16,0,12,16,0,5,116,111,107,101,
110,0,0,0,9,13,13,16,12,14,0,1,58,0,0,0,
12,15,0,1,44,0,0,0,31,8,13,3,19,8,11,8,
21,8,0,0,18,0,0,8,12,14,0,7,97,100,118,97,
110,99,101,0,13,13,14,0,31,8,0,0,19,8,13,8,
18,0,0,1,18,0,255,189,12,16,0,7,114,101,115,116,
111,114,101,0,13,15,16,0,31,14,0,0,19,14,15,14,
12,17,0,7,97,100,118,97,110,99,101,0,13,16,17,0,
12,17,0,1,125,0,0,0,31,14,17,1,19,14,16,14,
20,1,0,0,0,0,0,0,12,33,0,8,100,105,99,116,
95,110,117,100,0,0,0,0,14,33,32,0,16,33,0,17,
44,6,0,0,28,1,0,0,28,2,0,0,32,1,0,2,
12,5,0,1,80,0,0,0,13,4,5,0,12,5,0,7,
97,100,118,97,110,99,101,0,9,4,4,5,15,5,1,0,
31,3,5,1,19,3,4,3,20,3,0,0,0,0,0,0,
12,34,0,7,97,100,118,97,110,99,101,0,14,34,33,0,
16,34,1,0,44,36,0,0,27,2,0,0,15,1,2,0,
12,4,0,1,80,0,0,0,13,3,4,0,12,4,0,5,
116,111,107,101,110,0,0,0,9,3,3,4,15,2,3,0,
12,5,0,5,99,104,101,99,107,0,0,0,13,4,5,0,
12,7,0,1,80,0,0,0,13,5,7,0,12,7,0,5,
116,111,107,101,110,0,0,0,9,5,5,7,12,6,0,2,
110,108,0,0,31,3,5,2,19,3,4,3,21,3,0,0,
18,0,0,8,12,7,0,7,97,100,118,97,110,99,101,0,
13,6,7,0,31,5,0,0,19,5,6,5,18,0,255,233,
12,9,0,5,99,104,101,99,107,0,0,0,13,8,9,0,
12,11,0,1,80,0,0,0,13,9,11,0,12,11,0,5,
116,111,107,101,110,0,0,0,9,9,9,11,12,10,0,6,
105,110,100,101,110,116,0,0,31,7,9,2,19,7,8,7,
21,7,0,0,18,0,0,85,12,10,0,7,97,100,118,97,
110,99,101,0,13,9,10,0,12,10,0,6,105,110,100,101,
110,116,0,0,31,7,10,1,19,7,9,7,11,7,0,0,
0,0,0,0,0,0,0,0,12,12,0,5,99,104,101,99,
107,0,0,0,13,11,12,0,12,14,0,1,80,0,0,0,
13,12,14,0,12,14,0,5,116,111,107,101,110,0,0,0,
9,12,12,14,12,13,0,6,100,101,100,101,110,116,0,0,
31,10,12,2,19,10,11,10,23,7,7,10,21,7,0,0,
18,0,0,44,12,13,0,6,97,112,112,101,110,100,0,0,
9,12,1,13,12,15,0,10,101,120,112,114,101,115,115,105,
111,110,0,0,13,14,15,0,11,15,0,0,0,0,0,0,
0,0,0,0,31,13,15,1,19,13,14,13,31,10,13,1,
19,10,12,10,12,15,0,5,99,104,101,99,107,0,0,0,
13,13,15,0,12,18,0,1,80,0,0,0,13,15,18,0,
12,18,0,5,116,111,107,101,110,0,0,0,9,15,15,18,
12,16,0,1,59,0,0,0,12,17,0,2,110,108,0,0,
31,10,15,3,19,10,13,10,21,10,0,0,18,0,0,8,
12,17,0,7,97,100,118,97,110,99,101,0,13,16,17,0,
31,15,0,0,19,15,16,15,18,0,255,231,18,0,255,192,
12,19,0,7,97,100,118,97,110,99,101,0,13,18,19,0,
12,19,0,6,100,101,100,101,110,116,0,0,31,17,19,1,
19,17,18,17,18,0,0,60,12,20,0,6,97,112,112,101,
110,100,0,0,9,19,1,20,12,22,0,10,101,120,112,114,
101,115,115,105,111,110,0,0,13,21,22,0,11,22,0,0,
0,0,0,0,0,0,0,0,31,20,22,1,19,20,21,20,
31,17,20,1,19,17,19,17,12,22,0,5,99,104,101,99,
107,0,0,0,13,20,22,0,12,24,0,1,80,0,0,0,
13,22,24,0,12,24,0,5,116,111,107,101,110,0,0,0,
9,22,22,24,12,23,0,1,59,0,0,0,31,17,22,2,
19,17,20,17,21,17,0,0,18,0,0,26,12,24,0,7,
97,100,118,97,110,99,101,0,13,23,24,0,12,24,0,1,
59,0,0,0,31,22,24,1,19,22,23,22,12,25,0,6,
97,112,112,101,110,100,0,0,9,24,1,25,12,27,0,10,
101,120,112,114,101,115,115,105,111,110,0,0,13,26,27,0,
11,27,0,0,0,0,0,0,0,0,0,0,31,25,27,1,
19,25,26,25,31,22,25,1,19,22,24,22,18,0,255,215,
18,0,0,1,12,27,0,5,99,104,101,99,107,0,0,0,
13,25,27,0,12,29,0,1,80,0,0,0,13,27,29,0,
12,29,0,5,116,111,107,101,110,0,0,0,9,27,27,29,
12,28,0,2,110,108,0,0,31,22,27,2,19,22,25,22,
21,22,0,0,18,0,0,8,12,29,0,7,97,100,118,97,
110,99,101,0,13,28,29,0,31,27,0,0,19,27,28,27,
18,0,255,233,11,29,0,0,0,0,0,0,0,0,240,63,
12,32,0,3,108,101,110,0,13,31,32,0,15,32,1,0,
31,30,32,1,19,30,31,30,25,29,29,30,21,29,0,0,
18,0,0,17,12,32,0,5,109,107,116,111,107,0,0,0,
13,30,32,0,15,32,2,0,12,33,0,10,115,116,97,116,
101,109,101,110,116,115,0,0,12,34,0,1,59,0,0,0,
15,35,1,0,31,29,32,4,19,29,30,29,20,29,0,0,
18,0,0,1,12,33,0,3,112,111,112,0,9,32,1,33,
31,29,0,0,19,29,32,29,20,29,0,0,0,0,0,0,
12,35,0,5,98,108,111,99,107,0,0,0,14,35,34,0,
16,35,0,173,44,27,0,0,28,2,0,0,9,1,0,2,
27,4,0,0,12,5,0,5,105,116,101,109,115,0,0,0,
10,1,5,4,15,3,4,0,12,7,0,6,97,112,112,101,
110,100,0,0,9,6,3,7,12,8,0,1,80,0,0,0,
13,7,8,0,12,8,0,5,116,111,107,101,110,0,0,0,
9,7,7,8,31,4,7,1,19,4,6,4,12,8,0,7,
97,100,118,97,110,99,101,0,13,7,8,0,31,4,0,0,
19,4,7,4,12,9,0,7,97,100,118,97,110,99,101,0,
13,8,9,0,12,9,0,1,40,0,0,0,31,4,9,1,
19,4,8,4,12,11,0,5,109,107,116,111,107,0,0,0,
13,10,11,0,15,11,1,0,12,12,0,6,115,121,109,98,
111,108,0,0,12,13,0,3,40,41,58,0,27,14,0,0,
31,9,11,4,19,9,10,9,15,4,9,0,12,12,0,6,
97,112,112,101,110,100,0,0,9,11,3,12,15,12,4,0,
31,9,12,1,19,9,11,9,11,9,0,0,0,0,0,0,
0,0,0,0,12,14,0,5,99,104,101,99,107,0,0,0,
13,13,14,0,12,16,0,1,80,0,0,0,13,14,16,0,
12,16,0,5,116,111,107,101,110,0,0,0,9,14,14,16,
12,15,0,1,41,0,0,0,31,12,14,2,19,12,13,12,
23,9,9,12,21,9,0,0,18,0,0,65,12,15,0,5,
116,119,101,97,107,0,0,0,13,14,15,0,12,15,0,1,
44,0,0,0,11,16,0,0,0,0,0,0,0,0,0,0,
31,12,15,2,19,12,14,12,12,16,0,5,105,116,101,109,
115,0,0,0,9,15,4,16,12,16,0,6,97,112,112,101,
110,100,0,0,9,15,15,16,12,18,0,10,101,120,112,114,
101,115,115,105,111,110,0,0,13,17,18,0,11,18,0,0,
0,0,0,0,0,0,0,0,31,16,18,1,19,16,17,16,
31,12,16,1,19,12,15,12,12,18,0,5,99,104,101,99,
107,0,0,0,13,16,18,0,12,20,0,1,80,0,0,0,
13,18,20,0,12,20,0,5,116,111,107,101,110,0,0,0,
9,18,18,20,12,19,0,1,44,0,0,0,31,12,18,2,
19,12,16,12,21,12,0,0,18,0,0,10,12,19,0,7,
97,100,118,97,110,99,101,0,13,18,19,0,12,19,0,1,
44,0,0,0,31,12,19,1,19,12,18,12,18,0,0,1,
12,21,0,7,114,101,115,116,111,114,101,0,13,20,21,0,
31,19,0,0,19,19,20,19,18,0,255,172,12,22,0,7,
97,100,118,97,110,99,101,0,13,21,22,0,12,22,0,1,
41,0,0,0,31,19,22,1,19,19,21,19,12,23,0,7,
97,100,118,97,110,99,101,0,13,22,23,0,12,23,0,1,
58,0,0,0,31,19,23,1,19,19,22,19,12,24,0,6,
97,112,112,101,110,100,0,0,9,23,3,24,12,26,0,5,
98,108,111,99,107,0,0,0,13,25,26,0,31,24,0,0,
19,24,25,24,31,19,24,1,19,19,23,19,20,1,0,0,
0,0,0,0,12,36,0,7,100,101,102,95,110,117,100,0,
14,36,35,0,16,36,0,48,44,13,0,0,28,2,0,0,
9,1,0,2,27,4,0,0,12,5,0,5,105,116,101,109,
115,0,0,0,10,1,5,4,15,3,4,0,12,7,0,6,
97,112,112,101,110,100,0,0,9,6,3,7,12,9,0,10,
101,120,112,114,101,115,115,105,111,110,0,0,13,8,9,0,
11,9,0,0,0,0,0,0,0,0,0,0,31,7,9,1,
19,7,8,7,31,4,7,1,19,4,6,4,12,9,0,7,
97,100,118,97,110,99,101,0,13,7,9,0,12,9,0,1,
58,0,0,0,31,4,9,1,19,4,7,4,12,10,0,6,
97,112,112,101,110,100,0,0,9,9,3,10,12,12,0,5,
98,108,111,99,107,0,0,0,13,11,12,0,31,10,0,0,
19,10,11,10,31,4,10,1,19,4,9,4,20,1,0,0,
0,0,0,0,12,37,0,9,119,104,105,108,101,95,110,117,
100,0,0,0,14,37,36,0,16,37,0,57,44,16,0,0,
28,2,0,0,9,1,0,2,27,4,0,0,12,5,0,5,
105,116,101,109,115,0,0,0,10,1,5,4,15,3,4,0,
12,7,0,6,97,112,112,101,110,100,0,0,9,6,3,7,
12,9,0,10,101,120,112,114,101,115,115,105,111,110,0,0,
13,8,9,0,11,9,0,0,0,0,0,0,0,0,0,0,
31,7,9,1,19,7,8,7,31,4,7,1,19,4,6,4,
12,9,0,7,97,100,118,97,110,99,101,0,13,7,9,0,
12,9,0,1,58,0,0,0,31,4,9,1,19,4,7,4,
12,10,0,6,97,112,112,101,110,100,0,0,9,9,3,10,
12,12,0,4,105,108,115,116,0,0,0,0,13,11,12,0,
12,12,0,7,109,101,116,104,111,100,115,0,12,15,0,5,
98,108,111,99,107,0,0,0,13,14,15,0,31,13,0,0,
19,13,14,13,31,10,12,2,19,10,11,10,31,4,10,1,
19,4,9,4,20,1,0,0,0,0,0,0,12,38,0,9,
99,108,97,115,115,95,110,117,100,0,0,0,14,38,37,0,
16,38,0,89,44,18,0,0,28,2,0,0,9,1,0,2,
27,4,0,0,12,5,0,5,105,116,101,109,115,0,0,0,
10,1,5,4,15,3,4,0,12,7,0,5,116,119,101,97,
107,0,0,0,13,6,7,0,12,7,0,2,105,110,0,0,
11,8,0,0,0,0,0,0,0,0,0,0,31,4,7,2,
19,4,6,4,12,8,0,6,97,112,112,101,110,100,0,0,
9,7,3,8,12,10,0,10,101,120,112,114,101,115,115,105,
111,110,0,0,13,9,10,0,11,10,0,0,0,0,0,0,
0,0,0,0,31,8,10,1,19,8,9,8,31,4,8,1,
19,4,7,4,12,10,0,7,97,100,118,97,110,99,101,0,
13,8,10,0,12,10,0,2,105,110,0,0,31,4,10,1,
19,4,8,4,12,11,0,6,97,112,112,101,110,100,0,0,
9,10,3,11,12,13,0,10,101,120,112,114,101,115,115,105,
111,110,0,0,13,12,13,0,11,13,0,0,0,0,0,0,
0,0,0,0,31,11,13,1,19,11,12,11,31,4,11,1,
19,4,10,4,12,13,0,7,114,101,115,116,111,114,101,0,
13,11,13,0,31,4,0,0,19,4,11,4,12,14,0,7,
97,100,118,97,110,99,101,0,13,13,14,0,12,14,0,1,
58,0,0,0,31,4,14,1,19,4,13,4,12,15,0,6,
97,112,112,101,110,100,0,0,9,14,3,15,12,17,0,5,
98,108,111,99,107,0,0,0,13,16,17,0,31,15,0,0,
19,15,16,15,31,4,15,1,19,4,14,4,20,1,0,0,
0,0,0,0,12,39,0,7,102,111,114,95,110,117,100,0,
14,39,38,0,16,39,0,216,44,34,0,0,28,2,0,0,
9,1,0,2,27,4,0,0,12,5,0,5,105,116,101,109,
115,0,0,0,10,1,5,4,15,3,4,0,12,8,0,10,
101,120,112,114,101,115,115,105,111,110,0,0,13,7,8,0,
11,8,0,0,0,0,0,0,0,0,0,0,31,6,8,1,
19,6,7,6,15,4,6,0,12,9,0,7,97,100,118,97,
110,99,101,0,13,8,9,0,12,9,0,1,58,0,0,0,
31,6,9,1,19,6,8,6,12,11,0,5,98,108,111,99,
107,0,0,0,13,10,11,0,31,9,0,0,19,9,10,9,
15,6,9,0,12,12,0,6,97,112,112,101,110,100,0,0,
9,11,3,12,12,14,0,5,109,107,116,111,107,0,0,0,
13,13,14,0,15,14,1,0,12,15,0,4,101,108,105,102,
0,0,0,0,12,16,0,4,101,108,105,102,0,0,0,0,
15,18,4,0,15,19,6,0,27,17,18,2,31,12,14,4,
19,12,13,12,31,9,12,1,19,9,11,9,12,14,0,5,
99,104,101,99,107,0,0,0,13,12,14,0,12,16,0,1,
80,0,0,0,13,14,16,0,12,16,0,5,116,111,107,101,
110,0,0,0,9,14,14,16,12,15,0,4,101,108,105,102,
0,0,0,0,31,9,14,2,19,9,12,9,21,9,0,0,
18,0,0,67,12,16,0,1,80,0,0,0,13,15,16,0,
12,16,0,5,116,111,107,101,110,0,0,0,9,15,15,16,
15,14,15,0,12,17,0,7,97,100,118,97,110,99,101,0,
13,16,17,0,12,17,0,4,101,108,105,102,0,0,0,0,
31,15,17,1,19,15,16,15,12,18,0,10,101,120,112,114,
101,115,115,105,111,110,0,0,13,17,18,0,11,18,0,0,
0,0,0,0,0,0,0,0,31,15,18,1,19,15,17,15,
15,4,15,0,12,19,0,7,97,100,118,97,110,99,101,0,
13,18,19,0,12,19,0,1,58,0,0,0,31,15,19,1,
19,15,18,15,12,20,0,5,98,108,111,99,107,0,0,0,
13,19,20,0,31,15,0,0,19,15,19,15,15,6,15,0,
12,21,0,6,97,112,112,101,110,100,0,0,9,20,3,21,
12,23,0,5,109,107,116,111,107,0,0,0,13,22,23,0,
15,23,14,0,12,24,0,4,101,108,105,102,0,0,0,0,
12,25,0,4,101,108,105,102,0,0,0,0,15,27,4,0,
15,28,6,0,27,26,27,2,31,21,23,4,19,21,22,21,
31,15,21,1,19,15,20,15,18,0,255,173,12,23,0,5,
99,104,101,99,107,0,0,0,13,21,23,0,12,25,0,1,
80,0,0,0,13,23,25,0,12,25,0,5,116,111,107,101,
110,0,0,0,9,23,23,25,12,24,0,4,101,108,115,101,
0,0,0,0,31,15,23,2,19,15,21,15,21,15,0,0,
18,0,0,55,12,23,0,1,80,0,0,0,13,15,23,0,
12,23,0,5,116,111,107,101,110,0,0,0,9,15,15,23,
15,14,15,0,12,24,0,7,97,100,118,97,110,99,101,0,
13,23,24,0,12,24,0,4,101,108,115,101,0,0,0,0,
31,15,24,1,19,15,23,15,12,25,0,7,97,100,118,97,
110,99,101,0,13,24,25,0,12,25,0,1,58,0,0,0,
31,15,25,1,19,15,24,15,12,26,0,5,98,108,111,99,
107,0,0,0,13,25,26,0,31,15,0,0,19,15,25,15,
15,6,15,0,12,27,0,6,97,112,112,101,110,100,0,0,
9,26,3,27,12,29,0,5,109,107,116,111,107,0,0,0,
13,28,29,0,15,29,14,0,12,30,0,4,101,108,115,101,
0,0,0,0,12,31,0,4,101,108,115,101,0,0,0,0,
15,33,6,0,27,32,33,1,31,27,29,4,19,27,28,27,
31,15,27,1,19,15,26,15,18,0,0,1,20,1,0,0,
0,0,0,0,12,40,0,6,105,102,95,110,117,100,0,0,
14,40,39,0,16,40,0,227,44,34,0,0,28,2,0,0,
9,1,0,2,27,4,0,0,12,5,0,5,105,116,101,109,
115,0,0,0,10,1,5,4,15,3,4,0,12,7,0,7,
97,100,118,97,110,99,101,0,13,6,7,0,12,7,0,1,
58,0,0,0,31,4,7,1,19,4,6,4,12,9,0,5,
98,108,111,99,107,0,0,0,13,8,9,0,31,7,0,0,
19,7,8,7,15,4,7,0,12,10,0,6,97,112,112,101,
110,100,0,0,9,9,3,10,15,10,4,0,31,7,10,1,
19,7,9,7,12,11,0,5,99,104,101,99,107,0,0,0,
13,10,11,0,12,13,0,1,80,0,0,0,13,11,13,0,
12,13,0,5,116,111,107,101,110,0,0,0,9,11,11,13,
12,12,0,6,101,120,99,101,112,116,0,0,31,7,11,2,
19,7,10,7,21,7,0,0,18,0,0,104,12,13,0,1,
80,0,0,0,13,12,13,0,12,13,0,5,116,111,107,101,
110,0,0,0,9,12,12,13,15,11,12,0,12,14,0,7,
97,100,118,97,110,99,101,0,13,13,14,0,12,14,0,6,
101,120,99,101,112,116,0,0,31,12,14,1,19,12,13,12,
11,12,0,0,0,0,0,0,0,0,0,0,12,16,0,5,
99,104,101,99,107,0,0,0,13,15,16,0,12,18,0,1,
80,0,0,0,13,16,18,0,12,18,0,5,116,111,107,101,
110,0,0,0,9,16,16,18,12,17,0,1,58,0,0,0,
31,14,16,2,19,14,15,14,23,12,12,14,21,12,0,0,
18,0,0,13,12,17,0,10,101,120,112,114,101,115,115,105,
111,110,0,0,13,16,17,0,11,17,0,0,0,0,0,0,
0,0,0,0,31,14,17,1,19,14,16,14,15,12,14,0,
18,0,0,16,12,18,0,5,109,107,116,111,107,0,0,0,
13,17,18,0,15,18,11,0,12,19,0,6,115,121,109,98,
111,108,0,0,12,20,0,4,78,111,110,101,0,0,0,0,
31,14,18,3,19,14,17,14,15,12,14,0,18,0,0,1,
12,19,0,7,97,100,118,97,110,99,101,0,13,18,19,0,
12,19,0,1,58,0,0,0,31,14,19,1,19,14,18,14,
12,20,0,5,98,108,111,99,107,0,0,0,13,19,20,0,
31,14,0,0,19,14,19,14,15,4,14,0,12,21,0,6,
97,112,112,101,110,100,0,0,9,20,3,21,12,23,0,5,
109,107,116,111,107,0,0,0,13,22,23,0,15,23,11,0,
12,24,0,6,101,120,99,101,112,116,0,0,12,25,0,6,
101,120,99,101,112,116,0,0,15,27,12,0,15,28,4,0,
27,26,27,2,31,21,23,4,19,21,22,21,31,14,21,1,
19,14,20,14,18,0,255,136,12,23,0,5,99,104,101,99,
107,0,0,0,13,21,23,0,12,25,0,1,80,0,0,0,
13,23,25,0,12,25,0,5,116,111,107,101,110,0,0,0,
9,23,23,25,12,24,0,4,101,108,115,101,0,0,0,0,
31,14,23,2,19,14,21,14,21,14,0,0,18,0,0,55,
12,23,0,1,80,0,0,0,13,14,23,0,12,23,0,5,
116,111,107,101,110,0,0,0,9,14,14,23,15,11,14,0,
12,24,0,7,97,100,118,97,110,99,101,0,13,23,24,0,
12,24,0,4,101,108,115,101,0,0,0,0,31,14,24,1,
19,14,23,14,12,25,0,7,97,100,118,97,110,99,101,0,
13,24,25,0,12,25,0,1,58,0,0,0,31,14,25,1,
19,14,24,14,12,26,0,5,98,108,111,99,107,0,0,0,
13,25,26,0,31,14,0,0,19,14,25,14,15,4,14,0,
12,27,0,6,97,112,112,101,110,100,0,0,9,26,3,27,
12,29,0,5,109,107,116,111,107,0,0,0,13,28,29,0,
15,29,11,0,12,30,0,4,101,108,115,101,0,0,0,0,
12,31,0,4,101,108,115,101,0,0,0,0,15,33,4,0,
27,32,33,1,31,27,29,4,19,27,28,27,31,14,27,1,
19,14,26,14,18,0,0,1,20,1,0,0,0,0,0,0,
12,41,0,7,116,114,121,95,110,117,100,0,14,41,40,0,
16,41,0,33,44,8,0,0,28,2,0,0,9,1,0,2,
11,4,0,0,0,0,0,0,0,128,81,64,15,3,4,0,
12,5,0,2,98,112,0,0,36,4,1,5,21,4,0,0,
18,0,0,6,12,5,0,2,98,112,0,0,9,4,1,5,
15,3,4,0,18,0,0,1,12,7,0,10,101,120,112,114,
101,115,115,105,111,110,0,0,13,6,7,0,15,7,3,0,
31,5,7,1,19,5,6,5,27,4,5,1,12,5,0,5,
105,116,101,109,115,0,0,0,10,1,5,4,20,1,0,0,
0,0,0,0,12,42,0,10,112,114,101,102,105,120,95,110,
117,100,0,0,14,42,41,0,16,42,0,40,44,11,0,0,
28,2,0,0,9,1,0,2,12,5,0,5,99,104,101,99,
107,0,0,0,13,4,5,0,12,10,0,1,80,0,0,0,
13,5,10,0,12,10,0,5,116,111,107,101,110,0,0,0,
9,5,5,10,12,6,0,2,110,108,0,0,12,7,0,1,
59,0,0,0,12,8,0,3,101,111,102,0,12,9,0,6,
100,101,100,101,110,116,0,0,31,3,5,5,19,3,4,3,
21,3,0,0,18,0,0,3,20,1,0,0,18,0,0,1,
12,6,0,10,112,114,101,102,105,120,95,110,117,100,0,0,
13,5,6,0,15,6,1,0,31,3,6,1,19,3,5,3,
20,3,0,0,0,0,0,0,12,43,0,11,112,114,101,102,
105,120,95,110,117,100,48,0,14,43,42,0,16,43,0,28,
44,10,0,0,28,2,0,0,9,1,0,2,12,6,0,10,
101,120,112,114,101,115,115,105,111,110,0,0,13,5,6,0,
11,6,0,0,0,0,0,0,0,0,0,0,31,4,6,1,
19,4,5,4,15,3,4,0,12,7,0,4,105,108,115,116,
0,0,0,0,13,6,7,0,12,9,0,4,116,121,112,101,
0,0,0,0,9,7,1,9,15,8,3,0,31,4,7,2,
19,4,6,4,20,4,0,0,0,0,0,0,12,44,0,11,
112,114,101,102,105,120,95,110,117,100,115,0,14,44,43,0,
16,44,0,68,44,15,0,0,28,2,0,0,9,1,0,2,
12,6,0,10,101,120,112,114,101,115,115,105,111,110,0,0,
13,5,6,0,11,6,0,0,0,0,0,0,0,0,73,64,
31,4,6,1,19,4,5,4,15,3,4,0,12,6,0,4,
116,121,112,101,0,0,0,0,9,4,3,6,12,6,0,6,
110,117,109,98,101,114,0,0,23,4,4,6,21,4,0,0,
18,0,0,24,12,7,0,3,115,116,114,0,13,6,7,0,
11,7,0,0,0,0,0,0,0,0,0,0,12,10,0,5,
102,108,111,97,116,0,0,0,13,9,10,0,12,11,0,3,
118,97,108,0,9,10,3,11,31,8,10,1,19,8,9,8,
2,7,7,8,31,4,7,1,19,4,6,4,12,7,0,3,
118,97,108,0,10,3,7,4,20,3,0,0,18,0,0,1,
12,12,0,5,109,107,116,111,107,0,0,0,13,8,12,0,
15,12,1,0,12,13,0,6,110,117,109,98,101,114,0,0,
12,14,0,1,48,0,0,0,31,10,12,3,19,10,8,10,
15,11,3,0,27,4,10,2,12,10,0,5,105,116,101,109,
115,0,0,0,10,1,10,4,20,1,0,0,0,0,0,0,
12,45,0,10,112,114,101,102,105,120,95,110,101,103,0,0,
14,45,44,0,16,45,0,27,44,8,0,0,28,2,0,0,
9,1,0,2,12,6,0,10,112,114,101,102,105,120,95,110,
117,100,0,0,13,5,6,0,15,6,1,0,31,4,6,1,
19,4,5,4,15,3,4,0,12,4,0,4,97,114,103,115,
0,0,0,0,12,6,0,4,116,121,112,101,0,0,0,0,
10,1,6,4,12,4,0,1,42,0,0,0,12,7,0,3,
118,97,108,0,10,1,7,4,20,1,0,0,0,0,0,0,
12,46,0,9,118,97,114,103,115,95,110,117,100,0,0,0,
14,46,45,0,16,46,0,27,44,8,0,0,28,2,0,0,
9,1,0,2,12,6,0,10,112,114,101,102,105,120,95,110,
117,100,0,0,13,5,6,0,15,6,1,0,31,4,6,1,
19,4,5,4,15,3,4,0,12,4,0,5,110,97,114,103,
115,0,0,0,12,6,0,4,116,121,112,101,0,0,0,0,
10,1,6,4,12,4,0,2,42,42,0,0,12,7,0,3,
118,97,108,0,10,1,7,4,20,1,0,0,0,0,0,0,
12,47,0,9,110,97,114,103,115,95,110,117,100,0,0,0,
14,47,46,0,12,47,0,9,98,97,115,101,95,100,109,97,
112,0,0,0,12,49,0,1,44,0,0,0,12,105,0,3,
108,98,112,0,11,106,0,0,0,0,0,0,0,0,52,64,
12,107,0,2,98,112,0,0,11,108,0,0,0,0,0,0,
0,0,52,64,12,109,0,3,108,101,100,0,12,111,0,11,
105,110,102,105,120,95,116,117,112,108,101,0,13,110,111,0,
26,50,105,6,12,51,0,1,43,0,0,0,12,105,0,3,
108,98,112,0,11,106,0,0,0,0,0,0,0,0,73,64,
12,107,0,2,98,112,0,0,11,108,0,0,0,0,0,0,
0,0,73,64,12,109,0,3,108,101,100,0,12,111,0,9,
105,110,102,105,120,95,108,101,100,0,0,0,13,110,111,0,
26,52,105,6,12,53,0,1,45,0,0,0,12,105,0,3,
108,98,112,0,11,106,0,0,0,0,0,0,0,0,73,64,
12,107,0,3,110,117,100,0,12,113,0,10,112,114,101,102,
105,120,95,110,101,103,0,0,13,108,113,0,12,109,0,2,
98,112,0,0,11,110,0,0,0,0,0,0,0,0,73,64,
12,111,0,3,108,101,100,0,12,113,0,9,105,110,102,105,
120,95,108,101,100,0,0,0,13,112,113,0,26,54,105,8,
12,55,0,3,110,111,116,0,12,105,0,3,108,98,112,0,
11,106,0,0,0,0,0,0,0,128,65,64,12,107,0,3,
110,117,100,0,12,115,0,10,112,114,101,102,105,120,95,110,
117,100,0,0,13,108,115,0,12,109,0,2,98,112,0,0,
11,110,0,0,0,0,0,0,0,128,65,64,12,111,0,2,
98,112,0,0,11,112,0,0,0,0,0,0,0,128,65,64,
12,113,0,3,108,101,100,0,12,115,0,9,105,110,102,105,
120,95,110,111,116,0,0,0,13,114,115,0,26,56,105,10,
12,57,0,1,37,0,0,0,12,105,0,3,108,98,112,0,
11,106,0,0,0,0,0,0,0,0,78,64,12,107,0,2,
98,112,0,0,11,108,0,0,0,0,0,0,0,0,78,64,
12,109,0,3,108,101,100,0,12,111,0,9,105,110,102,105,
120,95,108,101,100,0,0,0,13,110,111,0,26,58,105,6,
12,59,0,1,42,0,0,0,12,105,0,3,108,98,112,0,
11,106,0,0,0,0,0,0,0,0,78,64,12,107,0,3,
110,117,100,0,12,113,0,9,118,97,114,103,115,95,110,117,
100,0,0,0,13,108,113,0,12,109,0,2,98,112,0,0,
11,110,0,0,0,0,0,0,0,0,78,64,12,111,0,3,
108,101,100,0,12,113,0,9,105,110,102,105,120,95,108,101,
100,0,0,0,13,112,113,0,26,60,105,8,12,61,0,2,
42,42,0,0,12,105,0,3,108,98,112,0,11,106,0,0,
0,0,0,0,0,0,78,64,12,107,0,3,110,117,100,0,
12,113,0,9,110,97,114,103,115,95,110,117,100,0,0,0,
13,108,113,0,12,109,0,2,98,112,0,0,11,110,0,0,
0,0,0,0,0,64,80,64,12,111,0,3,108,101,100,0,
12,113,0,9,105,110,102,105,120,95,108,101,100,0,0,0,
13,112,113,0,26,62,105,8,12,63,0,1,47,0,0,0,
12,105,0,3,108,98,112,0,11,106,0,0,0,0,0,0,
0,0,78,64,12,107,0,2,98,112,0,0,11,108,0,0,
0,0,0,0,0,0,78,64,12,109,0,3,108,101,100,0,
12,111,0,9,105,110,102,105,120,95,108,101,100,0,0,0,
13,110,111,0,26,64,105,6,12,65,0,1,40,0,0,0,
12,105,0,3,108,98,112,0,11,106,0,0,0,0,0,0,
0,128,81,64,12,107,0,3,110,117,100,0,12,113,0,9,
112,97,114,101,110,95,110,117,100,0,0,0,13,108,113,0,
12,109,0,2,98,112,0,0,11,110,0,0,0,0,0,0,
0,0,84,64,12,111,0,3,108,101,100,0,12,113,0,8,
99,97,108,108,95,108,101,100,0,0,0,0,13,112,113,0,
26,66,105,8,12,67,0,1,91,0,0,0,12,105,0,3,
108,98,112,0,11,106,0,0,0,0,0,0,0,128,81,64,
12,107,0,3,110,117,100,0,12,113,0,8,108,105,115,116,
95,110,117,100,0,0,0,0,13,108,113,0,12,109,0,2,
98,112,0,0,11,110,0,0,0,0,0,0,0,0,84,64,
12,111,0,3,108,101,100,0,12,113,0,7,103,101,116,95,
108,101,100,0,13,112,113,0,26,68,105,8,12,69,0,1,
123,0,0,0,12,105,0,3,108,98,112,0,11,106,0,0,
0,0,0,0,0,0,0,0,12,107,0,3,110,117,100,0,
12,109,0,8,100,105,99,116,95,110,117,100,0,0,0,0,
13,108,109,0,26,70,105,4,12,71,0,1,46,0,0,0,
12,105,0,3,108,98,112,0,11,106,0,0,0,0,0,0,
0,0,84,64,12,107,0,2,98,112,0,0,11,108,0,0,
0,0,0,0,0,0,84,64,12,109,0,3,108,101,100,0,
12,113,0,7,100,111,116,95,108,101,100,0,13,110,113,0,
12,111,0,4,116,121,112,101,0,0,0,0,12,112,0,3,
103,101,116,0,26,72,105,8,12,73,0,5,98,114,101,97,
107,0,0,0,12,105,0,3,108,98,112,0,11,106,0,0,
0,0,0,0,0,0,0,0,12,107,0,3,110,117,100,0,
12,111,0,6,105,116,115,101,108,102,0,0,13,108,111,0,
12,109,0,4,116,121,112,101,0,0,0,0,12,110,0,5,
98,114,101,97,107,0,0,0,26,74,105,6,12,75,0,4,
112,97,115,115,0,0,0,0,12,105,0,3,108,98,112,0,
11,106,0,0,0,0,0,0,0,0,0,0,12,107,0,3,
110,117,100,0,12,111,0,6,105,116,115,101,108,102,0,0,
13,108,111,0,12,109,0,4,116,121,112,101,0,0,0,0,
12,110,0,4,112,97,115,115,0,0,0,0,26,76,105,6,
12,77,0,8,99,111,110,116,105,110,117,101,0,0,0,0,
12,105,0,3,108,98,112,0,11,106,0,0,0,0,0,0,
0,0,0,0,12,107,0,3,110,117,100,0,12,111,0,6,
105,116,115,101,108,102,0,0,13,108,111,0,12,109,0,4,
116,121,112,101,0,0,0,0,12,110,0,8,99,111,110,116,
105,110,117,101,0,0,0,0,26,78,105,6,12,79,0,3,
101,111,102,0,12,105,0,3,108,98,112,0,11,106,0,0,
0,0,0,0,0,0,0,0,12,107,0,4,116,121,112,101,
0,0,0,0,12,108,0,3,101,111,102,0,12,109,0,3,
118,97,108,0,12,110,0,3,101,111,102,0,26,80,105,6,
12,81,0,3,100,101,102,0,12,105,0,3,108,98,112,0,
11,106,0,0,0,0,0,0,0,0,0,0,12,107,0,3,
110,117,100,0,12,111,0,7,100,101,102,95,110,117,100,0,
13,108,111,0,12,109,0,4,116,121,112,101,0,0,0,0,
12,110,0,3,100,101,102,0,26,82,105,6,12,83,0,5,
119,104,105,108,101,0,0,0,12,105,0,3,108,98,112,0,
11,106,0,0,0,0,0,0,0,0,0,0,12,107,0,3,
110,117,100,0,12,111,0,9,119,104,105,108,101,95,110,117,
100,0,0,0,13,108,111,0,12,109,0,4,116,121,112,101,
0,0,0,0,12,110,0,5,119,104,105,108,101,0,0,0,
26,84,105,6,12,85,0,3,102,111,114,0,12,105,0,3,
108,98,112,0,11,106,0,0,0,0,0,0,0,0,0,0,
12,107,0,3,110,117,100,0,12,111,0,7,102,111,114,95,
110,117,100,0,13,108,111,0,12,109,0,4,116,121,112,101,
0,0,0,0,12,110,0,3,102,111,114,0,26,86,105,6,
12,87,0,3,116,114,121,0,12,105,0,3,108,98,112,0,
11,106,0,0,0,0,0,0,0,0,0,0,12,107,0,3,
110,117,100,0,12,111,0,7,116,114,121,95,110,117,100,0,
13,108,111,0,12,109,0,4,116,121,112,101,0,0,0,0,
12,110,0,3,116,114,121,0,26,88,105,6,12,89,0,2,
105,102,0,0,12,105,0,3,108,98,112,0,11,106,0,0,
0,0,0,0,0,0,0,0,12,107,0,3,110,117,100,0,
12,111,0,6,105,102,95,110,117,100,0,0,13,108,111,0,
12,109,0,4,116,121,112,101,0,0,0,0,12,110,0,2,
105,102,0,0,26,90,105,6,12,91,0,5,99,108,97,115,
115,0,0,0,12,105,0,3,108,98,112,0,11,106,0,0,
0,0,0,0,0,0,0,0,12,107,0,3,110,117,100,0,
12,111,0,9,99,108,97,115,115,95,110,117,100,0,0,0,
13,108,111,0,12,109,0,4,116,121,112,101,0,0,0,0,
12,110,0,5,99,108,97,115,115,0,0,0,26,92,105,6,
12,93,0,5,114,97,105,115,101,0,0,0,12,105,0,3,
108,98,112,0,11,106,0,0,0,0,0,0,0,0,0,0,
12,107,0,3,110,117,100,0,12,113,0,11,112,114,101,102,
105,120,95,110,117,100,48,0,13,108,113,0,12,109,0,4,
116,121,112,101,0,0,0,0,12,110,0,5,114,97,105,115,
101,0,0,0,12,111,0,2,98,112,0,0,11,112,0,0,
0,0,0,0,0,0,52,64,26,94,105,8,12,95,0,6,
114,101,116,117,114,110,0,0,12,105,0,3,108,98,112,0,
11,106,0,0,0,0,0,0,0,0,0,0,12,107,0,3,
110,117,100,0,12,113,0,11,112,114,101,102,105,120,95,110,
117,100,48,0,13,108,113,0,12,109,0,4,116,121,112,101,
0,0,0,0,12,110,0,6,114,101,116,117,114,110,0,0,
12,111,0,2,98,112,0,0,11,112,0,0,0,0,0,0,
0,0,36,64,26,96,105,8,12,97,0,6,105,109,112,111,
114,116,0,0,12,105,0,3,108,98,112,0,11,106,0,0,
0,0,0,0,0,0,0,0,12,107,0,3,110,117,100,0,
12,113,0,11,112,114,101,102,105,120,95,110,117,100,115,0,
13,108,113,0,12,109,0,4,116,121,112,101,0,0,0,0,
12,110,0,6,105,109,112,111,114,116,0,0,12,111,0,2,
98,112,0,0,11,112,0,0,0,0,0,0,0,0,52,64,
26,98,105,8,12,99,0,3,100,101,108,0,12,105,0,3,
108,98,112,0,11,106,0,0,0,0,0,0,0,0,0,0,
12,107,0,3,110,117,100,0,12,113,0,11,112,114,101,102,
105,120,95,110,117,100,115,0,13,108,113,0,12,109,0,4,
116,121,112,101,0,0,0,0,12,110,0,3,100,101,108,0,
12,111,0,2,98,112,0,0,11,112,0,0,0,0,0,0,
0,0,36,64,26,100,105,8,12,101,0,6,103,108,111,98,
97,108,0,0,12,105,0,3,108,98,112,0,11,106,0,0,
0,0,0,0,0,0,0,0,12,107,0,3,110,117,100,0,
12,113,0,11,112,114,101,102,105,120,95,110,117,100,115,0,
13,108,113,0,12,109,0,4,116,121,112,101,0,0,0,0,
12,110,0,7,103,108,111,98,97,108,115,0,12,111,0,2,
98,112,0,0,11,112,0,0,0,0,0,0,0,0,52,64,
26,102,105,8,12,103,0,1,61,0,0,0,12,105,0,3,
108,98,112,0,11,106,0,0,0,0,0,0,0,0,36,64,
12,107,0,2,98,112,0,0,11,108,0,0,0,0,0,0,
0,0,34,64,12,109,0,3,108,101,100,0,12,111,0,9,
105,110,102,105,120,95,108,101,100,0,0,0,13,110,111,0,
26,104,105,6,26,48,49,56,14,47,48,0,16,47,0,32,
44,17,0,0,28,2,0,0,9,1,0,2,28,4,0,0,
9,3,0,4,12,6,0,1,42,0,0,0,9,5,0,6,
11,8,0,0,0,0,0,0,0,0,0,0,42,7,5,8,
18,0,0,18,12,10,0,9,98,97,115,101,95,100,109,97,
112,0,0,0,13,9,10,0,12,11,0,3,108,98,112,0,
15,12,1,0,12,13,0,2,98,112,0,0,15,14,1,0,
12,15,0,3,108,101,100,0,15,16,3,0,26,10,11,6,
10,9,7,10,18,0,255,238,0,0,0,0,12,48,0,7,
105,95,105,110,102,105,120,0,14,48,47,0,12,50,0,7,
105,95,105,110,102,105,120,0,13,49,50,0,11,50,0,0,
0,0,0,0,0,0,68,64,12,58,0,9,105,110,102,105,
120,95,108,101,100,0,0,0,13,51,58,0,12,52,0,1,
60,0,0,0,12,53,0,1,62,0,0,0,12,54,0,2,
60,61,0,0,12,55,0,2,62,61,0,0,12,56,0,2,
33,61,0,0,12,57,0,2,61,61,0,0,31,48,50,8,
19,48,49,48,12,51,0,7,105,95,105,110,102,105,120,0,
13,50,51,0,11,51,0,0,0,0,0,0,0,0,68,64,
12,55,0,8,105,110,102,105,120,95,105,115,0,0,0,0,
13,52,55,0,12,53,0,2,105,115,0,0,12,54,0,2,
105,110,0,0,31,48,51,4,19,48,50,48,12,52,0,7,
105,95,105,110,102,105,120,0,13,51,52,0,11,52,0,0,
0,0,0,0,0,0,36,64,12,58,0,9,105,110,102,105,
120,95,108,101,100,0,0,0,13,53,58,0,12,54,0,2,
43,61,0,0,12,55,0,2,45,61,0,0,12,56,0,2,
42,61,0,0,12,57,0,2,47,61,0,0,31,48,52,6,
19,48,51,48,12,53,0,7,105,95,105,110,102,105,120,0,
13,52,53,0,11,53,0,0,0,0,0,0,0,0,63,64,
12,57,0,9,105,110,102,105,120,95,108,101,100,0,0,0,
13,54,57,0,12,55,0,3,97,110,100,0,12,56,0,1,
38,0,0,0,31,48,53,4,19,48,52,48,12,54,0,7,
105,95,105,110,102,105,120,0,13,53,54,0,11,54,0,0,
0,0,0,0,0,0,62,64,12,58,0,9,105,110,102,105,
120,95,108,101,100,0,0,0,13,55,58,0,12,56,0,2,
111,114,0,0,12,57,0,1,124,0,0,0,31,48,54,4,
19,48,53,48,12,55,0,7,105,95,105,110,102,105,120,0,
13,54,55,0,11,55,0,0,0,0,0,0,0,0,66,64,
12,59,0,9,105,110,102,105,120,95,108,101,100,0,0,0,
13,56,59,0,12,57,0,2,60,60,0,0,12,58,0,2,
62,62,0,0,31,48,55,4,19,48,54,48,16,48,0,30,
44,12,0,0,12,2,0,1,42,0,0,0,9,1,0,2,
11,4,0,0,0,0,0,0,0,0,0,0,42,3,1,4,
18,0,0,20,12,6,0,9,98,97,115,101,95,100,109,97,
112,0,0,0,13,5,6,0,12,7,0,3,108,98,112,0,
11,8,0,0,0,0,0,0,0,0,0,0,12,9,0,3,
110,117,100,0,12,11,0,6,105,116,115,101,108,102,0,0,
13,10,11,0,26,6,7,4,10,5,3,6,18,0,255,236,
0,0,0,0,12,55,0,7,105,95,116,101,114,109,115,0,
14,55,48,0,12,57,0,7,105,95,116,101,114,109,115,0,
13,56,57,0,12,57,0,1,41,0,0,0,12,58,0,1,
125,0,0,0,12,59,0,1,93,0,0,0,12,60,0,1,
59,0,0,0,12,61,0,1,58,0,0,0,12,62,0,2,
110,108,0,0,12,63,0,4,101,108,105,102,0,0,0,0,
12,64,0,4,101,108,115,101,0,0,0,0,12,65,0,4,
84,114,117,101,0,0,0,0,12,66,0,5,70,97,108,115,
101,0,0,0,12,67,0,4,78,111,110,101,0,0,0,0,
12,68,0,4,110,97,109,101,0,0,0,0,12,69,0,6,
115,116,114,105,110,103,0,0,12,70,0,6,110,117,109,98,
101,114,0,0,12,71,0,6,105,110,100,101,110,116,0,0,
12,72,0,6,100,101,100,101,110,116,0,0,12,73,0,6,
101,120,99,101,112,116,0,0,31,55,57,17,19,55,56,55,
12,57,0,9,98,97,115,101,95,100,109,97,112,0,0,0,
13,55,57,0,12,57,0,2,110,108,0,0,9,55,55,57,
12,57,0,2,110,108,0,0,12,58,0,3,118,97,108,0,
10,55,58,57,16,57,0,38,44,9,0,0,28,2,0,0,
9,1,0,2,28,4,0,0,9,3,0,4,12,6,0,4,
100,109,97,112,0,0,0,0,13,5,6,0,36,5,5,3,
11,6,0,0,0,0,0,0,0,0,0,0,23,5,5,6,
21,5,0,0,18,0,0,15,12,7,0,5,101,114,114,111,
114,0,0,0,13,6,7,0,12,7,0,12,117,110,107,110,
111,119,110,32,34,37,115,34,0,0,0,0,39,7,7,3,
15,8,1,0,31,5,7,2,19,5,6,5,18,0,0,1,
12,8,0,4,100,109,97,112,0,0,0,0,13,7,8,0,
9,7,7,3,20,7,0,0,0,0,0,0,12,59,0,4,
103,109,97,112,0,0,0,0,14,59,57,0,16,59,0,49,
44,10,0,0,28,2,0,0,9,1,0,2,12,4,0,4,
116,121,112,101,0,0,0,0,9,3,1,4,12,4,0,6,
115,121,109,98,111,108,0,0,23,3,3,4,21,3,0,0,
18,0,0,13,12,6,0,4,103,109,97,112,0,0,0,0,
13,5,6,0,15,6,1,0,12,8,0,3,118,97,108,0,
9,7,1,8,31,4,6,2,19,4,5,4,15,3,4,0,
18,0,0,14,12,7,0,4,103,109,97,112,0,0,0,0,
13,6,7,0,15,7,1,0,12,9,0,4,116,121,112,101,
0,0,0,0,9,8,1,9,31,4,7,2,19,4,6,4,
15,3,4,0,18,0,0,1,11,7,0,0,0,0,0,0,
0,0,0,0,42,4,3,7,18,0,0,4,9,8,3,4,
10,1,4,8,18,0,255,252,20,1,0,0,0,0,0,0,
12,60,0,2,100,111,0,0,14,60,59,0,16,60,0,81,
44,15,0,0,12,3,0,1,80,0,0,0,13,2,3,0,
12,3,0,5,116,111,107,101,110,0,0,0,9,2,2,3,
15,1,2,0,27,3,0,0,15,2,3,0,11,3,0,0,
0,0,0,0,0,0,0,0,12,6,0,5,99,104,101,99,
107,0,0,0,13,5,6,0,12,8,0,1,80,0,0,0,
13,6,8,0,12,8,0,5,116,111,107,101,110,0,0,0,
9,6,6,8,12,7,0,3,101,111,102,0,31,4,6,2,
19,4,5,4,23,3,3,4,21,3,0,0,18,0,0,14,
12,7,0,6,97,112,112,101,110,100,0,0,9,6,2,7,
12,9,0,5,98,108,111,99,107,0,0,0,13,8,9,0,
31,7,0,0,19,7,8,7,31,4,7,1,19,4,6,4,
18,0,255,223,11,7,0,0,0,0,0,0,0,0,240,63,
12,11,0,3,108,101,110,0,13,10,11,0,15,11,2,0,
31,9,11,1,19,9,10,9,25,7,7,9,21,7,0,0,
18,0,0,17,12,11,0,5,109,107,116,111,107,0,0,0,
13,9,11,0,15,11,1,0,12,12,0,10,115,116,97,116,
101,109,101,110,116,115,0,0,12,13,0,1,59,0,0,0,
15,14,2,0,31,7,11,4,19,7,9,7,20,7,0,0,
18,0,0,1,12,12,0,3,112,111,112,0,9,11,2,12,
31,7,0,0,19,7,11,7,20,7,0,0,0,0,0,0,
12,61,0,9,100,111,95,109,111,100,117,108,101,0,0,0,
14,61,60,0,16,61,0,58,44,14,0,0,28,2,0,0,
9,1,0,2,28,4,0,0,9,3,0,4,11,5,0,0,
0,0,0,0,0,0,0,0,28,6,0,0,32,5,0,6,
12,9,0,8,116,111,107,101,110,105,122,101,0,0,0,0,
13,8,9,0,12,9,0,5,99,108,101,97,110,0,0,0,
9,8,8,9,15,9,1,0,31,7,9,1,19,7,8,7,
15,1,7,0,12,7,0,1,80,0,0,0,12,11,0,5,
80,68,97,116,97,0,0,0,13,10,11,0,15,11,1,0,
15,12,3,0,31,9,11,2,19,9,10,9,14,7,9,0,
12,11,0,1,80,0,0,0,13,9,11,0,12,11,0,4,
105,110,105,116,0,0,0,0,9,9,9,11,31,7,0,0,
19,7,9,7,12,13,0,9,100,111,95,109,111,100,117,108,
101,0,0,0,13,12,13,0,31,11,0,0,19,11,12,11,
15,7,11,0,12,11,0,1,80,0,0,0,28,13,0,0,
14,11,13,0,20,7,0,0,0,0,0,0,12,62,0,5,
112,97,114,115,101,0,0,0,14,62,61,0,0,0,0,0,
};
unsigned char tp_encode[] = {
44,151,0,0,12,2,0,6,105,109,112,111,114,116,0,0,
13,1,2,0,12,2,0,8,116,111,107,101,110,105,122,101,
0,0,0,0,31,0,2,1,19,0,1,0,12,2,0,8,
116,111,107,101,110,105,122,101,0,0,0,0,14,2,0,0,
11,0,0,0,0,0,0,0,0,0,0,0,12,4,0,3,
115,116,114,0,13,3,4,0,11,4,0,0,0,0,0,0,
0,0,240,63,31,2,4,1,19,2,3,2,12,4,0,1,
49,0,0,0,23,2,2,4,23,0,0,2,21,0,0,0,
18,0,0,21,12,2,0,4,102,114,111,109,0,0,0,0,
13,0,2,0,12,2,0,5,98,117,105,108,100,0,0,0,
13,0,2,0,12,4,0,6,105,109,112,111,114,116,0,0,
13,2,4,0,12,4,0,1,42,0,0,0,31,0,4,1,
19,0,2,0,12,4,0,1,42,0,0,0,14,4,0,0,
18,0,0,1,11,4,0,0,0,0,0,0,0,0,0,0,
15,0,4,0,11,6,0,0,0,0,0,0,0,0,240,63,
15,5,6,0,11,8,0,0,0,0,0,0,0,0,0,64,
15,7,8,0,11,10,0,0,0,0,0,0,0,0,8,64,
15,9,10,0,11,12,0,0,0,0,0,0,0,0,16,64,
15,11,12,0,11,14,0,0,0,0,0,0,0,0,20,64,
15,13,14,0,11,16,0,0,0,0,0,0,0,0,24,64,
15,15,16,0,11,18,0,0,0,0,0,0,0,0,28,64,
15,17,18,0,11,20,0,0,0,0,0,0,0,0,32,64,
15,19,20,0,11,22,0,0,0,0,0,0,0,0,34,64,
15,21,22,0,11,24,0,0,0,0,0,0,0,0,36,64,
15,23,24,0,11,26,0,0,0,0,0,0,0,0,38,64,
15,25,26,0,11,28,0,0,0,0,0,0,0,0,40,64,
15,27,28,0,11,30,0,0,0,0,0,0,0,0,42,64,
15,29,30,0,11,32,0,0,0,0,0,0,0,0,44,64,
15,31,32,0,11,34,0,0,0,0,0,0,0,0,46,64,
15,33,34,0,11,36,0,0,0,0,0,0,0,0,48,64,
15,35,36,0,11,38,0,0,0,0,0,0,0,0,49,64,
15,37,38,0,11,40,0,0,0,0,0,0,0,0,50,64,
15,39,40,0,11,42,0,0,0,0,0,0,0,0,51,64,
15,41,42,0,11,44,0,0,0,0,0,0,0,0,52,64,
15,43,44,0,11,46,0,0,0,0,0,0,0,0,53,64,
15,45,46,0,11,48,0,0,0,0,0,0,0,0,54,64,
15,47,48,0,11,50,0,0,0,0,0,0,0,0,55,64,
15,49,50,0,11,52,0,0,0,0,0,0,0,0,56,64,
15,51,52,0,11,54,0,0,0,0,0,0,0,0,57,64,
15,53,54,0,11,56,0,0,0,0,0,0,0,0,58,64,
15,55,56,0,11,58,0,0,0,0,0,0,0,0,59,64,
15,57,58,0,11,60,0,0,0,0,0,0,0,0,60,64,
15,59,60,0,11,62,0,0,0,0,0,0,0,0,61,64,
15,61,62,0,11,64,0,0,0,0,0,0,0,0,62,64,
15,63,64,0,11,66,0,0,0,0,0,0,0,0,63,64,
15,65,66,0,11,68,0,0,0,0,0,0,0,0,64,64,
15,67,68,0,11,70,0,0,0,0,0,0,0,128,64,64,
15,69,70,0,11,72,0,0,0,0,0,0,0,0,65,64,
15,71,72,0,11,74,0,0,0,0,0,0,0,128,65,64,
15,73,74,0,11,76,0,0,0,0,0,0,0,0,66,64,
15,75,76,0,11,78,0,0,0,0,0,0,0,128,66,64,
15,77,78,0,11,80,0,0,0,0,0,0,0,0,67,64,
15,79,80,0,11,82,0,0,0,0,0,0,0,128,67,64,
15,81,82,0,11,84,0,0,0,0,0,0,0,0,68,64,
15,83,84,0,11,86,0,0,0,0,0,0,0,128,68,64,
15,85,86,0,11,88,0,0,0,0,0,0,0,0,69,64,
15,87,88,0,11,90,0,0,0,0,0,0,0,128,69,64,
15,89,90,0,11,92,0,0,0,0,0,0,0,0,70,64,
15,91,92,0,12,93,0,3,69,79,70,0,14,93,0,0,
12,0,0,3,65,68,68,0,14,0,5,0,12,0,0,3,
83,85,66,0,14,0,7,0,12,0,0,3,77,85,76,0,
14,0,9,0,12,0,0,3,68,73,86,0,14,0,11,0,
12,0,0,3,80,79,87,0,14,0,13,0,12,0,0,3,
65,78,68,0,14,0,15,0,12,0,0,2,79,82,0,0,
14,0,17,0,12,0,0,3,67,77,80,0,14,0,19,0,
12,0,0,3,71,69,84,0,14,0,21,0,12,0,0,3,
83,69,84,0,14,0,23,0,12,0,0,6,78,85,77,66,
69,82,0,0,14,0,25,0,12,0,0,6,83,84,82,73,
78,71,0,0,14,0,27,0,12,0,0,4,71,71,69,84,
0,0,0,0,14,0,29,0,12,0,0,4,71,83,69,84,
0,0,0,0,14,0,31,0,12,0,0,4,77,79,86,69,
0,0,0,0,14,0,33,0,12,0,0,3,68,69,70,0,
14,0,35,0,12,0,0,4,80,65,83,83,0,0,0,0,
14,0,37,0,12,0,0,4,74,85,77,80,0,0,0,0,
14,0,39,0,12,0,0,4,67,65,76,76,0,0,0,0,
14,0,41,0,12,0,0,6,82,69,84,85,82,78,0,0,
14,0,43,0,12,0,0,2,73,70,0,0,14,0,45,0,
12,0,0,5,68,69,66,85,71,0,0,0,14,0,47,0,
12,0,0,2,69,81,0,0,14,0,49,0,12,0,0,2,
76,69,0,0,14,0,51,0,12,0,0,2,76,84,0,0,
14,0,53,0,12,0,0,4,68,73,67,84,0,0,0,0,
14,0,55,0,12,0,0,4,76,73,83,84,0,0,0,0,
14,0,57,0,12,0,0,4,78,79,78,69,0,0,0,0,
14,0,59,0,12,0,0,3,76,69,78,0,14,0,61,0,
12,0,0,3,80,79,83,0,14,0,63,0,12,0,0,6,
80,65,82,65,77,83,0,0,14,0,65,0,12,0,0,4,
73,71,69,84,0,0,0,0,14,0,67,0,12,0,0,4,
70,73,76,69,0,0,0,0,14,0,69,0,12,0,0,4,
78,65,77,69,0,0,0,0,14,0,71,0,12,0,0,2,
78,69,0,0,14,0,73,0,12,0,0,3,72,65,83,0,
14,0,75,0,12,0,0,5,82,65,73,83,69,0,0,0,
14,0,77,0,12,0,0,6,83,69,84,74,77,80,0,0,
14,0,79,0,12,0,0,3,77,79,68,0,14,0,81,0,
12,0,0,3,76,83,72,0,14,0,83,0,12,0,0,3,
82,83,72,0,14,0,85,0,12,0,0,4,73,84,69,82,
0,0,0,0,14,0,87,0,12,0,0,3,68,69,76,0,
14,0,89,0,12,0,0,4,82,69,71,83,0,0,0,0,
14,0,91,0,26,0,0,0,12,5,0,6,68,83,116,97,
116,101,0,0,14,5,0,0,16,7,0,86,44,33,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
28,6,0,0,9,5,0,6,15,7,3,0,15,8,5,0,
12,9,0,4,99,111,100,101,0,0,0,0,10,1,9,7,
12,10,0,5,102,110,97,109,101,0,0,0,10,1,10,8,
12,13,0,4,99,111,100,101,0,0,0,0,9,12,1,13,
12,13,0,5,115,112,108,105,116,0,0,0,9,12,12,13,
12,13,0,1,10,0,0,0,31,11,13,1,19,11,12,11,
12,13,0,5,108,105,110,101,115,0,0,0,10,1,13,11,
27,14,0,0,15,11,14,0,12,18,0,3,116,97,103,0,
12,19,0,3,69,79,70,0,27,17,18,2,27,16,17,1,
15,15,16,0,11,18,0,0,0,0,0,0,0,0,0,0,
15,17,18,0,27,20,0,0,15,19,20,0,11,22,0,0,
0,0,0,0,0,0,0,0,15,21,22,0,26,24,0,0,
15,23,24,0,12,25,0,5,115,116,97,99,107,0,0,0,
10,1,25,11,12,26,0,3,111,117,116,0,10,1,26,15,
12,27,0,7,95,115,99,111,112,101,105,0,10,1,27,17,
12,28,0,6,116,115,116,97,99,107,0,0,10,1,28,19,
12,29,0,5,95,116,97,103,105,0,0,0,10,1,29,21,
12,30,0,4,100,97,116,97,0,0,0,0,10,1,30,23,
11,31,0,0,0,0,0,0,0,0,0,0,12,32,0,5,
101,114,114,111,114,0,0,0,10,1,32,31,0,0,0,0,
12,9,0,8,95,95,105,110,105,116,95,95,0,0,0,0,
10,0,9,7,16,9,0,181,44,45,0,0,28,2,0,0,
9,1,0,2,11,3,0,0,0,0,0,0,0,0,0,0,
28,4,0,0,32,3,0,4,12,7,0,3,108,101,110,0,
13,6,7,0,12,8,0,5,115,116,97,99,107,0,0,0,
9,7,1,8,31,5,7,1,19,5,6,5,21,5,0,0,
18,0,0,52,12,8,0,5,115,116,97,99,107,0,0,0,
9,7,1,8,12,8,0,6,97,112,112,101,110,100,0,0,
9,7,7,8,12,19,0,4,118,97,114,115,0,0,0,0,
9,9,1,19,12,19,0,3,114,50,110,0,9,10,1,19,
12,19,0,3,110,50,114,0,9,11,1,19,12,19,0,5,
95,116,109,112,105,0,0,0,9,12,1,19,12,19,0,4,
109,114,101,103,0,0,0,0,9,13,1,19,12,19,0,4,
115,110,117,109,0,0,0,0,9,14,1,19,12,19,0,8,
95,103,108,111,98,97,108,115,0,0,0,0,9,15,1,19,
12,19,0,6,108,105,110,101,110,111,0,0,9,16,1,19,
12,19,0,7,103,108,111,98,97,108,115,0,9,17,1,19,
12,19,0,5,99,114,101,103,115,0,0,0,9,18,1,19,
27,8,9,10,31,5,8,1,19,5,7,5,18,0,0,13,
12,10,0,5,115,116,97,99,107,0,0,0,9,9,1,10,
12,10,0,6,97,112,112,101,110,100,0,0,9,9,9,10,
28,10,0,0,31,8,10,1,19,8,9,8,18,0,0,1,
27,11,0,0,15,10,11,0,26,13,0,0,15,12,13,0,
26,15,0,0,15,14,15,0,11,17,0,0,0,0,0,0,
0,0,0,0,15,16,17,0,11,19,0,0,0,0,0,0,
0,0,0,0,15,18,19,0,12,23,0,3,115,116,114,0,
13,22,23,0,12,24,0,7,95,115,99,111,112,101,105,0,
9,23,1,24,31,21,23,1,19,21,22,21,15,20,21,0,
15,23,3,0,11,25,0,0,0,0,0,0,0,0,240,191,
15,24,25,0,27,27,0,0,15,26,27,0,12,30,0,4,
114,101,103,115,0,0,0,0,27,29,30,1,15,28,29,0,
12,30,0,4,118,97,114,115,0,0,0,0,10,1,30,10,
12,31,0,3,114,50,110,0,10,1,31,12,12,32,0,3,
110,50,114,0,10,1,32,14,12,33,0,5,95,116,109,112,
105,0,0,0,10,1,33,16,12,34,0,4,109,114,101,103,
0,0,0,0,10,1,34,18,12,35,0,4,115,110,117,109,
0,0,0,0,10,1,35,20,12,36,0,8,95,103,108,111,
98,97,108,115,0,0,0,0,10,1,36,23,12,37,0,6,
108,105,110,101,110,111,0,0,10,1,37,24,12,38,0,7,
103,108,111,98,97,108,115,0,10,1,38,26,12,39,0,5,
99,114,101,103,115,0,0,0,10,1,39,28,12,41,0,7,
95,115,99,111,112,101,105,0,9,40,1,41,11,41,0,0,
0,0,0,0,0,0,240,63,1,40,40,41,12,41,0,7,
95,115,99,111,112,101,105,0,10,1,41,40,12,43,0,6,
105,110,115,101,114,116,0,0,13,42,43,0,12,44,0,5,
99,114,101,103,115,0,0,0,9,43,1,44,31,40,43,1,
19,40,42,40,0,0,0,0,12,11,0,5,98,101,103,105,
110,0,0,0,10,0,11,9,16,11,0,142,44,30,0,0,
28,2,0,0,9,1,0,2,12,5,0,5,99,114,101,103,
115,0,0,0,9,4,1,5,12,5,0,6,97,112,112,101,
110,100,0,0,9,4,4,5,12,6,0,4,109,114,101,103,
0,0,0,0,9,5,1,6,31,3,5,1,19,3,4,3,
12,6,0,4,99,111,100,101,0,0,0,0,13,5,6,0,
12,7,0,3,69,79,70,0,13,6,7,0,31,3,6,1,
19,3,5,3,11,3,0,0,0,0,0,0,0,0,240,63,
12,8,0,3,108,101,110,0,13,7,8,0,12,9,0,5,
115,116,97,99,107,0,0,0,9,8,1,9,31,6,8,1,
19,6,7,6,25,3,3,6,21,3,0,0,18,0,0,90,
12,8,0,5,115,116,97,99,107,0,0,0,9,6,1,8,
12,8,0,3,112,111,112,0,9,6,6,8,31,3,0,0,
19,3,6,3,11,9,0,0,0,0,0,0,0,0,0,0,
9,8,3,9,12,9,0,4,118,97,114,115,0,0,0,0,
10,1,9,8,11,11,0,0,0,0,0,0,0,0,240,63,
9,10,3,11,12,11,0,3,114,50,110,0,10,1,11,10,
11,13,0,0,0,0,0,0,0,0,0,64,9,12,3,13,
12,13,0,3,110,50,114,0,10,1,13,12,11,15,0,0,
0,0,0,0,0,0,8,64,9,14,3,15,12,15,0,5,
95,116,109,112,105,0,0,0,10,1,15,14,11,17,0,0,
0,0,0,0,0,0,16,64,9,16,3,17,12,17,0,4,
109,114,101,103,0,0,0,0,10,1,17,16,11,19,0,0,
0,0,0,0,0,0,20,64,9,18,3,19,12,19,0,4,
115,110,117,109,0,0,0,0,10,1,19,18,11,21,0,0,
0,0,0,0,0,0,24,64,9,20,3,21,12,21,0,8,
95,103,108,111,98,97,108,115,0,0,0,0,10,1,21,20,
11,23,0,0,0,0,0,0,0,0,28,64,9,22,3,23,
12,23,0,6,108,105,110,101,110,111,0,0,10,1,23,22,
11,25,0,0,0,0,0,0,0,0,32,64,9,24,3,25,
12,25,0,7,103,108,111,98,97,108,115,0,10,1,25,24,
11,27,0,0,0,0,0,0,0,0,34,64,9,26,3,27,
12,27,0,5,99,114,101,103,115,0,0,0,10,1,27,26,
18,0,0,11,12,29,0,5,115,116,97,99,107,0,0,0,
9,28,1,29,12,29,0,3,112,111,112,0,9,28,28,29,
31,3,0,0,19,3,28,3,18,0,0,1,0,0,0,0,
12,13,0,3,101,110,100,0,10,0,13,11,16,13,0,62,
44,12,0,0,28,2,0,0,9,1,0,2,12,5,0,4,
98,105,110,100,0,0,0,0,13,4,5,0,12,7,0,6,
68,83,116,97,116,101,0,0,13,5,7,0,12,7,0,8,
95,95,105,110,105,116,95,95,0,0,0,0,9,5,5,7,
15,6,1,0,31,3,5,2,19,3,4,3,12,5,0,8,
95,95,105,110,105,116,95,95,0,0,0,0,10,1,5,3,
12,7,0,4,98,105,110,100,0,0,0,0,13,6,7,0,
12,9,0,6,68,83,116,97,116,101,0,0,13,7,9,0,
12,9,0,5,98,101,103,105,110,0,0,0,9,7,7,9,
15,8,1,0,31,3,7,2,19,3,6,3,12,7,0,5,
98,101,103,105,110,0,0,0,10,1,7,3,12,9,0,4,
98,105,110,100,0,0,0,0,13,8,9,0,12,11,0,6,
68,83,116,97,116,101,0,0,13,9,11,0,12,11,0,3,
101,110,100,0,9,9,9,11,15,10,1,0,31,3,9,2,
19,3,8,3,12,9,0,3,101,110,100,0,10,1,9,3,
0,0,0,0,12,15,0,7,95,95,110,101,119,95,95,0,
10,0,15,13,16,15,0,22,44,7,0,0,26,1,0,0,
12,4,0,6,68,83,116,97,116,101,0,0,13,3,4,0,
12,4,0,7,95,95,110,101,119,95,95,0,9,3,3,4,
15,4,1,0,31,2,4,1,19,2,3,2,12,5,0,8,
95,95,105,110,105,116,95,95,0,0,0,0,9,4,1,5,
19,6,4,0,20,1,0,0,0,0,0,0,12,17,0,8,
95,95,99,97,108,108,95,95,0,0,0,0,10,0,17,15,
16,19,0,18,44,6,0,0,28,2,0,0,9,1,0,2,
12,5,0,1,68,0,0,0,13,4,5,0,12,5,0,3,
111,117,116,0,9,4,4,5,12,5,0,6,97,112,112,101,
110,100,0,0,9,4,4,5,15,5,1,0,31,3,5,1,
19,3,4,3,0,0,0,0,12,21,0,6,105,110,115,101,
114,116,0,0,14,21,19,0,16,21,0,68,44,19,0,0,
28,2,0,0,9,1,0,2,12,5,0,6,105,115,116,121,
112,101,0,0,13,4,5,0,15,5,1,0,12,6,0,4,
108,105,115,116,0,0,0,0,31,3,5,2,19,3,4,3,
21,3,0,0,18,0,0,11,12,6,0,6,105,110,115,101,
114,116,0,0,13,5,6,0,15,6,1,0,31,3,6,1,
19,3,5,3,28,3,0,0,20,3,0,0,18,0,0,1,
12,8,0,5,114,97,110,103,101,0,0,0,13,7,8,0,
11,8,0,0,0,0,0,0,0,0,0,0,12,12,0,3,
108,101,110,0,13,11,12,0,15,12,1,0,31,9,12,1,
19,9,11,9,11,10,0,0,0,0,0,0,0,0,16,64,
31,6,8,3,19,6,7,6,11,8,0,0,0,0,0,0,
0,0,0,0,42,3,6,8,18,0,0,19,12,12,0,6,
105,110,115,101,114,116,0,0,13,10,12,0,12,13,0,4,
100,97,116,97,0,0,0,0,15,16,3,0,11,18,0,0,
0,0,0,0,0,0,16,64,1,17,3,18,27,15,16,2,
9,14,1,15,27,12,13,2,31,9,12,1,19,9,10,9,
18,0,255,237,0,0,0,0,12,23,0,5,119,114,105,116,
101,0,0,0,14,23,21,0,16,23,0,107,44,16,0,0,
28,2,0,0,9,1,0,2,12,4,0,4,65,82,71,86,
0,0,0,0,13,3,4,0,12,4,0,6,45,110,111,112,
111,115,0,0,36,3,3,4,21,3,0,0,18,0,0,4,
28,3,0,0,20,3,0,0,18,0,0,1,11,5,0,0,
0,0,0,0,0,0,0,0,9,4,1,5,15,3,4,0,
11,6,0,0,0,0,0,0,0,0,240,63,9,5,1,6,
15,4,5,0,12,6,0,1,68,0,0,0,13,5,6,0,
12,6,0,6,108,105,110,101,110,111,0,0,9,5,5,6,
23,1,3,5,21,1,0,0,18,0,0,4,28,1,0,0,
20,1,0,0,18,0,0,1,12,6,0,1,68,0,0,0,
13,5,6,0,12,6,0,5,108,105,110,101,115,0,0,0,
9,5,5,6,11,7,0,0,0,0,0,0,0,0,240,63,
2,6,3,7,9,5,5,6,15,1,5,0,12,6,0,1,
68,0,0,0,13,5,6,0,12,6,0,6,108,105,110,101,
110,111,0,0,10,5,6,3,12,9,0,1,0,0,0,0,
11,10,0,0,0,0,0,0,0,0,16,64,12,13,0,3,
108,101,110,0,13,12,13,0,15,13,1,0,31,11,13,1,
19,11,12,11,11,13,0,0,0,0,0,0,0,0,16,64,
39,11,11,13,2,10,10,11,3,9,9,10,1,8,1,9,
15,7,8,0,12,10,0,7,99,111,100,101,95,49,54,0,
13,9,10,0,12,10,0,3,80,79,83,0,13,13,10,0,
12,11,0,3,108,101,110,0,13,10,11,0,15,11,7,0,
31,14,11,1,19,14,10,14,11,11,0,0,0,0,0,0,
0,0,16,64,4,14,14,11,15,15,3,0,31,8,13,3,
19,8,9,8,12,13,0,5,119,114,105,116,101,0,0,0,
13,11,13,0,15,13,7,0,31,8,13,1,19,8,11,8,
0,0,0,0,12,25,0,6,115,101,116,112,111,115,0,0,
14,25,23,0,16,25,0,110,44,21,0,0,28,2,0,0,
9,1,0,2,11,3,0,0,0,0,0,0,0,0,0,0,
28,4,0,0,32,3,0,4,11,5,0,0,0,0,0,0,
0,0,0,0,28,6,0,0,32,5,0,6,11,7,0,0,
0,0,0,0,0,0,0,0,28,8,0,0,32,7,0,8,
11,9,0,0,0,0,0,0,0,0,0,0,12,12,0,6,
105,115,116,121,112,101,0,0,13,11,12,0,15,12,1,0,
12,13,0,6,110,117,109,98,101,114,0,0,31,10,12,2,
19,10,11,10,23,9,9,10,21,9,0,0,18,0,0,4,
28,9,0,0,37,9,0,0,18,0,0,1,11,9,0,0,
0,0,0,0,0,0,0,0,12,13,0,6,105,115,116,121,
112,101,0,0,13,12,13,0,15,13,3,0,12,14,0,6,
110,117,109,98,101,114,0,0,31,10,13,2,19,10,12,10,
23,9,9,10,21,9,0,0,18,0,0,4,28,9,0,0,
37,9,0,0,18,0,0,1,11,9,0,0,0,0,0,0,
0,0,0,0,12,14,0,6,105,115,116,121,112,101,0,0,
13,13,14,0,15,14,5,0,12,15,0,6,110,117,109,98,
101,114,0,0,31,10,14,2,19,10,13,10,23,9,9,10,
21,9,0,0,18,0,0,4,28,9,0,0,37,9,0,0,
18,0,0,1,11,9,0,0,0,0,0,0,0,0,0,0,
12,15,0,6,105,115,116,121,112,101,0,0,13,14,15,0,
15,15,7,0,12,16,0,6,110,117,109,98,101,114,0,0,
31,10,15,2,19,10,14,10,23,9,9,10,21,9,0,0,
18,0,0,4,28,9,0,0,37,9,0,0,18,0,0,1,
12,15,0,5,119,114,105,116,101,0,0,0,13,10,15,0,
12,16,0,4,99,111,100,101,0,0,0,0,15,17,1,0,
15,18,3,0,15,19,5,0,15,20,7,0,27,15,16,5,
31,9,15,1,19,9,10,9,0,0,0,0,12,27,0,4,
99,111,100,101,0,0,0,0,14,27,25,0,16,27,0,45,
44,14,0,0,28,2,0,0,9,1,0,2,28,4,0,0,
9,3,0,4,28,6,0,0,9,5,0,6,11,8,0,0,
0,0,0,0,0,0,0,0,25,7,5,8,21,7,0,0,
18,0,0,7,11,8,0,0,0,0,0,0,0,0,224,64,
1,7,5,8,15,5,7,0,18,0,0,1,12,9,0,4,
99,111,100,101,0,0,0,0,13,8,9,0,15,9,1,0,
15,10,3,0,11,13,0,0,0,0,0,0,0,224,239,64,
6,11,5,13,11,13,0,0,0,0,0,0,0,0,32,64,
41,11,11,13,11,13,0,0,0,0,0,0,0,224,111,64,
6,12,5,13,11,13,0,0,0,0,0,0,0,0,0,0,
41,12,12,13,31,7,9,4,19,7,8,7,0,0,0,0,
12,29,0,7,99,111,100,101,95,49,54,0,14,29,27,0,
16,29,0,32,44,14,0,0,28,2,0,0,9,1,0,2,
28,4,0,0,9,3,0,4,28,6,0,0,9,5,0,6,
12,8,0,4,99,111,100,101,0,0,0,0,15,9,1,0,
15,10,3,0,11,13,0,0,0,0,0,0,0,224,239,64,
6,11,5,13,11,13,0,0,0,0,0,0,0,0,32,64,
41,11,11,13,11,13,0,0,0,0,0,0,0,224,111,64,
6,12,5,13,11,13,0,0,0,0,0,0,0,0,0,0,
41,12,12,13,27,7,8,5,20,7,0,0,0,0,0,0,
12,31,0,10,103,101,116,95,99,111,100,101,49,54,0,0,
14,31,29,0,16,31,0,67,44,16,0,0,28,2,0,0,
9,1,0,2,28,3,0,0,28,4,0,0,32,3,0,4,
12,7,0,7,103,101,116,95,116,109,112,0,13,6,7,0,
15,7,3,0,31,5,7,1,19,5,6,5,15,3,5,0,
12,8,0,3,118,97,108,0,9,7,1,8,12,8,0,1,
0,0,0,0,11,9,0,0,0,0,0,0,0,0,16,64,
12,12,0,3,108,101,110,0,13,11,12,0,12,13,0,3,
118,97,108,0,9,12,1,13,31,10,12,1,19,10,11,10,
11,12,0,0,0,0,0,0,0,0,16,64,39,10,10,12,
2,9,9,10,3,8,8,9,1,7,7,8,15,5,7,0,
12,9,0,7,99,111,100,101,95,49,54,0,13,8,9,0,
12,9,0,6,83,84,82,73,78,71,0,0,13,12,9,0,
15,13,3,0,12,10,0,3,108,101,110,0,13,9,10,0,
12,15,0,3,118,97,108,0,9,10,1,15,31,14,10,1,
19,14,9,14,31,7,12,3,19,7,8,7,12,12,0,5,
119,114,105,116,101,0,0,0,13,10,12,0,15,12,5,0,
31,7,12,1,19,7,10,7,20,3,0,0,0,0,0,0,
12,33,0,9,100,111,95,115,116,114,105,110,103,0,0,0,
14,33,31,0,16,33,0,55,44,15,0,0,28,2,0,0,
9,1,0,2,28,3,0,0,28,4,0,0,32,3,0,4,
12,7,0,7,103,101,116,95,116,109,112,0,13,6,7,0,
15,7,3,0,31,5,7,1,19,5,6,5,15,3,5,0,
12,8,0,4,99,111,100,101,0,0,0,0,13,7,8,0,
12,12,0,6,78,85,77,66,69,82,0,0,13,8,12,0,
15,9,3,0,11,10,0,0,0,0,0,0,0,0,0,0,
11,11,0,0,0,0,0,0,0,0,0,0,31,5,8,4,
19,5,7,5,12,9,0,5,119,114,105,116,101,0,0,0,
13,8,9,0,12,11,0,5,102,112,97,99,107,0,0,0,
13,10,11,0,12,13,0,6,110,117,109,98,101,114,0,0,
13,12,13,0,12,14,0,3,118,97,108,0,9,13,1,14,
31,11,13,1,19,11,12,11,31,9,11,1,19,9,10,9,
31,5,9,1,19,5,8,5,20,3,0,0,0,0,0,0,
12,35,0,9,100,111,95,110,117,109,98,101,114,0,0,0,
14,35,33,0,16,35,0,35,44,6,0,0,12,4,0,3,
115,116,114,0,13,3,4,0,12,5,0,1,68,0,0,0,
13,4,5,0,12,5,0,5,95,116,97,103,105,0,0,0,
9,4,4,5,31,2,4,1,19,2,3,2,15,1,2,0,
12,4,0,1,68,0,0,0,13,2,4,0,12,5,0,1,
68,0,0,0,13,4,5,0,12,5,0,5,95,116,97,103,
105,0,0,0,9,4,4,5,11,5,0,0,0,0,0,0,
0,0,240,63,1,4,4,5,12,5,0,5,95,116,97,103,
105,0,0,0,10,2,5,4,20,1,0,0,0,0,0,0,
12,37,0,7,103,101,116,95,116,97,103,0,14,37,35,0,
16,37,0,25,44,6,0,0,12,4,0,7,103,101,116,95,
116,97,103,0,13,3,4,0,31,2,0,0,19,2,3,2,
15,1,2,0,12,5,0,1,68,0,0,0,13,4,5,0,
12,5,0,6,116,115,116,97,99,107,0,0,9,4,4,5,
12,5,0,6,97,112,112,101,110,100,0,0,9,4,4,5,
15,5,1,0,31,2,5,1,19,2,4,2,20,1,0,0,
0,0,0,0,12,39,0,9,115,116,97,99,107,95,116,97,
103,0,0,0,14,39,37,0,16,39,0,15,44,4,0,0,
12,3,0,1,68,0,0,0,13,2,3,0,12,3,0,6,
116,115,116,97,99,107,0,0,9,2,2,3,12,3,0,3,
112,111,112,0,9,2,2,3,31,1,0,0,19,1,2,1,
0,0,0,0,12,41,0,7,112,111,112,95,116,97,103,0,
14,41,39,0,16,41,0,52,44,15,0,0,12,2,0,1,
42,0,0,0,9,1,0,2,12,4,0,1,68,0,0,0,
13,3,4,0,12,4,0,4,115,110,117,109,0,0,0,0,
9,3,3,4,12,4,0,1,58,0,0,0,1,3,3,4,
12,5,0,1,58,0,0,0,12,6,0,4,106,111,105,110,
0,0,0,0,9,5,5,6,27,7,0,0,11,9,0,0,
0,0,0,0,0,0,0,0,42,8,1,9,18,0,0,10,
12,12,0,3,115,116,114,0,13,11,12,0,15,12,8,0,
31,10,12,1,19,10,11,10,28,12,0,0,10,7,12,10,
18,0,255,246,15,6,7,0,31,4,6,1,19,4,5,4,
1,3,3,4,15,1,3,0,12,6,0,6,105,110,115,101,
114,116,0,0,13,4,6,0,12,13,0,3,116,97,103,0,
15,14,1,0,27,6,13,2,31,3,6,1,19,3,4,3,
0,0,0,0,12,43,0,3,116,97,103,0,14,43,41,0,
16,43,0,53,44,15,0,0,12,2,0,1,42,0,0,0,
9,1,0,2,12,4,0,1,68,0,0,0,13,3,4,0,
12,4,0,4,115,110,117,109,0,0,0,0,9,3,3,4,
12,4,0,1,58,0,0,0,1,3,3,4,12,5,0,1,
58,0,0,0,12,6,0,4,106,111,105,110,0,0,0,0,
9,5,5,6,27,7,0,0,11,9,0,0,0,0,0,0,
0,0,0,0,42,8,1,9,18,0,0,10,12,12,0,3,
115,116,114,0,13,11,12,0,15,12,8,0,31,10,12,1,
19,10,11,10,28,12,0,0,10,7,12,10,18,0,255,246,
15,6,7,0,31,4,6,1,19,4,5,4,1,3,3,4,
15,1,3,0,12,6,0,6,105,110,115,101,114,116,0,0,
13,4,6,0,12,13,0,4,106,117,109,112,0,0,0,0,
15,14,1,0,27,6,13,2,31,3,6,1,19,3,4,3,
0,0,0,0,12,45,0,4,106,117,109,112,0,0,0,0,
14,45,43,0,16,45,0,53,44,15,0,0,12,2,0,1,
42,0,0,0,9,1,0,2,12,4,0,1,68,0,0,0,
13,3,4,0,12,4,0,4,115,110,117,109,0,0,0,0,
9,3,3,4,12,4,0,1,58,0,0,0,1,3,3,4,
12,5,0,1,58,0,0,0,12,6,0,4,106,111,105,110,
0,0,0,0,9,5,5,6,27,7,0,0,11,9,0,0,
0,0,0,0,0,0,0,0,42,8,1,9,18,0,0,10,
12,12,0,3,115,116,114,0,13,11,12,0,15,12,8,0,
31,10,12,1,19,10,11,10,28,12,0,0,10,7,12,10,
18,0,255,246,15,6,7,0,31,4,6,1,19,4,5,4,
1,3,3,4,15,1,3,0,12,6,0,6,105,110,115,101,
114,116,0,0,13,4,6,0,12,13,0,6,115,101,116,106,
109,112,0,0,15,14,1,0,27,6,13,2,31,3,6,1,
19,3,4,3,0,0,0,0,12,47,0,6,115,101,116,106,
109,112,0,0,14,47,45,0,16,47,0,62,44,17,0,0,
12,2,0,1,42,0,0,0,9,1,0,2,12,4,0,1,
68,0,0,0,13,3,4,0,12,4,0,4,115,110,117,109,
0,0,0,0,9,3,3,4,12,4,0,1,58,0,0,0,
1,3,3,4,12,5,0,1,58,0,0,0,12,6,0,4,
106,111,105,110,0,0,0,0,9,5,5,6,27,7,0,0,
11,9,0,0,0,0,0,0,0,0,0,0,42,8,1,9,
18,0,0,10,12,12,0,3,115,116,114,0,13,11,12,0,
15,12,8,0,31,10,12,1,19,10,11,10,28,12,0,0,
10,7,12,10,18,0,255,246,15,6,7,0,31,4,6,1,
19,4,5,4,1,3,3,4,15,1,3,0,12,9,0,7,
103,101,116,95,114,101,103,0,13,6,9,0,15,9,1,0,
31,4,9,1,19,4,6,4,15,3,4,0,12,13,0,6,
105,110,115,101,114,116,0,0,13,9,13,0,12,14,0,3,
102,110,99,0,15,15,3,0,15,16,1,0,27,13,14,3,
31,4,13,1,19,4,9,4,20,3,0,0,0,0,0,0,
12,49,0,3,102,110,99,0,14,49,47,0,16,49,1,119,
44,41,0,0,26,2,0,0,15,1,2,0,27,3,0,0,
15,2,3,0,11,4,0,0,0,0,0,0,0,0,0,0,
15,3,4,0,12,6,0,1,68,0,0,0,13,5,6,0,
12,6,0,3,111,117,116,0,9,5,5,6,11,6,0,0,
0,0,0,0,0,0,0,0,42,4,5,6,18,0,0,71,
11,8,0,0,0,0,0,0,0,0,0,0,9,7,4,8,
12,8,0,3,116,97,103,0,23,7,7,8,21,7,0,0,
18,0,0,8,11,8,0,0,0,0,0,0,0,0,240,63,
9,7,4,8,10,1,7,3,18,0,255,240,18,0,0,1,
11,9,0,0,0,0,0,0,0,0,0,0,9,8,4,9,
12,9,0,4,114,101,103,115,0,0,0,0,23,8,8,9,
21,8,0,0,18,0,0,32,12,10,0,6,97,112,112,101,
110,100,0,0,9,9,2,10,12,12,0,10,103,101,116,95,
99,111,100,101,49,54,0,0,13,11,12,0,12,15,0,4,
82,69,71,83,0,0,0,0,13,12,15,0,11,15,0,0,
0,0,0,0,0,0,240,63,9,13,4,15,11,14,0,0,
0,0,0,0,0,0,0,0,31,10,12,3,19,10,11,10,
31,8,10,1,19,8,9,8,11,10,0,0,0,0,0,0,
0,0,240,63,1,8,3,10,15,3,8,0,18,0,255,199,
18,0,0,1,12,12,0,6,97,112,112,101,110,100,0,0,
9,10,2,12,15,12,4,0,31,8,12,1,19,8,10,8,
11,12,0,0,0,0,0,0,0,0,240,63,1,8,3,12,
15,3,8,0,18,0,255,185,12,12,0,5,114,97,110,103,
101,0,0,0,13,8,12,0,11,12,0,0,0,0,0,0,
0,0,0,0,12,15,0,3,108,101,110,0,13,14,15,0,
15,15,2,0,31,13,15,1,19,13,14,13,31,6,12,2,
19,6,8,6,11,12,0,0,0,0,0,0,0,0,0,0,
42,3,6,12,18,0,0,99,9,13,2,3,15,4,13,0,
11,15,0,0,0,0,0,0,0,0,0,0,9,13,4,15,
12,15,0,4,106,117,109,112,0,0,0,0,23,13,13,15,
21,13,0,0,18,0,0,23,12,16,0,10,103,101,116,95,
99,111,100,101,49,54,0,0,13,15,16,0,12,19,0,4,
74,85,77,80,0,0,0,0,13,16,19,0,11,17,0,0,
0,0,0,0,0,0,0,0,11,20,0,0,0,0,0,0,
0,0,240,63,9,19,4,20,9,18,1,19,2,18,18,3,
31,13,16,3,19,13,15,13,10,2,3,13,18,0,0,64,
11,17,0,0,0,0,0,0,0,0,0,0,9,16,4,17,
12,17,0,6,115,101,116,106,109,112,0,0,23,16,16,17,
21,16,0,0,18,0,0,23,12,18,0,10,103,101,116,95,
99,111,100,101,49,54,0,0,13,17,18,0,12,21,0,6,
83,69,84,74,77,80,0,0,13,18,21,0,11,19,0,0,
0,0,0,0,0,0,0,0,11,22,0,0,0,0,0,0,
0,0,240,63,9,21,4,22,9,20,1,21,2,20,20,3,
31,16,18,3,19,16,17,16,10,2,3,16,18,0,0,32,
11,19,0,0,0,0,0,0,0,0,0,0,9,18,4,19,
12,19,0,3,102,110,99,0,23,18,18,19,21,18,0,0,
18,0,0,23,12,20,0,10,103,101,116,95,99,111,100,101,
49,54,0,0,13,19,20,0,12,23,0,3,68,69,70,0,
13,20,23,0,11,23,0,0,0,0,0,0,0,0,240,63,
9,21,4,23,11,24,0,0,0,0,0,0,0,0,0,64,
9,23,4,24,9,22,1,23,2,22,22,3,31,18,20,3,
19,18,19,18,10,2,3,18,18,0,0,1,18,0,255,157,
12,21,0,5,114,97,110,103,101,0,0,0,13,20,21,0,
11,21,0,0,0,0,0,0,0,0,0,0,12,24,0,3,
108,101,110,0,13,23,24,0,15,24,2,0,31,22,24,1,
19,22,23,22,31,12,21,2,19,12,20,12,11,21,0,0,
0,0,0,0,0,0,0,0,42,3,12,21,18,0,0,140,
9,22,2,3,15,4,22,0,11,24,0,0,0,0,0,0,
0,0,0,0,9,22,4,24,12,24,0,4,100,97,116,97,
0,0,0,0,23,22,22,24,21,22,0,0,18,0,0,7,
11,24,0,0,0,0,0,0,0,0,240,63,9,22,4,24,
10,2,3,22,18,0,0,78,11,25,0,0,0,0,0,0,
0,0,0,0,9,24,4,25,12,25,0,4,99,111,100,101,
0,0,0,0,23,24,24,25,21,24,0,0,18,0,0,56,
11,26,0,0,0,0,0,0,0,0,240,63,28,27,0,0,
27,25,26,2,9,24,4,25,11,27,0,0,0,0,0,0,
0,0,0,0,9,26,24,27,15,25,26,0,11,28,0,0,
0,0,0,0,0,0,240,63,9,27,24,28,15,26,27,0,
11,29,0,0,0,0,0,0,0,0,0,64,9,28,24,29,
15,27,28,0,11,30,0,0,0,0,0,0,0,0,8,64,
9,29,24,30,15,28,29,0,12,30,0,3,99,104,114,0,
13,29,30,0,15,30,25,0,31,24,30,1,19,24,29,24,
12,32,0,3,99,104,114,0,13,31,32,0,15,32,26,0,
31,30,32,1,19,30,31,30,1,24,24,30,12,33,0,3,
99,104,114,0,13,32,33,0,15,33,27,0,31,30,33,1,
19,30,32,30,1,24,24,30,12,34,0,3,99,104,114,0,
13,33,34,0,15,34,28,0,31,30,34,1,19,30,33,30,
1,24,24,30,10,2,3,24,18,0,0,13,12,34,0,3,
115,116,114,0,13,30,34,0,12,35,0,4,104,117,104,63,
0,0,0,0,15,36,4,0,27,34,35,2,31,24,34,1,
19,24,30,24,37,24,0,0,18,0,0,1,12,35,0,3,
108,101,110,0,13,34,35,0,9,35,2,3,31,24,35,1,
19,24,34,24,11,35,0,0,0,0,0,0,0,0,16,64,
35,24,24,35,21,24,0,0,18,0,0,32,12,24,0,5,
99,111,100,101,32,0,0,0,12,37,0,3,115,116,114,0,
13,36,37,0,15,37,3,0,31,35,37,1,19,35,36,35,
1,24,24,35,12,35,0,17,32,105,115,32,119,114,111,110,
103,32,108,101,110,103,116,104,32,0,0,0,1,24,24,35,
12,38,0,3,115,116,114,0,13,37,38,0,12,40,0,3,
108,101,110,0,13,39,40,0,9,40,2,3,31,38,40,1,
19,38,39,38,31,35,38,1,19,35,37,35,1,24,24,35,
37,24,0,0,18,0,0,1,18,0,255,116,12,24,0,1,
68,0,0,0,13,21,24,0,12,24,0,3,111,117,116,0,
10,21,24,2,0,0,0,0,12,51,0,8,109,97,112,95,
116,97,103,115,0,0,0,0,14,51,49,0,16,51,0,27,
44,6,0,0,28,1,0,0,28,2,0,0,32,1,0,2,
28,4,0,0,35,3,1,4,21,3,0,0,18,0,0,3,
20,1,0,0,18,0,0,1,12,5,0,8,103,101,116,95,
116,109,112,115,0,0,0,0,13,4,5,0,11,5,0,0,
0,0,0,0,0,0,240,63,31,3,5,1,19,3,4,3,
11,5,0,0,0,0,0,0,0,0,0,0,9,3,3,5,
20,3,0,0,0,0,0,0,12,53,0,7,103,101,116,95,
116,109,112,0,14,53,51,0,16,53,0,69,44,17,0,0,
28,2,0,0,9,1,0,2,12,6,0,5,97,108,108,111,
99,0,0,0,13,5,6,0,15,6,1,0,31,4,6,1,
19,4,5,4,15,3,4,0,12,8,0,5,114,97,110,103,
101,0,0,0,13,7,8,0,15,8,3,0,1,9,3,1,
31,6,8,2,19,6,7,6,15,4,6,0,11,8,0,0,
0,0,0,0,0,0,0,0,42,6,4,8,18,0,0,42,
12,11,0,7,115,101,116,95,114,101,103,0,13,10,11,0,
15,11,6,0,12,12,0,1,36,0,0,0,12,15,0,3,
115,116,114,0,13,14,15,0,12,16,0,1,68,0,0,0,
13,15,16,0,12,16,0,5,95,116,109,112,105,0,0,0,
9,15,15,16,31,13,15,1,19,13,14,13,1,12,12,13,
31,9,11,2,19,9,10,9,12,11,0,1,68,0,0,0,
13,9,11,0,12,12,0,1,68,0,0,0,13,11,12,0,
12,12,0,5,95,116,109,112,105,0,0,0,9,11,11,12,
11,12,0,0,0,0,0,0,0,0,240,63,1,11,11,12,
12,12,0,5,95,116,109,112,105,0,0,0,10,9,12,11,
18,0,255,214,20,4,0,0,0,0,0,0,12,55,0,8,
103,101,116,95,116,109,112,115,0,0,0,0,14,55,53,0,
16,55,0,69,44,17,0,0,28,2,0,0,9,1,0,2,
12,5,0,0,0,0,0,0,12,6,0,4,106,111,105,110,
0,0,0,0,9,5,5,6,27,7,0,0,12,11,0,5,
114,97,110,103,101,0,0,0,13,10,11,0,11,11,0,0,
0,0,0,0,0,0,0,0,12,14,0,3,109,105,110,0,
13,13,14,0,11,14,0,0,0,0,0,0,0,0,112,64,
12,16,0,1,68,0,0,0,13,15,16,0,12,16,0,4,
109,114,101,103,0,0,0,0,9,15,15,16,1,15,15,1,
31,12,14,2,19,12,13,12,31,9,11,2,19,9,10,9,
11,11,0,0,0,0,0,0,0,0,0,0,42,8,9,11,
18,0,0,14,12,12,0,2,48,49,0,0,12,15,0,1,
68,0,0,0,13,14,15,0,12,15,0,3,114,50,110,0,
9,14,14,15,36,14,14,8,9,12,12,14,28,14,0,0,
10,7,14,12,18,0,255,242,15,6,7,0,31,4,6,1,
19,4,5,4,15,3,4,0,12,11,0,5,105,110,100,101,
120,0,0,0,9,6,3,11,12,11,0,1,48,0,0,0,
3,11,11,1,31,4,11,1,19,4,6,4,20,4,0,0,
0,0,0,0,12,57,0,5,97,108,108,111,99,0,0,0,
14,57,55,0,16,57,0,29,44,5,0,0,28,2,0,0,
9,1,0,2,28,4,0,0,23,3,1,4,21,3,0,0,
18,0,0,6,11,3,0,0,0,0,0,0,0,0,0,0,
20,3,0,0,18,0,0,1,12,4,0,1,68,0,0,0,
13,3,4,0,12,4,0,3,114,50,110,0,9,3,3,4,
9,3,3,1,11,4,0,0,0,0,0,0,0,0,0,0,
9,3,3,4,12,4,0,1,36,0,0,0,23,3,3,4,
20,3,0,0,0,0,0,0,12,59,0,6,105,115,95,116,
109,112,0,0,14,59,57,0,16,59,0,31,44,9,0,0,
28,2,0,0,9,1,0,2,12,5,0,1,68,0,0,0,
13,4,5,0,12,5,0,3,114,50,110,0,9,4,4,5,
9,4,4,1,15,3,4,0,12,6,0,8,102,114,101,101,
95,114,101,103,0,0,0,0,13,5,6,0,15,6,1,0,
31,4,6,1,19,4,5,4,12,7,0,7,115,101,116,95,
114,101,103,0,13,6,7,0,15,7,1,0,12,8,0,1,
42,0,0,0,1,8,8,3,31,4,7,2,19,4,6,4,
0,0,0,0,12,61,0,6,117,110,95,116,109,112,0,0,
14,61,59,0,16,61,0,24,44,7,0,0,28,2,0,0,
9,1,0,2,12,5,0,6,105,115,95,116,109,112,0,0,
13,4,5,0,15,5,1,0,31,3,5,1,19,3,4,3,
21,3,0,0,18,0,0,10,12,6,0,8,102,114,101,101,
95,114,101,103,0,0,0,0,13,5,6,0,15,6,1,0,
31,3,6,1,19,3,5,3,18,0,0,1,20,1,0,0,
0,0,0,0,12,63,0,8,102,114,101,101,95,116,109,112,
0,0,0,0,14,63,61,0,16,63,0,19,44,8,0,0,
28,2,0,0,9,1,0,2,11,4,0,0,0,0,0,0,
0,0,0,0,42,3,1,4,18,0,0,10,12,7,0,8,
102,114,101,101,95,116,109,112,0,0,0,0,13,6,7,0,
15,7,3,0,31,5,7,1,19,5,6,5,18,0,255,246,
0,0,0,0,12,65,0,9,102,114,101,101,95,116,109,112,
115,0,0,0,14,65,63,0,16,65,0,43,44,9,0,0,
28,2,0,0,9,1,0,2,12,4,0,1,68,0,0,0,
13,3,4,0,12,4,0,3,110,50,114,0,9,3,3,4,
36,3,3,1,11,4,0,0,0,0,0,0,0,0,0,0,
23,3,3,4,21,3,0,0,18,0,0,18,12,5,0,7,
115,101,116,95,114,101,103,0,13,4,5,0,12,8,0,5,
97,108,108,111,99,0,0,0,13,7,8,0,11,8,0,0,
0,0,0,0,0,0,240,63,31,5,8,1,19,5,7,5,
15,6,1,0,31,3,5,2,19,3,4,3,18,0,0,1,
12,6,0,1,68,0,0,0,13,5,6,0,12,6,0,3,
110,50,114,0,9,5,5,6,9,5,5,1,20,5,0,0,
0,0,0,0,12,67,0,7,103,101,116,95,114,101,103,0,
14,67,65,0,16,67,0,39,44,8,0,0,28,2,0,0,
9,1,0,2,12,4,0,1,68,0,0,0,13,3,4,0,
12,4,0,3,110,50,114,0,9,3,3,4,36,3,3,1,
21,3,0,0,18,0,0,4,28,3,0,0,37,3,0,0,
18,0,0,1,12,5,0,7,115,101,116,95,114,101,103,0,
13,4,5,0,12,7,0,1,68,0,0,0,13,5,7,0,
12,7,0,4,109,114,101,103,0,0,0,0,9,5,5,7,
15,6,1,0,31,3,5,2,19,3,4,3,12,5,0,1,
68,0,0,0,13,3,5,0,12,5,0,3,110,50,114,0,
9,3,3,5,9,3,3,1,20,3,0,0,0,0,0,0,
12,69,0,13,103,101,116,95,99,108,101,97,110,95,114,101,
103,0,0,0,14,69,67,0,16,69,0,44,44,13,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
12,6,0,1,68,0,0,0,13,5,6,0,12,6,0,3,
110,50,114,0,9,5,5,6,10,5,3,1,12,7,0,1,
68,0,0,0,13,6,7,0,12,7,0,3,114,50,110,0,
9,6,6,7,10,6,1,3,12,8,0,1,68,0,0,0,
13,7,8,0,12,10,0,3,109,97,120,0,13,9,10,0,
12,12,0,1,68,0,0,0,13,10,12,0,12,12,0,4,
109,114,101,103,0,0,0,0,9,10,10,12,11,12,0,0,
0,0,0,0,0,0,240,63,1,11,1,12,31,8,10,2,
19,8,9,8,12,10,0,4,109,114,101,103,0,0,0,0,
10,7,10,8,0,0,0,0,12,71,0,7,115,101,116,95,
114,101,103,0,14,71,69,0,16,71,0,27,44,7,0,0,
28,2,0,0,9,1,0,2,12,5,0,1,68,0,0,0,
13,4,5,0,12,5,0,3,114,50,110,0,9,4,4,5,
9,4,4,1,15,3,4,0,12,5,0,1,68,0,0,0,
13,4,5,0,12,5,0,3,114,50,110,0,9,4,4,5,
43,4,1,0,12,6,0,1,68,0,0,0,13,5,6,0,
12,6,0,3,110,50,114,0,9,5,5,6,43,5,3,0,
0,0,0,0,12,73,0,8,102,114,101,101,95,114,101,103,
0,0,0,0,14,73,71,0,16,73,0,56,44,20,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
12,7,0,5,105,116,101,109,115,0,0,0,9,6,1,7,
15,5,6,0,12,7,0,3,118,97,108,0,9,6,1,7,
28,8,0,0,11,9,0,0,0,0,0,0,0,0,240,191,
27,7,8,2,9,6,6,7,12,7,0,3,118,97,108,0,
10,1,7,6,12,9,0,4,102,114,111,109,0,0,0,0,
12,17,0,4,102,114,111,109,0,0,0,0,9,10,1,17,
12,11,0,4,116,121,112,101,0,0,0,0,12,12,0,6,
115,121,109,98,111,108,0,0,12,13,0,3,118,97,108,0,
12,14,0,1,61,0,0,0,12,15,0,5,105,116,101,109,
115,0,0,0,11,19,0,0,0,0,0,0,0,0,0,0,
9,17,5,19,15,18,1,0,27,16,17,2,26,8,9,8,
15,6,8,0,15,9,6,0,31,8,9,1,19,8,3,8,
20,8,0,0,0,0,0,0,12,75,0,7,105,109,97,110,
97,103,101,0,14,75,73,0,16,75,0,68,44,22,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
28,6,0,0,9,5,0,6,28,7,0,0,28,8,0,0,
32,7,0,8,12,11,0,7,103,101,116,95,116,109,112,0,
13,10,11,0,15,11,7,0,31,9,11,1,19,9,10,9,
15,7,9,0,12,13,0,2,100,111,0,0,13,12,13,0,
15,13,3,0,15,14,7,0,31,11,13,2,19,11,12,11,
15,9,11,0,12,16,0,2,100,111,0,0,13,15,16,0,
15,16,5,0,31,14,16,1,19,14,15,14,15,13,14,0,
15,16,9,0,15,9,13,0,12,18,0,4,99,111,100,101,
0,0,0,0,13,17,18,0,15,18,1,0,15,19,7,0,
15,20,16,0,15,21,9,0,31,13,18,4,19,13,17,13,
35,13,7,16,21,13,0,0,18,0,0,10,12,19,0,8,
102,114,101,101,95,116,109,112,0,0,0,0,13,18,19,0,
15,19,16,0,31,13,19,1,19,13,18,13,18,0,0,1,
12,21,0,8,102,114,101,101,95,116,109,112,0,0,0,0,
13,20,21,0,15,21,9,0,31,19,21,1,19,19,20,19,
20,7,0,0,0,0,0,0,12,77,0,5,105,110,102,105,
120,0,0,0,14,77,75,0,16,77,0,123,44,28,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
28,6,0,0,9,5,0,6,28,8,0,0,9,7,0,8,
28,9,0,0,28,10,0,0,32,9,0,10,12,13,0,7,
103,101,116,95,116,109,112,0,13,12,13,0,15,13,9,0,
31,11,13,1,19,11,12,11,15,9,11,0,12,15,0,7,
103,101,116,95,116,109,112,0,13,14,15,0,31,13,0,0,
19,13,14,13,15,11,13,0,12,16,0,9,100,111,95,110,
117,109,98,101,114,0,0,0,13,15,16,0,12,17,0,3,
118,97,108,0,15,18,1,0,26,16,17,2,31,13,16,1,
19,13,15,13,15,1,13,0,12,18,0,7,103,101,116,95,
116,97,103,0,13,17,18,0,31,16,0,0,19,16,17,16,
15,13,16,0,12,19,0,2,100,111,0,0,13,18,19,0,
15,19,5,0,15,20,9,0,31,16,19,2,19,16,18,16,
15,9,16,0,12,20,0,4,99,111,100,101,0,0,0,0,
13,19,20,0,12,24,0,2,69,81,0,0,13,20,24,0,
15,21,11,0,15,22,9,0,15,23,1,0,31,16,20,4,
19,16,19,16,12,21,0,4,99,111,100,101,0,0,0,0,
13,20,21,0,12,23,0,2,73,70,0,0,13,21,23,0,
15,22,11,0,31,16,21,2,19,16,20,16,12,22,0,4,
106,117,109,112,0,0,0,0,13,21,22,0,15,22,13,0,
12,23,0,4,101,108,115,101,0,0,0,0,31,16,22,2,
19,16,21,16,12,23,0,4,106,117,109,112,0,0,0,0,
13,22,23,0,15,23,13,0,12,24,0,3,101,110,100,0,
31,16,23,2,19,16,22,16,12,24,0,3,116,97,103,0,
13,23,24,0,15,24,13,0,12,25,0,4,101,108,115,101,
0,0,0,0,31,16,24,2,19,16,23,16,12,25,0,2,
100,111,0,0,13,24,25,0,15,25,7,0,15,26,9,0,
31,16,25,2,19,16,24,16,15,9,16,0,12,26,0,3,
116,97,103,0,13,25,26,0,15,26,13,0,12,27,0,3,
101,110,100,0,31,16,26,2,19,16,25,16,20,9,0,0,
0,0,0,0,12,79,0,8,115,115,95,105,110,102,105,120,
0,0,0,0,14,79,77,0,16,79,2,95,44,44,0,0,
28,2,0,0,9,1,0,2,28,3,0,0,28,4,0,0,
32,3,0,4,12,7,0,1,61,0,0,0,27,6,7,1,
15,5,6,0,12,8,0,2,43,61,0,0,12,9,0,2,
45,61,0,0,12,10,0,2,42,61,0,0,12,11,0,2,
47,61,0,0,27,7,8,4,15,6,7,0,12,9,0,1,
60,0,0,0,12,10,0,1,62,0,0,0,12,11,0,2,
60,61,0,0,12,12,0,2,62,61,0,0,12,13,0,2,
61,61,0,0,12,14,0,2,33,61,0,0,27,8,9,6,
15,7,8,0,12,10,0,1,43,0,0,0,12,34,0,3,
65,68,68,0,13,11,34,0,12,12,0,1,42,0,0,0,
12,34,0,3,77,85,76,0,13,13,34,0,12,14,0,1,
47,0,0,0,12,34,0,3,68,73,86,0,13,15,34,0,
12,16,0,2,42,42,0,0,12,34,0,3,80,79,87,0,
13,17,34,0,12,18,0,1,45,0,0,0,12,34,0,3,
83,85,66,0,13,19,34,0,12,20,0,3,97,110,100,0,
12,34,0,3,65,78,68,0,13,21,34,0,12,22,0,2,
111,114,0,0,12,34,0,2,79,82,0,0,13,23,34,0,
12,24,0,1,37,0,0,0,12,34,0,3,77,79,68,0,
13,25,34,0,12,26,0,2,62,62,0,0,12,34,0,3,
82,83,72,0,13,27,34,0,12,28,0,2,60,60,0,0,
12,34,0,3,76,83,72,0,13,29,34,0,12,30,0,1,
38,0,0,0,12,34,0,3,65,78,68,0,13,31,34,0,
12,32,0,1,124,0,0,0,12,34,0,2,79,82,0,0,
13,33,34,0,26,9,10,24,15,8,9,0,12,10,0,3,
118,97,108,0,9,9,1,10,12,10,0,4,78,111,110,101,
0,0,0,0,23,9,9,10,21,9,0,0,18,0,0,22,
12,11,0,7,103,101,116,95,116,109,112,0,13,10,11,0,
15,11,3,0,31,9,11,1,19,9,10,9,15,3,9,0,
12,12,0,4,99,111,100,101,0,0,0,0,13,11,12,0,
12,14,0,4,78,79,78,69,0,0,0,0,13,12,14,0,
15,13,3,0,31,9,12,2,19,9,11,9,20,3,0,0,
18,0,0,1,12,12,0,3,118,97,108,0,9,9,1,12,
12,12,0,4,84,114,117,101,0,0,0,0,23,9,9,12,
21,9,0,0,18,0,0,24,12,13,0,9,100,111,95,110,
117,109,98,101,114,0,0,0,13,12,13,0,12,15,0,4,
102,114,111,109,0,0,0,0,12,19,0,4,102,114,111,109,
0,0,0,0,9,16,1,19,12,17,0,3,118,97,108,0,
11,18,0,0,0,0,0,0,0,0,240,63,26,13,15,4,
15,14,3,0,31,9,13,2,19,9,12,9,20,9,0,0,
18,0,0,1,12,13,0,3,118,97,108,0,9,9,1,13,
12,13,0,5,70,97,108,115,101,0,0,0,23,9,9,13,
21,9,0,0,18,0,0,24,12,14,0,9,100,111,95,110,
117,109,98,101,114,0,0,0,13,13,14,0,12,16,0,4,
102,114,111,109,0,0,0,0,12,20,0,4,102,114,111,109,
0,0,0,0,9,17,1,20,12,18,0,3,118,97,108,0,
11,19,0,0,0,0,0,0,0,0,0,0,26,14,16,4,
15,15,3,0,31,9,14,2,19,9,13,9,20,9,0,0,
18,0,0,1,12,15,0,5,105,116,101,109,115,0,0,0,
9,14,1,15,15,9,14,0,12,15,0,3,97,110,100,0,
12,16,0,2,111,114,0,0,27,14,15,2,12,16,0,3,
118,97,108,0,9,15,1,16,36,14,14,15,21,14,0,0,
18,0,0,36,12,17,0,3,105,110,116,0,13,16,17,0,
12,18,0,3,118,97,108,0,9,17,1,18,12,18,0,2,
111,114,0,0,23,17,17,18,31,15,17,1,19,15,16,15,
15,14,15,0,12,18,0,8,115,115,95,105,110,102,105,120,
0,0,0,0,13,17,18,0,15,18,14,0,12,24,0,3,
118,97,108,0,9,23,1,24,9,19,8,23,11,23,0,0,
0,0,0,0,0,0,0,0,9,20,9,23,11,23,0,0,
0,0,0,0,0,0,240,63,9,21,9,23,15,22,3,0,
31,15,18,5,19,15,17,15,20,15,0,0,18,0,0,1,
12,19,0,3,118,97,108,0,9,18,1,19,36,15,6,18,
21,15,0,0,18,0,0,15,12,19,0,7,105,109,97,110,
97,103,101,0,13,18,19,0,15,19,1,0,12,21,0,9,
100,111,95,115,121,109,98,111,108,0,0,0,13,20,21,0,
31,15,19,2,19,15,18,15,20,15,0,0,18,0,0,1,
12,19,0,3,118,97,108,0,9,15,1,19,12,19,0,2,
105,115,0,0,23,15,15,19,21,15,0,0,18,0,0,21,
12,20,0,5,105,110,102,105,120,0,0,0,13,19,20,0,
12,24,0,2,69,81,0,0,13,20,24,0,11,24,0,0,
0,0,0,0,0,0,0,0,9,21,9,24,11,24,0,0,
0,0,0,0,0,0,240,63,9,22,9,24,15,23,3,0,
31,15,20,4,19,15,19,15,20,15,0,0,18,0,0,1,
12,20,0,3,118,97,108,0,9,15,1,20,12,20,0,5,
105,115,110,111,116,0,0,0,23,15,15,20,21,15,0,0,
18,0,0,21,12,21,0,5,105,110,102,105,120,0,0,0,
13,20,21,0,12,25,0,3,67,77,80,0,13,21,25,0,
11,25,0,0,0,0,0,0,0,0,0,0,9,22,9,25,
11,25,0,0,0,0,0,0,0,0,240,63,9,23,9,25,
15,24,3,0,31,15,21,4,19,15,20,15,20,15,0,0,
18,0,0,1,12,21,0,3,118,97,108,0,9,15,1,21,
12,21,0,3,110,111,116,0,23,15,15,21,21,15,0,0,
18,0,0,36,12,22,0,5,105,110,102,105,120,0,0,0,
13,21,22,0,12,26,0,2,69,81,0,0,13,22,26,0,
12,26,0,4,102,114,111,109,0,0,0,0,12,32,0,4,
102,114,111,109,0,0,0,0,9,27,1,32,12,28,0,4,
116,121,112,101,0,0,0,0,12,29,0,6,110,117,109,98,
101,114,0,0,12,30,0,3,118,97,108,0,11,31,0,0,
0,0,0,0,0,0,0,0,26,23,26,6,11,26,0,0,
0,0,0,0,0,0,0,0,9,24,9,26,15,25,3,0,
31,15,22,4,19,15,21,15,20,15,0,0,18,0,0,1,
12,22,0,3,118,97,108,0,9,15,1,22,12,22,0,2,
105,110,0,0,23,15,15,22,21,15,0,0,18,0,0,21,
12,23,0,5,105,110,102,105,120,0,0,0,13,22,23,0,
12,27,0,3,72,65,83,0,13,23,27,0,11,27,0,0,
0,0,0,0,0,0,240,63,9,24,9,27,11,27,0,0,
0,0,0,0,0,0,0,0,9,25,9,27,15,26,3,0,
31,15,23,4,19,15,22,15,20,15,0,0,18,0,0,1,
12,23,0,3,118,97,108,0,9,15,1,23,12,23,0,5,
110,111,116,105,110,0,0,0,23,15,15,23,21,15,0,0,
18,0,0,68,12,24,0,5,105,110,102,105,120,0,0,0,
13,23,24,0,12,28,0,3,72,65,83,0,13,24,28,0,
11,28,0,0,0,0,0,0,0,0,240,63,9,25,9,28,
11,28,0,0,0,0,0,0,0,0,0,0,9,26,9,28,
15,27,3,0,31,15,24,4,19,15,23,15,15,3,15,0,
12,26,0,9,100,111,95,110,117,109,98,101,114,0,0,0,
13,25,26,0,12,27,0,4,102,114,111,109,0,0,0,0,
12,33,0,4,102,114,111,109,0,0,0,0,9,28,1,33,
12,29,0,4,116,121,112,101,0,0,0,0,12,30,0,6,
110,117,109,98,101,114,0,0,12,31,0,3,118,97,108,0,
11,32,0,0,0,0,0,0,0,0,0,0,26,26,27,6,
31,24,26,1,19,24,25,24,15,15,24,0,12,27,0,4,
99,111,100,101,0,0,0,0,13,26,27,0,12,31,0,2,
69,81,0,0,13,27,31,0,15,28,3,0,15,29,3,0,
12,32,0,8,102,114,101,101,95,116,109,112,0,0,0,0,
13,31,32,0,15,32,15,0,31,30,32,1,19,30,31,30,
31,24,27,4,19,24,26,24,20,3,0,0,18,0,0,1,
12,28,0,3,118,97,108,0,9,27,1,28,36,24,5,27,
21,24,0,0,18,0,0,18,12,28,0,10,100,111,95,115,
101,116,95,99,116,120,0,0,13,27,28,0,11,30,0,0,
0,0,0,0,0,0,0,0,9,28,9,30,11,30,0,0,
0,0,0,0,0,0,240,63,9,29,9,30,31,24,28,2,
19,24,27,24,20,24,0,0,18,0,0,118,12,29,0,3,
118,97,108,0,9,28,1,29,36,24,7,28,21,24,0,0,
18,0,0,91,11,29,0,0,0,0,0,0,0,0,0,0,
9,28,9,29,15,24,28,0,11,32,0,0,0,0,0,0,
0,0,240,63,9,30,9,32,15,29,30,0,15,32,24,0,
15,24,29,0,12,34,0,3,118,97,108,0,9,33,1,34,
15,29,33,0,12,34,0,1,62,0,0,0,12,35,0,2,
62,61,0,0,27,33,34,2,11,35,0,0,0,0,0,0,
0,0,0,0,9,34,29,35,36,33,33,34,21,33,0,0,
18,0,0,17,15,33,24,0,15,34,32,0,12,36,0,1,
60,0,0,0,11,39,0,0,0,0,0,0,0,0,240,63,
28,40,0,0,27,38,39,2,9,37,29,38,1,36,36,37,
15,35,36,0,15,32,33,0,15,24,34,0,15,29,35,0,
18,0,0,1,12,35,0,2,69,81,0,0,13,34,35,0,
15,33,34,0,12,35,0,1,60,0,0,0,23,34,29,35,
21,34,0,0,18,0,0,6,12,35,0,2,76,84,0,0,
13,34,35,0,15,33,34,0,18,0,0,1,12,35,0,2,
60,61,0,0,23,34,29,35,21,34,0,0,18,0,0,6,
12,35,0,2,76,69,0,0,13,34,35,0,15,33,34,0,
18,0,0,1,12,35,0,2,33,61,0,0,23,34,29,35,
21,34,0,0,18,0,0,6,12,35,0,2,78,69,0,0,
13,34,35,0,15,33,34,0,18,0,0,1,12,37,0,5,
105,110,102,105,120,0,0,0,13,35,37,0,15,37,33,0,
15,38,32,0,15,39,24,0,15,40,3,0,31,34,37,4,
19,34,35,34,20,34,0,0,18,0,0,22,12,38,0,5,
105,110,102,105,120,0,0,0,13,37,38,0,12,43,0,3,
118,97,108,0,9,42,1,43,9,38,8,42,11,42,0,0,
0,0,0,0,0,0,0,0,9,39,9,42,11,42,0,0,
0,0,0,0,0,0,240,63,9,40,9,42,15,41,3,0,
31,34,38,4,19,34,37,34,20,34,0,0,18,0,0,1,
0,0,0,0,12,81,0,9,100,111,95,115,121,109,98,111,
108,0,0,0,14,81,79,0,16,81,1,195,44,63,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
12,6,0,4,116,121,112,101,0,0,0,0,9,5,1,6,
12,6,0,4,110,97,109,101,0,0,0,0,23,5,5,6,
21,5,0,0,18,0,0,136,11,7,0,0,0,0,0,0,
0,0,240,63,11,9,0,0,0,0,0,0,0,0,0,0,
12,10,0,1,68,0,0,0,13,5,10,0,12,10,0,8,
95,103,108,111,98,97,108,115,0,0,0,0,9,5,5,10,
23,8,5,9,21,8,0,0,18,0,0,2,18,0,0,16,
12,10,0,1,68,0,0,0,13,5,10,0,12,10,0,4,
118,97,114,115,0,0,0,0,9,5,5,10,12,11,0,3,
118,97,108,0,9,10,1,11,36,5,5,10,11,10,0,0,
0,0,0,0,0,0,0,0,23,5,5,10,23,6,5,7,
21,6,0,0,18,0,0,2,18,0,0,12,12,10,0,1,
68,0,0,0,13,5,10,0,12,10,0,7,103,108,111,98,
97,108,115,0,9,5,5,10,12,11,0,3,118,97,108,0,
9,10,1,11,36,5,5,10,21,5,0,0,18,0,0,48,
12,12,0,9,100,111,95,115,116,114,105,110,103,0,0,0,
13,11,12,0,15,12,1,0,31,10,12,1,19,10,11,10,
15,5,10,0,12,14,0,2,100,111,0,0,13,13,14,0,
15,14,3,0,31,12,14,1,19,12,13,12,15,10,12,0,
12,15,0,4,99,111,100,101,0,0,0,0,13,14,15,0,
12,18,0,4,71,83,69,84,0,0,0,0,13,15,18,0,
15,16,5,0,15,17,10,0,31,12,15,3,19,12,14,12,
12,16,0,8,102,114,101,101,95,116,109,112,0,0,0,0,
13,15,16,0,15,16,5,0,31,12,16,1,19,12,15,12,
12,17,0,8,102,114,101,101,95,116,109,112,0,0,0,0,
13,16,17,0,15,17,10,0,31,12,17,1,19,12,16,12,
28,12,0,0,20,12,0,0,18,0,0,1,12,19,0,8,
100,111,95,108,111,99,97,108,0,0,0,0,13,18,19,0,
15,19,1,0,31,17,19,1,19,17,18,17,15,12,17,0,
12,20,0,2,100,111,0,0,13,19,20,0,15,20,3,0,
31,17,20,1,19,17,19,17,15,10,17,0,12,21,0,4,
99,111,100,101,0,0,0,0,13,20,21,0,12,24,0,4,
77,79,86,69,0,0,0,0,13,21,24,0,15,22,12,0,
15,23,10,0,31,17,21,3,19,17,20,17,12,22,0,8,
102,114,101,101,95,116,109,112,0,0,0,0,13,21,22,0,
15,22,10,0,31,17,22,1,19,17,21,17,20,12,0,0,
18,0,0,254,12,22,0,5,116,117,112,108,101,0,0,0,
12,23,0,4,108,105,115,116,0,0,0,0,27,17,22,2,
12,23,0,4,116,121,112,101,0,0,0,0,9,22,1,23,
36,17,17,22,21,17,0,0,18,0,0,240,12,22,0,5,
116,117,112,108,101,0,0,0,12,23,0,4,108,105,115,116,
0,0,0,0,27,17,22,2,12,23,0,4,116,121,112,101,
0,0,0,0,9,22,3,23,36,17,17,22,21,17,0,0,
18,0,0,115,11,22,0,0,0,0,0,0,0,0,0,0,
15,17,22,0,27,24,0,0,15,23,24,0,15,25,17,0,
15,17,23,0,12,27,0,5,105,116,101,109,115,0,0,0,
9,26,1,27,11,27,0,0,0,0,0,0,0,0,0,0,
42,23,26,27,18,0,0,44,12,30,0,5,105,116,101,109,
115,0,0,0,9,29,3,30,9,29,29,25,15,28,29,0,
12,32,0,7,103,101,116,95,116,109,112,0,13,31,32,0,
31,30,0,0,19,30,31,30,15,29,30,0,12,33,0,6,
97,112,112,101,110,100,0,0,9,32,17,33,15,33,29,0,
31,30,33,1,19,30,32,30,12,34,0,4,99,111,100,101,
0,0,0,0,13,33,34,0,12,37,0,4,77,79,86,69,
0,0,0,0,13,34,37,0,15,35,29,0,12,38,0,2,
100,111,0,0,13,37,38,0,15,38,28,0,31,36,38,1,
19,36,37,36,31,30,34,3,19,30,33,30,11,34,0,0,
0,0,0,0,0,0,240,63,1,30,25,34,15,25,30,0,
18,0,255,212,11,27,0,0,0,0,0,0,0,0,0,0,
15,25,27,0,12,30,0,5,105,116,101,109,115,0,0,0,
9,27,1,30,11,30,0,0,0,0,0,0,0,0,0,0,
42,23,27,30,18,0,0,39,12,35,0,5,105,116,101,109,
115,0,0,0,9,34,3,35,9,34,34,25,15,28,34,0,
9,34,17,25,15,29,34,0,12,36,0,10,100,111,95,115,
101,116,95,99,116,120,0,0,13,35,36,0,15,38,23,0,
12,40,0,4,102,114,111,109,0,0,0,0,12,36,0,4,
102,114,111,109,0,0,0,0,9,41,28,36,12,42,0,4,
116,121,112,101,0,0,0,0,12,43,0,3,114,101,103,0,
12,44,0,3,118,97,108,0,15,45,29,0,26,39,40,6,
31,34,38,2,19,34,35,34,11,36,0,0,0,0,0,0,
0,0,240,63,1,34,25,36,15,25,34,0,18,0,255,217,
28,30,0,0,20,30,0,0,18,0,0,1,12,38,0,2,
100,111,0,0,13,36,38,0,15,38,3,0,31,34,38,1,
19,34,36,34,15,30,34,0,12,39,0,6,117,110,95,116,
109,112,0,0,13,38,39,0,15,39,30,0,31,34,39,1,
19,34,38,34,11,39,0,0,0,0,0,0,0,0,0,0,
15,34,39,0,12,42,0,4,102,114,111,109,0,0,0,0,
12,48,0,4,102,114,111,109,0,0,0,0,9,43,3,48,
12,44,0,4,116,121,112,101,0,0,0,0,12,45,0,3,
114,101,103,0,12,46,0,3,118,97,108,0,15,47,30,0,
26,41,42,6,15,40,41,0,15,25,34,0,15,29,40,0,
12,42,0,5,105,116,101,109,115,0,0,0,9,40,1,42,
11,42,0,0,0,0,0,0,0,0,0,0,42,34,40,42,
18,0,0,55,12,45,0,10,100,111,95,115,101,116,95,99,
116,120,0,0,13,44,45,0,15,45,34,0,12,47,0,4,
102,114,111,109,0,0,0,0,12,53,0,4,102,114,111,109,
0,0,0,0,9,48,29,53,12,49,0,4,116,121,112,101,
0,0,0,0,12,50,0,3,103,101,116,0,12,51,0,5,
105,116,101,109,115,0,0,0,15,53,29,0,12,55,0,4,
102,114,111,109,0,0,0,0,12,61,0,4,102,114,111,109,
0,0,0,0,9,56,29,61,12,57,0,4,116,121,112,101,
0,0,0,0,12,58,0,6,110,117,109,98,101,114,0,0,
12,59,0,3,118,97,108,0,12,62,0,3,115,116,114,0,
13,61,62,0,15,62,25,0,31,60,62,1,19,60,61,60,
26,54,55,6,27,52,53,2,26,46,47,6,31,43,45,2,
19,43,44,43,11,45,0,0,0,0,0,0,0,0,240,63,
1,43,25,45,15,25,43,0,18,0,255,201,12,45,0,8,
102,114,101,101,95,114,101,103,0,0,0,0,13,43,45,0,
15,45,30,0,31,42,45,1,19,42,43,42,28,42,0,0,
20,42,0,0,18,0,0,1,12,46,0,2,100,111,0,0,
13,45,46,0,12,47,0,5,105,116,101,109,115,0,0,0,
9,46,1,47,11,47,0,0,0,0,0,0,0,0,0,0,
9,46,46,47,31,42,46,1,19,42,45,42,15,30,42,0,
12,48,0,2,100,111,0,0,13,47,48,0,15,48,3,0,
31,46,48,1,19,46,47,46,15,42,46,0,12,49,0,4,
99,111,100,101,0,0,0,0,13,48,49,0,12,53,0,3,
83,69,84,0,13,49,53,0,15,50,30,0,12,54,0,2,
100,111,0,0,13,53,54,0,12,55,0,5,105,116,101,109,
115,0,0,0,9,54,1,55,11,55,0,0,0,0,0,0,
0,0,240,63,9,54,54,55,31,51,54,1,19,51,53,51,
15,52,42,0,31,46,49,4,19,46,48,46,20,42,0,0,
0,0,0,0,12,83,0,10,100,111,95,115,101,116,95,99,
116,120,0,0,14,83,81,0,16,83,0,152,44,34,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
28,6,0,0,9,5,0,6,11,7,0,0,0,0,0,0,
0,0,0,0,28,8,0,0,32,7,0,8,12,12,0,3,
109,97,120,0,13,11,12,0,15,12,7,0,12,15,0,3,
108,101,110,0,13,14,15,0,15,15,5,0,31,13,15,1,
19,13,14,13,31,10,12,2,19,10,11,10,15,9,10,0,
11,12,0,0,0,0,0,0,0,0,0,0,15,10,12,0,
12,17,0,8,103,101,116,95,116,109,112,115,0,0,0,0,
13,16,17,0,15,17,9,0,31,15,17,1,19,15,16,15,
15,13,15,0,15,17,10,0,15,10,13,0,11,18,0,0,
0,0,0,0,0,0,0,0,42,13,5,18,18,0,0,41,
9,20,10,17,15,19,20,0,12,23,0,2,100,111,0,0,
13,22,23,0,15,23,13,0,15,24,19,0,31,21,23,2,
19,21,22,21,15,20,21,0,35,21,19,20,21,21,0,0,
18,0,0,22,12,24,0,4,99,111,100,101,0,0,0,0,
13,23,24,0,12,27,0,4,77,79,86,69,0,0,0,0,
13,24,27,0,15,25,19,0,15,26,20,0,31,21,24,3,
19,21,23,21,12,25,0,8,102,114,101,101,95,116,109,112,
0,0,0,0,13,24,25,0,15,25,20,0,31,21,25,1,
19,21,24,21,18,0,0,1,11,25,0,0,0,0,0,0,
0,0,240,63,1,21,17,25,15,17,21,0,18,0,255,215,
11,18,0,0,0,0,0,0,0,0,0,0,12,26,0,3,
108,101,110,0,13,25,26,0,15,26,10,0,31,21,26,1,
19,21,25,21,23,18,18,21,21,18,0,0,18,0,0,20,
12,26,0,4,99,111,100,101,0,0,0,0,13,21,26,0,
15,26,1,0,15,27,3,0,11,28,0,0,0,0,0,0,
0,0,0,0,11,29,0,0,0,0,0,0,0,0,0,0,
31,18,26,4,19,18,21,18,11,18,0,0,0,0,0,0,
0,0,0,0,20,18,0,0,18,0,0,1,12,27,0,4,
99,111,100,101,0,0,0,0,13,26,27,0,15,27,1,0,
15,28,3,0,11,31,0,0,0,0,0,0,0,0,0,0,
9,29,10,31,12,32,0,3,108,101,110,0,13,31,32,0,
15,32,5,0,31,30,32,1,19,30,31,30,31,18,27,4,
19,18,26,18,12,28,0,9,102,114,101,101,95,116,109,112,
115,0,0,0,13,27,28,0,15,32,7,0,28,33,0,0,
27,29,32,2,9,28,10,29,31,18,28,1,19,18,27,18,
11,28,0,0,0,0,0,0,0,0,0,0,9,18,10,28,
20,18,0,0,0,0,0,0,12,85,0,10,109,97,110,97,
103,101,95,115,101,113,0,0,14,85,83,0,16,85,0,92,
44,23,0,0,28,2,0,0,9,1,0,2,27,4,0,0,
15,3,4,0,27,6,0,0,15,5,6,0,28,8,0,0,
15,7,8,0,28,10,0,0,15,9,10,0,15,11,3,0,
15,3,5,0,15,5,7,0,15,7,9,0,11,12,0,0,
0,0,0,0,0,0,0,0,42,9,1,12,18,0,0,65,
11,15,0,0,0,0,0,0,0,0,0,0,12,16,0,4,
116,121,112,101,0,0,0,0,9,13,9,16,12,16,0,6,
115,121,109,98,111,108,0,0,23,13,13,16,23,14,13,15,
21,14,0,0,18,0,0,2,18,0,0,7,12,16,0,3,
118,97,108,0,9,13,9,16,12,16,0,1,61,0,0,0,
23,13,13,16,21,13,0,0,18,0,0,9,12,17,0,6,
97,112,112,101,110,100,0,0,9,16,3,17,15,17,9,0,
31,13,17,1,19,13,16,13,18,0,0,33,12,18,0,4,
116,121,112,101,0,0,0,0,9,17,9,18,12,18,0,4,
97,114,103,115,0,0,0,0,23,17,17,18,21,17,0,0,
18,0,0,3,15,5,9,0,18,0,0,21,12,18,0,4,
116,121,112,101,0,0,0,0,9,17,9,18,12,18,0,5,
110,97,114,103,115,0,0,0,23,17,17,18,21,17,0,0,
18,0,0,3,15,7,9,0,18,0,0,9,12,19,0,6,
97,112,112,101,110,100,0,0,9,18,11,19,15,19,9,0,
31,17,19,1,19,17,18,17,18,0,0,1,18,0,255,191,
15,19,11,0,15,20,3,0,15,21,5,0,15,22,7,0,
27,12,19,4,20,12,0,0,0,0,0,0,12,87,0,8,
112,95,102,105,108,116,101,114,0,0,0,0,14,87,85,0,
16,87,0,85,44,24,0,0,28,2,0,0,9,1,0,2,
12,5,0,5,105,116,101,109,115,0,0,0,9,4,1,5,
11,5,0,0,0,0,0,0,0,0,0,0,42,3,4,5,
18,0,0,72,12,6,0,6,115,116,114,105,110,103,0,0,
12,7,0,4,116,121,112,101,0,0,0,0,10,3,7,6,
12,10,0,7,100,111,95,99,97,108,108,0,13,9,10,0,
12,11,0,4,102,114,111,109,0,0,0,0,12,15,0,4,
102,114,111,109,0,0,0,0,9,12,1,15,12,13,0,5,
105,116,101,109,115,0,0,0,12,17,0,4,102,114,111,109,
0,0,0,0,12,23,0,4,102,114,111,109,0,0,0,0,
9,18,1,23,12,19,0,4,116,121,112,101,0,0,0,0,
12,20,0,4,110,97,109,101,0,0,0,0,12,21,0,3,
118,97,108,0,12,22,0,6,105,109,112,111,114,116,0,0,
26,15,17,6,15,16,3,0,27,14,15,2,26,10,11,4,
31,8,10,1,19,8,9,8,15,6,8,0,12,8,0,4,
110,97,109,101,0,0,0,0,12,10,0,4,116,121,112,101,
0,0,0,0,10,3,10,8,12,12,0,10,100,111,95,115,
101,116,95,99,116,120,0,0,13,11,12,0,15,12,3,0,
12,14,0,4,116,121,112,101,0,0,0,0,12,15,0,3,
114,101,103,0,12,16,0,3,118,97,108,0,15,17,6,0,
26,13,14,4,31,8,12,2,19,8,11,8,18,0,255,184,
0,0,0,0,12,89,0,9,100,111,95,105,109,112,111,114,
116,0,0,0,14,89,87,0,16,89,0,49,44,9,0,0,
28,2,0,0,9,1,0,2,12,4,0,5,105,116,101,109,
115,0,0,0,9,3,1,4,11,4,0,0,0,0,0,0,
0,0,0,0,42,1,3,4,18,0,0,36,12,6,0,1,
68,0,0,0,13,5,6,0,12,6,0,7,103,108,111,98,
97,108,115,0,9,5,5,6,12,7,0,3,118,97,108,0,
9,6,1,7,36,5,5,6,11,6,0,0,0,0,0,0,
0,0,0,0,23,5,5,6,21,5,0,0,18,0,0,18,
12,7,0,1,68,0,0,0,13,6,7,0,12,7,0,7,
103,108,111,98,97,108,115,0,9,6,6,7,12,7,0,6,
97,112,112,101,110,100,0,0,9,6,6,7,12,8,0,3,
118,97,108,0,9,7,1,8,31,5,7,1,19,5,6,5,
18,0,0,1,18,0,255,220,0,0,0,0,12,91,0,10,
100,111,95,103,108,111,98,97,108,115,0,0,14,91,89,0,
16,91,0,52,44,16,0,0,28,2,0,0,9,1,0,2,
12,5,0,5,105,116,101,109,115,0,0,0,9,4,1,5,
11,5,0,0,0,0,0,0,0,0,0,0,42,3,4,5,
18,0,0,39,12,9,0,2,100,111,0,0,13,8,9,0,
12,10,0,5,105,116,101,109,115,0,0,0,9,9,3,10,
11,10,0,0,0,0,0,0,0,0,0,0,9,9,9,10,
31,7,9,1,19,7,8,7,15,6,7,0,12,10,0,4,
99,111,100,101,0,0,0,0,13,9,10,0,12,13,0,3,
68,69,76,0,13,10,13,0,15,11,6,0,12,14,0,2,
100,111,0,0,13,13,14,0,12,15,0,5,105,116,101,109,
115,0,0,0,9,14,3,15,11,15,0,0,0,0,0,0,
0,0,240,63,9,14,14,15,31,12,14,1,19,12,13,12,
31,7,10,3,19,7,9,7,18,0,255,217,0,0,0,0,
12,93,0,6,100,111,95,100,101,108,0,0,14,93,91,0,
16,93,1,36,44,42,0,0,28,2,0,0,9,1,0,2,
28,3,0,0,28,4,0,0,32,3,0,4,12,7,0,7,
103,101,116,95,116,109,112,0,13,6,7,0,15,7,3,0,
31,5,7,1,19,5,6,5,15,3,5,0,12,8,0,5,
105,116,101,109,115,0,0,0,9,7,1,8,15,5,7,0,
12,10,0,2,100,111,0,0,13,9,10,0,11,11,0,0,
0,0,0,0,0,0,0,0,9,10,5,11,31,8,10,1,
19,8,9,8,15,7,8,0,12,11,0,8,112,95,102,105,
108,116,101,114,0,0,0,0,13,10,11,0,12,12,0,5,
105,116,101,109,115,0,0,0,9,11,1,12,11,13,0,0,
0,0,0,0,0,0,240,63,28,14,0,0,27,12,13,2,
9,11,11,12,31,8,11,1,19,8,10,8,11,13,0,0,
0,0,0,0,0,0,0,0,9,12,8,13,15,11,12,0,
11,14,0,0,0,0,0,0,0,0,240,63,9,13,8,14,
15,12,13,0,11,15,0,0,0,0,0,0,0,0,0,64,
9,14,8,15,15,13,14,0,11,16,0,0,0,0,0,0,
0,0,8,64,9,15,8,16,15,14,15,0,28,15,0,0,
15,8,15,0,11,17,0,0,0,0,0,0,0,0,240,63,
12,19,0,3,108,101,110,0,13,18,19,0,15,19,12,0,
31,15,19,1,19,15,18,15,11,19,0,0,0,0,0,0,
0,0,0,0,35,15,15,19,23,16,15,17,21,16,0,0,
18,0,0,2,18,0,0,3,28,19,0,0,35,15,14,19,
21,15,0,0,18,0,0,106,12,20,0,7,100,111,95,100,
105,99,116,0,13,19,20,0,12,21,0,5,105,116,101,109,
115,0,0,0,27,22,0,0,26,20,21,2,31,15,20,1,
19,15,19,15,15,8,15,0,12,21,0,6,117,110,95,116,
109,112,0,0,13,20,21,0,15,21,8,0,31,15,21,1,
19,15,20,15,11,21,0,0,0,0,0,0,0,0,0,0,
42,15,12,21,18,0,0,38,12,24,0,4,99,111,100,101,
0,0,0,0,13,23,24,0,12,28,0,3,83,69,84,0,
13,24,28,0,15,25,8,0,12,29,0,2,100,111,0,0,
13,28,29,0,12,30,0,5,105,116,101,109,115,0,0,0,
9,29,15,30,11,30,0,0,0,0,0,0,0,0,0,0,
9,29,29,30,31,26,29,1,19,26,28,26,12,30,0,2,
100,111,0,0,13,29,30,0,12,31,0,5,105,116,101,109,
115,0,0,0,9,30,15,31,11,31,0,0,0,0,0,0,
0,0,240,63,9,30,30,31,31,27,30,1,19,27,29,27,
31,22,24,4,19,22,23,22,18,0,255,218,21,14,0,0,
18,0,0,42,12,25,0,7,100,111,95,99,97,108,108,0,
13,24,25,0,12,26,0,5,105,116,101,109,115,0,0,0,
12,33,0,4,116,121,112,101,0,0,0,0,12,34,0,4,
110,97,109,101,0,0,0,0,12,35,0,3,118,97,108,0,
12,36,0,5,109,101,114,103,101,0,0,0,26,30,33,4,
12,33,0,4,116,121,112,101,0,0,0,0,12,34,0,3,
114,101,103,0,12,35,0,3,118,97,108,0,15,36,8,0,
26,31,33,4,12,33,0,5,105,116,101,109,115,0,0,0,
9,32,14,33,11,33,0,0,0,0,0,0,0,0,0,0,
9,32,32,33,27,27,30,3,26,25,26,2,31,21,25,1,
19,21,24,21,18,0,0,1,18,0,0,1,12,27,0,10,
109,97,110,97,103,101,95,115,101,113,0,0,13,26,27,0,
12,27,0,6,80,65,82,65,77,83,0,0,13,30,27,0,
15,31,3,0,15,32,11,0,31,25,30,3,19,25,26,25,
28,27,0,0,35,25,13,27,21,25,0,0,18,0,0,37,
12,30,0,4,99,111,100,101,0,0,0,0,13,27,30,0,
12,34,0,3,83,69,84,0,13,30,34,0,15,31,3,0,
12,35,0,9,100,111,95,115,116,114,105,110,103,0,0,0,
13,34,35,0,12,36,0,3,118,97,108,0,12,37,0,1,
42,0,0,0,26,35,36,2,31,32,35,1,19,32,34,32,
12,36,0,2,100,111,0,0,13,35,36,0,12,37,0,5,
105,116,101,109,115,0,0,0,9,36,13,37,11,37,0,0,
0,0,0,0,0,0,0,0,9,36,36,37,31,33,36,1,
19,33,35,33,31,25,30,4,19,25,27,25,18,0,0,1,
28,31,0,0,35,30,8,31,21,30,0,0,18,0,0,26,
12,32,0,4,99,111,100,101,0,0,0,0,13,31,32,0,
12,32,0,3,83,69,84,0,13,36,32,0,15,37,3,0,
12,33,0,9,100,111,95,115,121,109,98,111,108,0,0,0,
13,32,33,0,12,40,0,3,118,97,108,0,12,41,0,4,
78,111,110,101,0,0,0,0,26,33,40,2,31,38,33,1,
19,38,32,38,15,39,8,0,31,30,36,4,19,30,31,30,
18,0,0,1,12,37,0,4,99,111,100,101,0,0,0,0,
13,36,37,0,12,41,0,4,67,65,76,76,0,0,0,0,
13,37,41,0,15,38,3,0,15,39,7,0,15,40,3,0,
31,33,37,4,19,33,36,33,20,3,0,0,0,0,0,0,
12,94,0,7,100,111,95,99,97,108,108,0,14,94,93,0,
16,94,0,42,44,10,0,0,28,2,0,0,9,1,0,2,
28,3,0,0,28,4,0,0,32,3,0,4,12,6,0,1,
68,0,0,0,13,5,6,0,12,6,0,4,118,97,114,115,
0,0,0,0,9,5,5,6,12,7,0,3,118,97,108,0,
9,6,1,7,36,5,5,6,21,5,0,0,18,0,0,12,
12,7,0,8,100,111,95,108,111,99,97,108,0,0,0,0,
13,6,7,0,15,7,1,0,15,8,3,0,31,5,7,2,
19,5,6,5,20,5,0,0,18,0,0,1,12,8,0,9,
100,111,95,103,108,111,98,97,108,0,0,0,13,7,8,0,
15,8,1,0,15,9,3,0,31,5,8,2,19,5,7,5,
20,5,0,0,0,0,0,0,12,95,0,7,100,111,95,110,
97,109,101,0,14,95,94,0,16,95,0,52,44,11,0,0,
28,2,0,0,9,1,0,2,28,3,0,0,28,4,0,0,
32,3,0,4,12,6,0,1,68,0,0,0,13,5,6,0,
12,6,0,4,118,97,114,115,0,0,0,0,9,5,5,6,
12,7,0,3,118,97,108,0,9,6,1,7,36,5,5,6,
11,6,0,0,0,0,0,0,0,0,0,0,23,5,5,6,
21,5,0,0,18,0,0,18,12,7,0,1,68,0,0,0,
13,6,7,0,12,7,0,4,118,97,114,115,0,0,0,0,
9,6,6,7,12,7,0,6,97,112,112,101,110,100,0,0,
9,6,6,7,12,8,0,3,118,97,108,0,9,7,1,8,
31,5,7,1,19,5,6,5,18,0,0,1,12,9,0,7,
103,101,116,95,114,101,103,0,13,8,9,0,12,10,0,3,
118,97,108,0,9,9,1,10,31,7,9,1,19,7,8,7,
20,7,0,0,0,0,0,0,12,96,0,8,100,111,95,108,
111,99,97,108,0,0,0,0,14,96,95,0,16,96,0,46,
44,14,0,0,28,2,0,0,9,1,0,2,28,3,0,0,
28,4,0,0,32,3,0,4,12,7,0,7,103,101,116,95,
116,109,112,0,13,6,7,0,15,7,3,0,31,5,7,1,
19,5,6,5,15,3,5,0,12,9,0,9,100,111,95,115,
116,114,105,110,103,0,0,0,13,8,9,0,15,9,1,0,
31,7,9,1,19,7,8,7,15,5,7,0,12,10,0,4,
99,111,100,101,0,0,0,0,13,9,10,0,12,13,0,4,
71,71,69,84,0,0,0,0,13,10,13,0,15,11,3,0,
15,12,5,0,31,7,10,3,19,7,9,7,12,11,0,8,
102,114,101,101,95,116,109,112,0,0,0,0,13,10,11,0,
15,11,5,0,31,7,11,1,19,7,10,7,20,3,0,0,
0,0,0,0,12,97,0,9,100,111,95,103,108,111,98,97,
108,0,0,0,14,97,96,0,16,97,1,170,44,52,0,0,
28,2,0,0,9,1,0,2,28,3,0,0,28,4,0,0,
32,3,0,4,12,7,0,5,105,116,101,109,115,0,0,0,
9,6,1,7,15,5,6,0,12,9,0,7,103,101,116,95,
116,97,103,0,13,8,9,0,31,7,0,0,19,7,8,7,
15,6,7,0,12,11,0,3,102,110,99,0,13,10,11,0,
15,11,6,0,12,12,0,3,101,110,100,0,31,9,11,2,
19,9,10,9,15,7,9,0,12,12,0,1,68,0,0,0,
13,11,12,0,12,12,0,5,98,101,103,105,110,0,0,0,
9,11,11,12,31,9,0,0,19,9,11,9,12,12,0,4,
102,114,111,109,0,0,0,0,36,9,1,12,21,9,0,0,
18,0,0,12,12,13,0,6,115,101,116,112,111,115,0,0,
13,12,13,0,12,14,0,4,102,114,111,109,0,0,0,0,
9,13,1,14,31,9,13,1,19,9,12,9,18,0,0,1,
12,16,0,8,100,111,95,108,111,99,97,108,0,0,0,0,
13,15,16,0,12,17,0,3,118,97,108,0,12,18,0,8,
95,95,112,97,114,97,109,115,0,0,0,0,26,16,17,2,
31,14,16,1,19,14,15,14,15,13,14,0,12,17,0,7,
100,111,95,105,110,102,111,0,13,16,17,0,11,18,0,0,
0,0,0,0,0,0,0,0,9,17,5,18,12,18,0,3,
118,97,108,0,9,17,17,18,31,14,17,1,19,14,16,14,
12,18,0,8,112,95,102,105,108,116,101,114,0,0,0,0,
13,17,18,0,11,19,0,0,0,0,0,0,0,0,240,63,
9,18,5,19,12,19,0,5,105,116,101,109,115,0,0,0,
9,18,18,19,31,14,18,1,19,14,17,14,11,20,0,0,
0,0,0,0,0,0,0,0,9,19,14,20,15,18,19,0,
11,21,0,0,0,0,0,0,0,0,240,63,9,20,14,21,
15,19,20,0,11,22,0,0,0,0,0,0,0,0,0,64,
9,21,14,22,15,20,21,0,11,23,0,0,0,0,0,0,
0,0,8,64,9,22,14,23,15,21,22,0,11,22,0,0,
0,0,0,0,0,0,0,0,42,14,18,22,18,0,0,35,
12,26,0,8,100,111,95,108,111,99,97,108,0,0,0,0,
13,25,26,0,15,26,14,0,31,24,26,1,19,24,25,24,
15,23,24,0,12,27,0,4,99,111,100,101,0,0,0,0,
13,26,27,0,12,31,0,3,71,69,84,0,13,27,31,0,
15,28,23,0,15,29,13,0,12,32,0,9,100,111,95,115,
121,109,98,111,108,0,0,0,13,31,32,0,12,33,0,3,
118,97,108,0,12,34,0,4,78,111,110,101,0,0,0,0,
26,32,33,2,31,30,32,1,19,30,31,30,31,24,27,4,
19,24,26,24,18,0,255,221,11,22,0,0,0,0,0,0,
0,0,0,0,42,14,19,22,18,0,0,57,12,28,0,8,
100,111,95,108,111,99,97,108,0,0,0,0,13,27,28,0,
12,29,0,5,105,116,101,109,115,0,0,0,9,28,14,29,
11,29,0,0,0,0,0,0,0,0,0,0,9,28,28,29,
31,24,28,1,19,24,27,24,15,23,24,0,12,29,0,2,
100,111,0,0,13,28,29,0,12,32,0,5,105,116,101,109,
115,0,0,0,9,29,14,32,11,32,0,0,0,0,0,0,
0,0,240,63,9,29,29,32,15,30,23,0,31,24,29,2,
19,24,28,24,12,30,0,4,99,111,100,101,0,0,0,0,
13,29,30,0,12,30,0,4,73,71,69,84,0,0,0,0,
13,32,30,0,15,33,23,0,15,34,13,0,12,36,0,9,
100,111,95,115,121,109,98,111,108,0,0,0,13,30,36,0,
12,37,0,3,118,97,108,0,12,38,0,4,78,111,110,101,
0,0,0,0,26,36,37,2,31,35,36,1,19,35,30,35,
31,24,32,4,19,24,29,24,18,0,255,199,28,24,0,0,
35,22,20,24,21,22,0,0,18,0,0,41,12,32,0,8,
100,111,95,108,111,99,97,108,0,0,0,0,13,24,32,0,
12,33,0,5,105,116,101,109,115,0,0,0,9,32,20,33,
11,33,0,0,0,0,0,0,0,0,0,0,9,32,32,33,
31,22,32,1,19,22,24,22,15,23,22,0,12,33,0,4,
99,111,100,101,0,0,0,0,13,32,33,0,12,37,0,3,
71,69,84,0,13,33,37,0,15,34,23,0,15,35,13,0,
12,38,0,9,100,111,95,115,116,114,105,110,103,0,0,0,
13,37,38,0,12,39,0,3,118,97,108,0,12,40,0,1,
42,0,0,0,26,38,39,2,31,36,38,1,19,36,37,36,
31,22,33,4,19,22,32,22,18,0,0,1,28,33,0,0,
35,22,21,33,21,22,0,0,18,0,0,42,12,35,0,8,
100,111,95,108,111,99,97,108,0,0,0,0,13,34,35,0,
12,36,0,5,105,116,101,109,115,0,0,0,9,35,21,36,
11,36,0,0,0,0,0,0,0,0,0,0,9,35,35,36,
31,33,35,1,19,33,34,33,15,22,33,0,12,36,0,4,
99,111,100,101,0,0,0,0,13,35,36,0,12,36,0,3,
71,69,84,0,13,38,36,0,15,39,22,0,15,40,13,0,
12,42,0,9,100,111,95,115,121,109,98,111,108,0,0,0,
13,36,42,0,12,43,0,3,118,97,108,0,12,44,0,4,
78,111,110,101,0,0,0,0,26,42,43,2,31,41,42,1,
19,41,36,41,31,33,38,4,19,33,35,33,18,0,0,1,
12,39,0,2,100,111,0,0,13,38,39,0,11,40,0,0,
0,0,0,0,0,0,0,64,9,39,5,40,31,33,39,1,
19,33,38,33,12,40,0,1,68,0,0,0,13,39,40,0,
12,40,0,3,101,110,100,0,9,39,39,40,31,33,0,0,
19,33,39,33,12,41,0,3,116,97,103,0,13,40,41,0,
15,41,6,0,12,42,0,3,101,110,100,0,31,33,41,2,
19,33,40,33,28,41,0,0,23,33,3,41,21,33,0,0,
18,0,0,50,12,41,0,1,68,0,0,0,13,33,41,0,
12,41,0,8,95,103,108,111,98,97,108,115,0,0,0,0,
9,33,33,41,21,33,0,0,18,0,0,18,12,42,0,10,
100,111,95,103,108,111,98,97,108,115,0,0,13,41,42,0,
12,43,0,5,105,116,101,109,115,0,0,0,11,46,0,0,
0,0,0,0,0,0,0,0,9,45,5,46,27,44,45,1,
26,42,43,2,31,33,42,1,19,33,41,33,18,0,0,1,
12,44,0,10,100,111,95,115,101,116,95,99,116,120,0,0,
13,43,44,0,11,46,0,0,0,0,0,0,0,0,0,0,
9,44,5,46,12,46,0,4,116,121,112,101,0,0,0,0,
12,47,0,3,114,101,103,0,12,48,0,3,118,97,108,0,
15,49,7,0,26,45,46,4,31,42,44,2,19,42,43,42,
15,13,42,0,18,0,0,34,12,46,0,9,100,111,95,115,
116,114,105,110,103,0,0,0,13,45,46,0,11,47,0,0,
0,0,0,0,0,0,0,0,9,46,5,47,31,44,46,1,
19,44,45,44,15,42,44,0,12,47,0,4,99,111,100,101,
0,0,0,0,13,46,47,0,12,51,0,3,83,69,84,0,
13,47,51,0,15,48,3,0,15,49,42,0,15,50,7,0,
31,44,47,4,19,44,46,44,12,48,0,8,102,114,101,101,
95,116,109,112,0,0,0,0,13,47,48,0,15,48,42,0,
31,44,48,1,19,44,47,44,18,0,0,1,12,49,0,8,
102,114,101,101,95,116,109,112,0,0,0,0,13,48,49,0,
15,49,7,0,31,44,49,1,19,44,48,44,0,0,0,0,
12,98,0,6,100,111,95,100,101,102,0,0,14,98,97,0,
16,98,2,116,44,52,0,0,28,2,0,0,9,1,0,2,
15,3,1,0,12,6,0,5,105,116,101,109,115,0,0,0,
9,5,1,6,15,4,5,0,28,6,0,0,15,5,6,0,
11,7,0,0,0,0,0,0,0,0,0,0,9,6,4,7,
12,7,0,4,116,121,112,101,0,0,0,0,9,6,6,7,
12,7,0,4,110,97,109,101,0,0,0,0,23,6,6,7,
21,6,0,0,18,0,0,10,11,8,0,0,0,0,0,0,
0,0,0,0,9,7,4,8,12,8,0,3,118,97,108,0,
9,7,7,8,15,6,7,0,18,0,0,34,11,8,0,0,
0,0,0,0,0,0,0,0,9,7,4,8,12,8,0,5,
105,116,101,109,115,0,0,0,9,7,7,8,11,8,0,0,
0,0,0,0,0,0,0,0,9,7,7,8,12,8,0,3,
118,97,108,0,9,7,7,8,15,6,7,0,11,8,0,0,
0,0,0,0,0,0,0,0,9,7,4,8,12,8,0,5,
105,116,101,109,115,0,0,0,9,7,7,8,11,8,0,0,
0,0,0,0,0,0,240,63,9,7,7,8,12,8,0,3,
118,97,108,0,9,7,7,8,15,5,7,0,18,0,0,1,
12,10,0,7,100,111,95,100,105,99,116,0,13,9,10,0,
12,11,0,5,105,116,101,109,115,0,0,0,27,12,0,0,
26,10,11,2,31,8,10,1,19,8,9,8,15,7,8,0,
12,11,0,4,99,111,100,101,0,0,0,0,13,10,11,0,
12,14,0,4,71,83,69,84,0,0,0,0,13,11,14,0,
12,15,0,9,100,111,95,115,116,114,105,110,103,0,0,0,
13,14,15,0,12,16,0,3,118,97,108,0,15,17,6,0,
26,15,16,2,31,12,15,1,19,12,14,12,15,13,7,0,
31,8,11,3,19,8,10,8,11,11,0,0,0,0,0,0,
0,0,0,0,15,8,11,0,27,13,0,0,15,12,13,0,
15,15,8,0,15,8,12,0,21,5,0,0,18,0,0,63,
12,17,0,6,97,112,112,101,110,100,0,0,9,16,8,17,
12,18,0,4,116,121,112,101,0,0,0,0,12,19,0,4,
99,97,108,108,0,0,0,0,12,20,0,5,105,116,101,109,
115,0,0,0,12,24,0,4,116,121,112,101,0,0,0,0,
12,25,0,3,103,101,116,0,12,26,0,5,105,116,101,109,
115,0,0,0,12,30,0,4,116,121,112,101,0,0,0,0,
12,31,0,4,110,97,109,101,0,0,0,0,12,32,0,3,
118,97,108,0,15,33,5,0,26,28,30,4,12,30,0,4,
116,121,112,101,0,0,0,0,12,31,0,6,115,116,114,105,
110,103,0,0,12,32,0,3,118,97,108,0,12,33,0,7,
95,95,110,101,119,95,95,0,26,29,30,4,27,27,28,2,
26,22,24,4,12,24,0,4,116,121,112,101,0,0,0,0,
12,25,0,4,110,97,109,101,0,0,0,0,12,26,0,3,
118,97,108,0,12,27,0,4,115,101,108,102,0,0,0,0,
26,23,24,4,27,21,22,2,26,17,18,4,31,12,17,1,
19,12,16,12,18,0,0,1,11,19,0,0,0,0,0,0,
0,0,240,63,9,18,4,19,12,19,0,5,105,116,101,109,
115,0,0,0,9,18,18,19,11,19,0,0,0,0,0,0,
0,0,0,0,42,17,18,19,18,0,0,163,12,21,0,4,
116,121,112,101,0,0,0,0,9,20,17,21,12,21,0,3,
100,101,102,0,35,20,20,21,21,20,0,0,18,0,0,3,
18,0,255,245,18,0,0,1,12,22,0,5,105,116,101,109,
115,0,0,0,9,21,17,22,11,22,0,0,0,0,0,0,
0,0,0,0,9,21,21,22,12,22,0,3,118,97,108,0,
9,21,21,22,15,20,21,0,12,22,0,8,95,95,105,110,
105,116,95,95,0,0,0,0,23,21,20,22,21,21,0,0,
18,0,0,6,11,21,0,0,0,0,0,0,0,0,240,63,
15,15,21,0,18,0,0,1,12,23,0,6,100,111,95,100,
101,102,0,0,13,22,23,0,15,23,17,0,15,24,7,0,
31,21,23,2,19,21,22,21,12,24,0,6,97,112,112,101,
110,100,0,0,9,23,8,24,12,25,0,4,116,121,112,101,
0,0,0,0,12,26,0,6,115,121,109,98,111,108,0,0,
12,27,0,3,118,97,108,0,12,28,0,1,61,0,0,0,
12,29,0,5,105,116,101,109,115,0,0,0,12,33,0,4,
116,121,112,101,0,0,0,0,12,34,0,3,103,101,116,0,
12,35,0,5,105,116,101,109,115,0,0,0,12,39,0,4,
116,121,112,101,0,0,0,0,12,40,0,4,110,97,109,101,
0,0,0,0,12,41,0,3,118,97,108,0,12,42,0,4,
115,101,108,102,0,0,0,0,26,37,39,4,12,39,0,4,
116,121,112,101,0,0,0,0,12,40,0,6,115,116,114,105,
110,103,0,0,12,41,0,3,118,97,108,0,15,42,20,0,
26,38,39,4,27,36,37,2,26,31,33,4,12,33,0,4,
116,121,112,101,0,0,0,0,12,34,0,4,99,97,108,108,
0,0,0,0,12,35,0,5,105,116,101,109,115,0,0,0,
12,40,0,4,116,121,112,101,0,0,0,0,12,41,0,4,
110,97,109,101,0,0,0,0,12,42,0,3,118,97,108,0,
12,43,0,4,98,105,110,100,0,0,0,0,26,37,40,4,
12,40,0,4,116,121,112,101,0,0,0,0,12,41,0,3,
103,101,116,0,12,42,0,5,105,116,101,109,115,0,0,0,
12,46,0,4,116,121,112,101,0,0,0,0,12,47,0,4,
110,97,109,101,0,0,0,0,12,48,0,3,118,97,108,0,
15,49,6,0,26,44,46,4,12,46,0,4,116,121,112,101,
0,0,0,0,12,47,0,6,115,116,114,105,110,103,0,0,
12,48,0,3,118,97,108,0,15,49,20,0,26,45,46,4,
27,43,44,2,26,38,40,4,12,40,0,4,116,121,112,101,
0,0,0,0,12,41,0,4,110,97,109,101,0,0,0,0,
12,42,0,3,118,97,108,0,12,43,0,4,115,101,108,102,
0,0,0,0,26,39,40,4,27,36,37,3,26,32,33,4,
27,30,31,2,26,24,25,6,31,21,24,1,19,21,23,21,
18,0,255,93,12,24,0,6,100,111,95,100,101,102,0,0,
13,21,24,0,12,26,0,5,105,116,101,109,115,0,0,0,
12,31,0,3,118,97,108,0,12,32,0,7,95,95,110,101,
119,95,95,0,26,28,31,2,12,31,0,5,105,116,101,109,
115,0,0,0,12,34,0,4,116,121,112,101,0,0,0,0,
12,35,0,4,110,97,109,101,0,0,0,0,12,36,0,3,
118,97,108,0,12,37,0,4,115,101,108,102,0,0,0,0,
26,33,34,4,27,32,33,1,26,29,31,2,12,31,0,4,
116,121,112,101,0,0,0,0,12,32,0,10,115,116,97,116,
101,109,101,110,116,115,0,0,12,33,0,5,105,116,101,109,
115,0,0,0,15,34,8,0,26,30,31,4,27,27,28,3,
26,24,26,2,15,25,7,0,31,19,24,2,19,19,21,19,
12,25,0,7,103,101,116,95,116,97,103,0,13,24,25,0,
31,19,0,0,19,19,24,19,15,1,19,0,12,27,0,3,
102,110,99,0,13,26,27,0,15,27,1,0,12,28,0,3,
101,110,100,0,31,25,27,2,19,25,26,25,15,19,25,0,
12,28,0,1,68,0,0,0,13,27,28,0,12,28,0,5,
98,101,103,105,110,0,0,0,9,27,27,28,31,25,0,0,
19,25,27,25,12,30,0,8,100,111,95,108,111,99,97,108,
0,0,0,0,13,29,30,0,12,31,0,3,118,97,108,0,
12,32,0,8,95,95,112,97,114,97,109,115,0,0,0,0,
26,30,31,2,31,28,30,1,19,28,29,28,15,25,28,0,
12,32,0,8,100,111,95,108,111,99,97,108,0,0,0,0,
13,31,32,0,12,33,0,3,118,97,108,0,12,34,0,4,
115,101,108,102,0,0,0,0,26,32,33,2,31,30,32,1,
19,30,31,30,15,28,30,0,12,33,0,4,99,111,100,101,
0,0,0,0,13,32,33,0,12,37,0,4,68,73,67,84,
0,0,0,0,13,33,37,0,15,34,28,0,11,35,0,0,
0,0,0,0,0,0,0,0,11,36,0,0,0,0,0,0,
0,0,0,0,31,30,33,4,19,30,32,30,12,34,0,7,
100,111,95,99,97,108,108,0,13,33,34,0,12,35,0,5,
105,116,101,109,115,0,0,0,12,39,0,4,116,121,112,101,
0,0,0,0,12,40,0,3,103,101,116,0,12,41,0,5,
105,116,101,109,115,0,0,0,12,45,0,4,116,121,112,101,
0,0,0,0,12,46,0,4,110,97,109,101,0,0,0,0,
12,47,0,3,118,97,108,0,15,48,6,0,26,43,45,4,
12,45,0,4,116,121,112,101,0,0,0,0,12,46,0,6,
115,116,114,105,110,103,0,0,12,47,0,3,118,97,108,0,
12,48,0,7,95,95,110,101,119,95,95,0,26,44,45,4,
27,42,43,2,26,37,39,4,12,39,0,4,116,121,112,101,
0,0,0,0,12,40,0,4,110,97,109,101,0,0,0,0,
12,41,0,3,118,97,108,0,12,42,0,4,115,101,108,102,
0,0,0,0,26,38,39,4,27,36,37,2,26,34,35,2,
31,30,34,1,19,30,33,30,21,15,0,0,18,0,0,52,
12,36,0,7,103,101,116,95,116,109,112,0,13,35,36,0,
31,34,0,0,19,34,35,34,15,30,34,0,12,37,0,4,
99,111,100,101,0,0,0,0,13,36,37,0,12,41,0,3,
71,69,84,0,13,37,41,0,15,38,30,0,15,39,28,0,
12,42,0,9,100,111,95,115,116,114,105,110,103,0,0,0,
13,41,42,0,12,43,0,3,118,97,108,0,12,44,0,8,
95,95,105,110,105,116,95,95,0,0,0,0,26,42,43,2,
31,40,42,1,19,40,41,40,31,34,37,4,19,34,36,34,
12,38,0,4,99,111,100,101,0,0,0,0,13,37,38,0,
12,38,0,4,67,65,76,76,0,0,0,0,13,42,38,0,
12,39,0,7,103,101,116,95,116,109,112,0,13,38,39,0,
31,43,0,0,19,43,38,43,15,44,30,0,15,45,25,0,
31,34,42,4,19,34,37,34,18,0,0,1,12,40,0,4,
99,111,100,101,0,0,0,0,13,39,40,0,12,40,0,6,
82,69,84,85,82,78,0,0,13,42,40,0,15,43,28,0,
31,34,42,2,19,34,39,34,12,42,0,1,68,0,0,0,
13,40,42,0,12,42,0,3,101,110,100,0,9,40,40,42,
31,34,0,0,19,34,40,34,12,43,0,3,116,97,103,0,
13,42,43,0,15,43,1,0,12,44,0,3,101,110,100,0,
31,34,43,2,19,34,42,34,12,44,0,4,99,111,100,101,
0,0,0,0,13,43,44,0,12,48,0,3,83,69,84,0,
13,44,48,0,15,45,7,0,12,49,0,9,100,111,95,115,
116,114,105,110,103,0,0,0,13,48,49,0,12,50,0,3,
118,97,108,0,12,51,0,8,95,95,99,97,108,108,95,95,
0,0,0,0,26,49,50,2,31,46,49,1,19,46,48,46,
15,47,19,0,31,34,44,4,19,34,43,34,0,0,0,0,
12,99,0,8,100,111,95,99,108,97,115,115,0,0,0,0,
14,99,98,0,16,99,0,108,44,18,0,0,28,2,0,0,
9,1,0,2,12,5,0,5,105,116,101,109,115,0,0,0,
9,4,1,5,15,3,4,0,12,6,0,9,115,116,97,99,
107,95,116,97,103,0,0,0,13,5,6,0,31,4,0,0,
19,4,5,4,15,1,4,0,12,7,0,3,116,97,103,0,
13,6,7,0,15,7,1,0,12,8,0,5,98,101,103,105,
110,0,0,0,31,4,7,2,19,4,6,4,12,8,0,3,
116,97,103,0,13,7,8,0,15,8,1,0,12,9,0,8,
99,111,110,116,105,110,117,101,0,0,0,0,31,4,8,2,
19,4,7,4,12,10,0,2,100,111,0,0,13,9,10,0,
11,11,0,0,0,0,0,0,0,0,0,0,9,10,3,11,
31,8,10,1,19,8,9,8,15,4,8,0,12,11,0,4,
99,111,100,101,0,0,0,0,13,10,11,0,12,13,0,2,
73,70,0,0,13,11,13,0,15,12,4,0,31,8,11,2,
19,8,10,8,12,12,0,4,106,117,109,112,0,0,0,0,
13,11,12,0,15,12,1,0,12,13,0,3,101,110,100,0,
31,8,12,2,19,8,11,8,12,13,0,2,100,111,0,0,
13,12,13,0,11,14,0,0,0,0,0,0,0,0,240,63,
9,13,3,14,31,8,13,1,19,8,12,8,12,14,0,4,
106,117,109,112,0,0,0,0,13,13,14,0,15,14,1,0,
12,15,0,5,98,101,103,105,110,0,0,0,31,8,14,2,
19,8,13,8,12,15,0,3,116,97,103,0,13,14,15,0,
15,15,1,0,12,16,0,5,98,114,101,97,107,0,0,0,
31,8,15,2,19,8,14,8,12,16,0,3,116,97,103,0,
13,15,16,0,15,16,1,0,12,17,0,3,101,110,100,0,
31,8,16,2,19,8,15,8,12,17,0,7,112,111,112,95,
116,97,103,0,13,16,17,0,31,8,0,0,19,8,16,8,
0,0,0,0,12,100,0,8,100,111,95,119,104,105,108,101,
0,0,0,0,14,100,99,0,16,100,0,144,44,24,0,0,
28,2,0,0,9,1,0,2,12,5,0,5,105,116,101,109,
115,0,0,0,9,4,1,5,15,3,4,0,12,7,0,8,
100,111,95,108,111,99,97,108,0,0,0,0,13,6,7,0,
11,8,0,0,0,0,0,0,0,0,0,0,9,7,3,8,
31,5,7,1,19,5,6,5,15,4,5,0,12,9,0,2,
100,111,0,0,13,8,9,0,11,10,0,0,0,0,0,0,
0,0,240,63,9,9,3,10,31,7,9,1,19,7,8,7,
15,5,7,0,12,11,0,9,100,111,95,110,117,109,98,101,
114,0,0,0,13,10,11,0,12,12,0,3,118,97,108,0,
12,13,0,1,48,0,0,0,26,11,12,2,31,9,11,1,
19,9,10,9,15,7,9,0,12,13,0,9,115,116,97,99,
107,95,116,97,103,0,0,0,13,12,13,0,31,11,0,0,
19,11,12,11,15,9,11,0,12,14,0,3,116,97,103,0,
13,13,14,0,15,14,9,0,12,15,0,4,108,111,111,112,
0,0,0,0,31,11,14,2,19,11,13,11,12,15,0,3,
116,97,103,0,13,14,15,0,15,15,9,0,12,16,0,8,
99,111,110,116,105,110,117,101,0,0,0,0,31,11,15,2,
19,11,14,11,12,16,0,4,99,111,100,101,0,0,0,0,
13,15,16,0,12,20,0,4,73,84,69,82,0,0,0,0,
13,16,20,0,15,17,4,0,15,18,5,0,15,19,7,0,
31,11,16,4,19,11,15,11,12,17,0,4,106,117,109,112,
0,0,0,0,13,16,17,0,15,17,9,0,12,18,0,3,
101,110,100,0,31,11,17,2,19,11,16,11,12,18,0,2,
100,111,0,0,13,17,18,0,11,19,0,0,0,0,0,0,
0,0,0,64,9,18,3,19,31,11,18,1,19,11,17,11,
12,19,0,4,106,117,109,112,0,0,0,0,13,18,19,0,
15,19,9,0,12,20,0,4,108,111,111,112,0,0,0,0,
31,11,19,2,19,11,18,11,12,20,0,3,116,97,103,0,
13,19,20,0,15,20,9,0,12,21,0,5,98,114,101,97,
107,0,0,0,31,11,20,2,19,11,19,11,12,21,0,3,
116,97,103,0,13,20,21,0,15,21,9,0,12,22,0,3,
101,110,100,0,31,11,21,2,19,11,20,11,12,22,0,7,
112,111,112,95,116,97,103,0,13,21,22,0,31,11,0,0,
19,11,21,11,12,23,0,8,102,114,101,101,95,116,109,112,
0,0,0,0,13,22,23,0,15,23,7,0,31,11,23,1,
19,11,22,11,0,0,0,0,12,101,0,6,100,111,95,102,
111,114,0,0,14,101,100,0,16,101,0,169,44,26,0,0,
28,2,0,0,9,1,0,2,28,3,0,0,28,4,0,0,
32,3,0,4,12,6,0,5,99,111,109,112,58,0,0,0,
12,9,0,7,103,101,116,95,116,97,103,0,13,8,9,0,
31,7,0,0,19,7,8,7,1,6,6,7,15,5,6,0,
12,9,0,8,100,111,95,108,111,99,97,108,0,0,0,0,
13,7,9,0,12,10,0,3,118,97,108,0,15,11,5,0,
26,9,10,2,31,6,9,1,19,6,7,6,15,3,6,0,
12,10,0,4,99,111,100,101,0,0,0,0,13,9,10,0,
12,14,0,4,76,73,83,84,0,0,0,0,13,10,14,0,
15,11,3,0,11,12,0,0,0,0,0,0,0,0,0,0,
11,13,0,0,0,0,0,0,0,0,0,0,31,6,10,4,
19,6,9,6,12,11,0,4,102,114,111,109,0,0,0,0,
12,17,0,4,102,114,111,109,0,0,0,0,9,12,1,17,
12,13,0,4,116,121,112,101,0,0,0,0,12,14,0,3,
103,101,116,0,12,15,0,5,105,116,101,109,115,0,0,0,
12,19,0,4,102,114,111,109,0,0,0,0,12,25,0,4,
102,114,111,109,0,0,0,0,9,20,1,25,12,21,0,4,
116,121,112,101,0,0,0,0,12,22,0,3,114,101,103,0,
12,23,0,3,118,97,108,0,15,24,3,0,26,17,19,6,
12,19,0,4,102,114,111,109,0,0,0,0,12,25,0,4,
102,114,111,109,0,0,0,0,9,20,1,25,12,21,0,4,
116,121,112,101,0,0,0,0,12,22,0,6,115,121,109,98,
111,108,0,0,12,23,0,3,118,97,108,0,12,24,0,4,
78,111,110,101,0,0,0,0,26,18,19,6,27,16,17,2,
26,10,11,6,15,6,10,0,12,12,0,4,102,114,111,109,
0,0,0,0,12,20,0,4,102,114,111,109,0,0,0,0,
9,13,1,20,12,14,0,4,116,121,112,101,0,0,0,0,
12,15,0,6,115,121,109,98,111,108,0,0,12,16,0,3,
118,97,108,0,12,17,0,1,61,0,0,0,12,18,0,5,
105,116,101,109,115,0,0,0,15,20,6,0,12,22,0,5,
105,116,101,109,115,0,0,0,9,21,1,22,11,22,0,0,
0,0,0,0,0,0,0,0,9,21,21,22,27,19,20,2,
26,11,12,8,15,10,11,0,12,13,0,6,100,111,95,102,
111,114,0,0,13,12,13,0,12,14,0,4,102,114,111,109,
0,0,0,0,12,18,0,4,102,114,111,109,0,0,0,0,
9,15,1,18,12,16,0,5,105,116,101,109,115,0,0,0,
12,21,0,5,105,116,101,109,115,0,0,0,9,18,1,21,
11,21,0,0,0,0,0,0,0,0,240,63,9,18,18,21,
12,21,0,5,105,116,101,109,115,0,0,0,9,19,1,21,
11,21,0,0,0,0,0,0,0,0,0,64,9,19,19,21,
15,20,10,0,27,17,18,3,26,13,14,4,31,11,13,1,
19,11,12,11,20,3,0,0,0,0,0,0,12,102,0,7,
100,111,95,99,111,109,112,0,14,102,101,0,16,102,0,157,
44,22,0,0,28,2,0,0,9,1,0,2,12,5,0,5,
105,116,101,109,115,0,0,0,9,4,1,5,15,3,4,0,
12,6,0,7,103,101,116,95,116,97,103,0,13,5,6,0,
31,4,0,0,19,4,5,4,15,1,4,0,11,6,0,0,
0,0,0,0,0,0,0,0,15,4,6,0,11,7,0,0,
0,0,0,0,0,0,0,0,42,6,3,7,18,0,0,117,
12,10,0,3,116,97,103,0,13,9,10,0,15,10,1,0,
15,11,4,0,31,8,10,2,19,8,9,8,12,10,0,4,
116,121,112,101,0,0,0,0,9,8,6,10,12,10,0,4,
101,108,105,102,0,0,0,0,23,8,8,10,21,8,0,0,
18,0,0,58,12,12,0,2,100,111,0,0,13,11,12,0,
12,13,0,5,105,116,101,109,115,0,0,0,9,12,6,13,
11,13,0,0,0,0,0,0,0,0,0,0,9,12,12,13,
31,10,12,1,19,10,11,10,15,8,10,0,12,13,0,4,
99,111,100,101,0,0,0,0,13,12,13,0,12,15,0,2,
73,70,0,0,13,13,15,0,15,14,8,0,31,10,13,2,
19,10,12,10,12,14,0,8,102,114,101,101,95,116,109,112,
0,0,0,0,13,13,14,0,15,14,8,0,31,10,14,1,
19,10,13,10,12,15,0,4,106,117,109,112,0,0,0,0,
13,14,15,0,15,15,1,0,11,17,0,0,0,0,0,0,
0,0,240,63,1,16,4,17,31,10,15,2,19,10,14,10,
12,16,0,2,100,111,0,0,13,15,16,0,12,17,0,5,
105,116,101,109,115,0,0,0,9,16,6,17,11,17,0,0,
0,0,0,0,0,0,240,63,9,16,16,17,31,10,16,1,
19,10,15,10,18,0,0,28,12,16,0,4,116,121,112,101,
0,0,0,0,9,10,6,16,12,16,0,4,101,108,115,101,
0,0,0,0,23,10,10,16,21,10,0,0,18,0,0,15,
12,17,0,2,100,111,0,0,13,16,17,0,12,18,0,5,
105,116,101,109,115,0,0,0,9,17,6,18,11,18,0,0,
0,0,0,0,0,0,0,0,9,17,17,18,31,10,17,1,
19,10,16,10,18,0,0,4,28,17,0,0,37,17,0,0,
18,0,0,1,12,19,0,4,106,117,109,112,0,0,0,0,
13,18,19,0,15,19,1,0,12,20,0,3,101,110,100,0,
31,17,19,2,19,17,18,17,11,19,0,0,0,0,0,0,
0,0,240,63,1,17,4,19,15,4,17,0,18,0,255,139,
12,19,0,3,116,97,103,0,13,17,19,0,15,19,1,0,
15,20,4,0,31,7,19,2,19,7,17,7,12,20,0,3,
116,97,103,0,13,19,20,0,15,20,1,0,12,21,0,3,
101,110,100,0,31,7,20,2,19,7,19,7,0,0,0,0,
12,103,0,5,100,111,95,105,102,0,0,0,14,103,102,0,
16,103,0,79,44,14,0,0,28,2,0,0,9,1,0,2,
12,5,0,5,105,116,101,109,115,0,0,0,9,4,1,5,
15,3,4,0,12,6,0,7,103,101,116,95,116,97,103,0,
13,5,6,0,31,4,0,0,19,4,5,4,15,1,4,0,
12,7,0,6,115,101,116,106,109,112,0,0,13,6,7,0,
15,7,1,0,12,8,0,6,101,120,99,101,112,116,0,0,
31,4,7,2,19,4,6,4,12,8,0,2,100,111,0,0,
13,7,8,0,11,9,0,0,0,0,0,0,0,0,0,0,
9,8,3,9,31,4,8,1,19,4,7,4,12,9,0,4,
106,117,109,112,0,0,0,0,13,8,9,0,15,9,1,0,
12,10,0,3,101,110,100,0,31,4,9,2,19,4,8,4,
12,10,0,3,116,97,103,0,13,9,10,0,15,10,1,0,
12,11,0,6,101,120,99,101,112,116,0,0,31,4,10,2,
19,4,9,4,12,11,0,2,100,111,0,0,13,10,11,0,
11,12,0,0,0,0,0,0,0,0,240,63,9,11,3,12,
12,12,0,5,105,116,101,109,115,0,0,0,9,11,11,12,
11,12,0,0,0,0,0,0,0,0,240,63,9,11,11,12,
31,4,11,1,19,4,10,4,12,12,0,3,116,97,103,0,
13,11,12,0,15,12,1,0,12,13,0,3,101,110,100,0,
31,4,12,2,19,4,11,4,0,0,0,0,12,104,0,6,
100,111,95,116,114,121,0,0,14,104,103,0,16,104,0,69,
44,13,0,0,28,2,0,0,9,1,0,2,12,4,0,5,
105,116,101,109,115,0,0,0,36,3,1,4,21,3,0,0,
18,0,0,16,12,6,0,2,100,111,0,0,13,5,6,0,
12,7,0,5,105,116,101,109,115,0,0,0,9,6,1,7,
11,7,0,0,0,0,0,0,0,0,0,0,9,6,6,7,
31,4,6,1,19,4,5,4,15,3,4,0,18,0,0,23,
12,7,0,9,100,111,95,115,121,109,98,111,108,0,0,0,
13,6,7,0,12,8,0,4,102,114,111,109,0,0,0,0,
12,12,0,4,102,114,111,109,0,0,0,0,9,9,1,12,
12,10,0,3,118,97,108,0,12,11,0,4,78,111,110,101,
0,0,0,0,26,7,8,4,31,4,7,1,19,4,6,4,
15,3,4,0,18,0,0,1,12,8,0,4,99,111,100,101,
0,0,0,0,13,7,8,0,12,10,0,6,82,69,84,85,
82,78,0,0,13,8,10,0,15,9,3,0,31,4,8,2,
19,4,7,4,12,9,0,8,102,114,101,101,95,116,109,112,
0,0,0,0,13,8,9,0,15,9,3,0,31,4,9,1,
19,4,8,4,28,4,0,0,20,4,0,0,0,0,0,0,
12,105,0,9,100,111,95,114,101,116,117,114,110,0,0,0,
14,105,104,0,16,105,0,69,44,13,0,0,28,2,0,0,
9,1,0,2,12,4,0,5,105,116,101,109,115,0,0,0,
36,3,1,4,21,3,0,0,18,0,0,16,12,6,0,2,
100,111,0,0,13,5,6,0,12,7,0,5,105,116,101,109,
115,0,0,0,9,6,1,7,11,7,0,0,0,0,0,0,
0,0,0,0,9,6,6,7,31,4,6,1,19,4,5,4,
15,3,4,0,18,0,0,23,12,7,0,9,100,111,95,115,
121,109,98,111,108,0,0,0,13,6,7,0,12,8,0,4,
102,114,111,109,0,0,0,0,12,12,0,4,102,114,111,109,
0,0,0,0,9,9,1,12,12,10,0,3,118,97,108,0,
12,11,0,4,78,111,110,101,0,0,0,0,26,7,8,4,
31,4,7,1,19,4,6,4,15,3,4,0,18,0,0,1,
12,8,0,4,99,111,100,101,0,0,0,0,13,7,8,0,
12,10,0,5,82,65,73,83,69,0,0,0,13,8,10,0,
15,9,3,0,31,4,8,2,19,4,7,4,12,9,0,8,
102,114,101,101,95,116,109,112,0,0,0,0,13,8,9,0,
15,9,3,0,31,4,9,1,19,4,8,4,28,4,0,0,
20,4,0,0,0,0,0,0,12,106,0,8,100,111,95,114,
97,105,115,101,0,0,0,0,14,106,105,0,16,106,0,28,
44,11,0,0,28,2,0,0,9,1,0,2,12,5,0,5,
105,116,101,109,115,0,0,0,9,4,1,5,11,5,0,0,
0,0,0,0,0,0,0,0,42,3,4,5,18,0,0,15,
12,8,0,8,102,114,101,101,95,116,109,112,0,0,0,0,
13,7,8,0,12,10,0,2,100,111,0,0,13,9,10,0,
15,10,3,0,31,8,10,1,19,8,9,8,31,6,8,1,
19,6,7,6,18,0,255,241,0,0,0,0,12,107,0,13,
100,111,95,115,116,97,116,101,109,101,110,116,115,0,0,0,
14,107,106,0,16,107,0,33,44,12,0,0,28,2,0,0,
9,1,0,2,28,3,0,0,28,4,0,0,32,3,0,4,
12,7,0,7,103,101,116,95,116,109,112,0,13,6,7,0,
15,7,3,0,31,5,7,1,19,5,6,5,15,3,5,0,
12,8,0,10,109,97,110,97,103,101,95,115,101,113,0,0,
13,7,8,0,12,11,0,4,76,73,83,84,0,0,0,0,
13,8,11,0,15,9,3,0,12,11,0,5,105,116,101,109,
115,0,0,0,9,10,1,11,31,5,8,3,19,5,7,5,
20,3,0,0,0,0,0,0,12,108,0,7,100,111,95,108,
105,115,116,0,14,108,107,0,16,108,0,33,44,12,0,0,
28,2,0,0,9,1,0,2,28,3,0,0,28,4,0,0,
32,3,0,4,12,7,0,7,103,101,116,95,116,109,112,0,
13,6,7,0,15,7,3,0,31,5,7,1,19,5,6,5,
15,3,5,0,12,8,0,10,109,97,110,97,103,101,95,115,
101,113,0,0,13,7,8,0,12,11,0,4,68,73,67,84,
0,0,0,0,13,8,11,0,15,9,3,0,12,11,0,5,
105,116,101,109,115,0,0,0,9,10,1,11,31,5,8,3,
19,5,7,5,20,3,0,0,0,0,0,0,12,109,0,7,
100,111,95,100,105,99,116,0,14,109,108,0,16,109,0,32,
44,13,0,0,28,2,0,0,9,1,0,2,28,3,0,0,
28,4,0,0,32,3,0,4,12,7,0,5,105,116,101,109,
115,0,0,0,9,6,1,7,15,5,6,0,12,8,0,5,
105,110,102,105,120,0,0,0,13,7,8,0,12,12,0,3,
71,69,84,0,13,8,12,0,11,12,0,0,0,0,0,0,
0,0,0,0,9,9,5,12,11,12,0,0,0,0,0,0,
0,0,240,63,9,10,5,12,15,11,3,0,31,6,8,4,
19,6,7,6,20,6,0,0,0,0,0,0,12,110,0,6,
100,111,95,103,101,116,0,0,14,110,109,0,16,110,0,25,
44,8,0,0,28,2,0,0,9,1,0,2,12,5,0,4,
106,117,109,112,0,0,0,0,13,4,5,0,12,7,0,1,
68,0,0,0,13,5,7,0,12,7,0,6,116,115,116,97,
99,107,0,0,9,5,5,7,11,7,0,0,0,0,0,0,
0,0,240,191,9,5,5,7,12,6,0,5,98,114,101,97,
107,0,0,0,31,3,5,2,19,3,4,3,0,0,0,0,
12,111,0,8,100,111,95,98,114,101,97,107,0,0,0,0,
14,111,110,0,16,111,0,26,44,8,0,0,28,2,0,0,
9,1,0,2,12,5,0,4,106,117,109,112,0,0,0,0,
13,4,5,0,12,7,0,1,68,0,0,0,13,5,7,0,
12,7,0,6,116,115,116,97,99,107,0,0,9,5,5,7,
11,7,0,0,0,0,0,0,0,0,240,191,9,5,5,7,
12,6,0,8,99,111,110,116,105,110,117,101,0,0,0,0,
31,3,5,2,19,3,4,3,0,0,0,0,12,112,0,11,
100,111,95,99,111,110,116,105,110,117,101,0,14,112,111,0,
16,112,0,15,44,7,0,0,28,2,0,0,9,1,0,2,
12,5,0,4,99,111,100,101,0,0,0,0,13,4,5,0,
12,6,0,4,80,65,83,83,0,0,0,0,13,5,6,0,
31,3,5,1,19,3,4,3,0,0,0,0,12,113,0,7,
100,111,95,112,97,115,115,0,14,113,112,0,16,113,0,82,
44,16,0,0,12,1,0,1,63,0,0,0,28,2,0,0,
32,1,0,2,12,4,0,4,65,82,71,86,0,0,0,0,
13,3,4,0,12,4,0,6,45,110,111,112,111,115,0,0,
36,3,3,4,21,3,0,0,18,0,0,4,28,3,0,0,
20,3,0,0,18,0,0,1,12,5,0,4,99,111,100,101,
0,0,0,0,13,4,5,0,12,7,0,4,70,73,76,69,
0,0,0,0,13,5,7,0,12,8,0,8,102,114,101,101,
95,116,109,112,0,0,0,0,13,7,8,0,12,10,0,9,
100,111,95,115,116,114,105,110,103,0,0,0,13,9,10,0,
12,11,0,3,118,97,108,0,12,13,0,1,68,0,0,0,
13,12,13,0,12,13,0,5,102,110,97,109,101,0,0,0,
9,12,12,13,26,10,11,2,31,8,10,1,19,8,9,8,
31,6,8,1,19,6,7,6,31,3,5,2,19,3,4,3,
12,6,0,4,99,111,100,101,0,0,0,0,13,5,6,0,
12,6,0,4,78,65,77,69,0,0,0,0,13,10,6,0,
12,8,0,8,102,114,101,101,95,116,109,112,0,0,0,0,
13,6,8,0,12,13,0,9,100,111,95,115,116,114,105,110,
103,0,0,0,13,12,13,0,12,14,0,3,118,97,108,0,
15,15,1,0,26,13,14,2,31,8,13,1,19,8,12,8,
31,11,8,1,19,11,6,11,31,3,10,2,19,3,5,3,
0,0,0,0,12,114,0,7,100,111,95,105,110,102,111,0,
14,114,113,0,16,114,0,24,44,8,0,0,28,2,0,0,
9,1,0,2,12,5,0,7,100,111,95,105,110,102,111,0,
13,4,5,0,31,3,0,0,19,3,4,3,12,6,0,2,
100,111,0,0,13,5,6,0,12,7,0,5,105,116,101,109,
115,0,0,0,9,6,1,7,11,7,0,0,0,0,0,0,
0,0,0,0,9,6,6,7,31,3,6,1,19,3,5,3,
0,0,0,0,12,115,0,9,100,111,95,109,111,100,117,108,
101,0,0,0,14,115,114,0,16,115,0,12,44,7,0,0,
28,2,0,0,9,1,0,2,28,3,0,0,28,4,0,0,
32,3,0,4,12,6,0,3,118,97,108,0,9,5,1,6,
20,5,0,0,0,0,0,0,12,116,0,6,100,111,95,114,
101,103,0,0,14,116,115,0,12,116,0,4,102,109,97,112,
0,0,0,0,12,118,0,6,109,111,100,117,108,101,0,0,
12,150,0,9,100,111,95,109,111,100,117,108,101,0,0,0,
13,119,150,0,12,120,0,10,115,116,97,116,101,109,101,110,
116,115,0,0,12,150,0,13,100,111,95,115,116,97,116,101,
109,101,110,116,115,0,0,0,13,121,150,0,12,122,0,3,
100,101,102,0,12,150,0,6,100,111,95,100,101,102,0,0,
13,123,150,0,12,124,0,6,114,101,116,117,114,110,0,0,
12,150,0,9,100,111,95,114,101,116,117,114,110,0,0,0,
13,125,150,0,12,126,0,5,119,104,105,108,101,0,0,0,
12,150,0,8,100,111,95,119,104,105,108,101,0,0,0,0,
13,127,150,0,12,128,0,2,105,102,0,0,12,150,0,5,
100,111,95,105,102,0,0,0,13,129,150,0,12,130,0,5,
98,114,101,97,107,0,0,0,12,150,0,8,100,111,95,98,
114,101,97,107,0,0,0,0,13,131,150,0,12,132,0,4,
112,97,115,115,0,0,0,0,12,150,0,7,100,111,95,112,
97,115,115,0,13,133,150,0,12,134,0,8,99,111,110,116,
105,110,117,101,0,0,0,0,12,150,0,11,100,111,95,99,
111,110,116,105,110,117,101,0,13,135,150,0,12,136,0,3,
102,111,114,0,12,150,0,6,100,111,95,102,111,114,0,0,
13,137,150,0,12,138,0,5,99,108,97,115,115,0,0,0,
12,150,0,8,100,111,95,99,108,97,115,115,0,0,0,0,
13,139,150,0,12,140,0,5,114,97,105,115,101,0,0,0,
12,150,0,8,100,111,95,114,97,105,115,101,0,0,0,0,
13,141,150,0,12,142,0,3,116,114,121,0,12,150,0,6,
100,111,95,116,114,121,0,0,13,143,150,0,12,144,0,6,
105,109,112,111,114,116,0,0,12,150,0,9,100,111,95,105,
109,112,111,114,116,0,0,0,13,145,150,0,12,146,0,7,
103,108,111,98,97,108,115,0,12,150,0,10,100,111,95,103,
108,111,98,97,108,115,0,0,13,147,150,0,12,148,0,3,
100,101,108,0,12,150,0,6,100,111,95,100,101,108,0,0,
13,149,150,0,26,117,118,32,14,116,117,0,12,116,0,4,
114,109,97,112,0,0,0,0,12,118,0,4,108,105,115,116,
0,0,0,0,12,142,0,7,100,111,95,108,105,115,116,0,
13,119,142,0,12,120,0,5,116,117,112,108,101,0,0,0,
12,142,0,7,100,111,95,108,105,115,116,0,13,121,142,0,
12,122,0,4,100,105,99,116,0,0,0,0,12,142,0,7,
100,111,95,100,105,99,116,0,13,123,142,0,12,124,0,5,
115,108,105,99,101,0,0,0,12,142,0,7,100,111,95,108,
105,115,116,0,13,125,142,0,12,126,0,4,99,111,109,112,
0,0,0,0,12,142,0,7,100,111,95,99,111,109,112,0,
13,127,142,0,12,128,0,4,110,97,109,101,0,0,0,0,
12,142,0,7,100,111,95,110,97,109,101,0,13,129,142,0,
12,130,0,6,115,121,109,98,111,108,0,0,12,142,0,9,
100,111,95,115,121,109,98,111,108,0,0,0,13,131,142,0,
12,132,0,6,110,117,109,98,101,114,0,0,12,142,0,9,
100,111,95,110,117,109,98,101,114,0,0,0,13,133,142,0,
12,134,0,6,115,116,114,105,110,103,0,0,12,142,0,9,
100,111,95,115,116,114,105,110,103,0,0,0,13,135,142,0,
12,136,0,3,103,101,116,0,12,142,0,6,100,111,95,103,
101,116,0,0,13,137,142,0,12,138,0,4,99,97,108,108,
0,0,0,0,12,142,0,7,100,111,95,99,97,108,108,0,
13,139,142,0,12,140,0,3,114,101,103,0,12,142,0,6,
100,111,95,114,101,103,0,0,13,141,142,0,26,117,118,24,
14,116,117,0,16,116,0,137,44,19,0,0,28,2,0,0,
9,1,0,2,28,3,0,0,28,4,0,0,32,3,0,4,
12,6,0,4,102,114,111,109,0,0,0,0,36,5,1,6,
21,5,0,0,18,0,0,12,12,7,0,6,115,101,116,112,
111,115,0,0,13,6,7,0,12,8,0,4,102,114,111,109,
0,0,0,0,9,7,1,8,31,5,7,1,19,5,6,5,
18,0,0,1,38,0,0,41,12,8,0,4,114,109,97,112,
0,0,0,0,13,7,8,0,12,9,0,4,116,121,112,101,
0,0,0,0,9,8,1,9,36,7,7,8,21,7,0,0,
18,0,0,16,12,9,0,4,114,109,97,112,0,0,0,0,
13,8,9,0,12,10,0,4,116,121,112,101,0,0,0,0,
9,9,1,10,9,8,8,9,15,9,1,0,15,10,3,0,
31,7,9,2,19,7,8,7,20,7,0,0,18,0,0,1,
12,10,0,4,102,109,97,112,0,0,0,0,13,9,10,0,
12,11,0,4,116,121,112,101,0,0,0,0,9,10,1,11,
9,9,9,10,15,10,1,0,31,7,10,1,19,7,9,7,
20,7,0,0,18,0,0,72,12,10,0,1,68,0,0,0,
13,7,10,0,12,10,0,5,101,114,114,111,114,0,0,0,
9,7,7,10,21,7,0,0,18,0,0,4,28,7,0,0,
37,7,0,0,18,0,0,1,12,10,0,1,68,0,0,0,
13,7,10,0,11,10,0,0,0,0,0,0,0,0,240,63,
12,11,0,5,101,114,114,111,114,0,0,0,10,7,11,10,
12,12,0,4,102,114,111,109,0,0,0,0,36,10,1,12,
11,12,0,0,0,0,0,0,0,0,0,0,23,10,10,12,
21,10,0,0,18,0,0,15,12,13,0,5,112,114,105,110,
116,0,0,0,13,12,13,0,12,13,0,6,117,104,32,111,
104,104,0,0,12,15,0,4,116,121,112,101,0,0,0,0,
9,14,1,15,31,10,13,2,19,10,12,10,18,0,0,1,
12,15,0,8,116,111,107,101,110,105,122,101,0,0,0,0,
13,14,15,0,12,15,0,7,117,95,101,114,114,111,114,0,
9,14,14,15,12,15,0,4,100,117,109,112,0,0,0,0,
12,18,0,1,68,0,0,0,13,16,18,0,12,18,0,4,
99,111,100,101,0,0,0,0,9,16,16,18,12,18,0,4,
102,114,111,109,0,0,0,0,9,17,1,18,31,13,15,3,
19,13,14,13,0,0,0,0,12,117,0,2,100,111,0,0,
14,117,116,0,16,117,0,115,44,18,0,0,28,2,0,0,
9,1,0,2,28,4,0,0,9,3,0,4,28,6,0,0,
9,5,0,6,12,8,0,4,102,114,111,109,0,0,0,0,
11,16,0,0,0,0,0,0,0,0,240,63,11,17,0,0,
0,0,0,0,0,0,240,63,27,9,16,2,12,10,0,4,
116,121,112,101,0,0,0,0,12,11,0,6,109,111,100,117,
108,101,0,0,12,12,0,3,118,97,108,0,12,13,0,6,
109,111,100,117,108,101,0,0,12,14,0,5,105,116,101,109,
115,0,0,0,15,16,5,0,27,15,16,1,26,7,8,8,
15,5,7,0,12,9,0,8,116,111,107,101,110,105,122,101,
0,0,0,0,13,8,9,0,12,9,0,5,99,108,101,97,
110,0,0,0,9,8,8,9,15,9,3,0,31,7,9,1,
19,7,8,7,15,3,7,0,12,7,0,1,68,0,0,0,
12,11,0,6,68,83,116,97,116,101,0,0,13,10,11,0,
15,11,3,0,15,12,1,0,31,9,11,2,19,9,10,9,
14,7,9,0,12,11,0,1,68,0,0,0,13,9,11,0,
12,11,0,5,98,101,103,105,110,0,0,0,9,9,9,11,
11,11,0,0,0,0,0,0,0,0,240,63,31,7,11,1,
19,7,9,7,12,12,0,2,100,111,0,0,13,11,12,0,
15,12,5,0,31,7,12,1,19,7,11,7,12,13,0,1,
68,0,0,0,13,12,13,0,12,13,0,3,101,110,100,0,
9,12,12,13,31,7,0,0,19,7,12,7,12,14,0,8,
109,97,112,95,116,97,103,115,0,0,0,0,13,13,14,0,
31,7,0,0,19,7,13,7,12,15,0,1,68,0,0,0,
13,14,15,0,12,15,0,3,111,117,116,0,9,14,14,15,
15,7,14,0,12,14,0,1,68,0,0,0,28,15,0,0,
14,14,15,0,12,15,0,0,0,0,0,0,12,16,0,4,
106,111,105,110,0,0,0,0,9,15,15,16,15,16,7,0,
31,14,16,1,19,14,15,14,20,14,0,0,0,0,0,0,
12,118,0,6,101,110,99,111,100,101,0,0,14,118,117,0,
0,0,0,0,
};
unsigned char tp_py2bc[] = {
44,16,0,0,11,0,0,0,0,0,0,0,0,0,0,0,
12,3,0,3,115,116,114,0,13,2,3,0,11,3,0,0,
0,0,0,0,0,0,240,63,31,1,3,1,19,1,2,1,
12,3,0,1,49,0,0,0,23,1,1,3,23,0,0,1,
21,0,0,0,18,0,0,21,12,1,0,4,102,114,111,109,
0,0,0,0,13,0,1,0,12,1,0,5,98,117,105,108,
100,0,0,0,13,0,1,0,12,3,0,6,105,109,112,111,
114,116,0,0,13,1,3,0,12,3,0,1,42,0,0,0,
31,0,3,1,19,0,1,0,12,3,0,1,42,0,0,0,
14,3,0,0,18,0,0,1,12,4,0,6,105,109,112,111,
114,116,0,0,13,3,4,0,12,4,0,8,116,111,107,101,
110,105,122,101,0,0,0,0,31,0,4,1,19,0,3,0,
12,4,0,8,116,111,107,101,110,105,122,101,0,0,0,0,
14,4,0,0,12,5,0,6,105,109,112,111,114,116,0,0,
13,4,5,0,12,5,0,5,112,97,114,115,101,0,0,0,
31,0,5,1,19,0,4,0,12,5,0,5,112,97,114,115,
101,0,0,0,14,5,0,0,12,6,0,6,105,109,112,111,
114,116,0,0,13,5,6,0,12,6,0,6,101,110,99,111,
100,101,0,0,31,0,6,1,19,0,5,0,12,6,0,6,
101,110,99,111,100,101,0,0,14,6,0,0,16,0,0,49,
44,15,0,0,28,2,0,0,9,1,0,2,28,4,0,0,
9,3,0,4,12,8,0,8,116,111,107,101,110,105,122,101,
0,0,0,0,13,7,8,0,12,8,0,8,116,111,107,101,
110,105,122,101,0,0,0,0,9,7,7,8,15,8,1,0,
31,6,8,1,19,6,7,6,15,5,6,0,12,10,0,5,
112,97,114,115,101,0,0,0,13,9,10,0,12,10,0,5,
112,97,114,115,101,0,0,0,9,9,9,10,15,10,1,0,
15,11,5,0,31,8,10,2,19,8,9,8,15,6,8,0,
12,12,0,6,101,110,99,111,100,101,0,0,13,11,12,0,
12,12,0,6,101,110,99,111,100,101,0,0,9,11,11,12,
15,12,3,0,15,13,1,0,15,14,6,0,31,10,12,3,
19,10,11,10,15,8,10,0,20,8,0,0,0,0,0,0,
12,6,0,8,95,99,111,109,112,105,108,101,0,0,0,0,
14,6,0,0,16,6,0,150,44,24,0,0,28,2,0,0,
9,1,0,2,12,4,0,7,77,79,68,85,76,69,83,0,
13,3,4,0,36,3,3,1,21,3,0,0,18,0,0,8,
12,4,0,7,77,79,68,85,76,69,83,0,13,3,4,0,
9,3,3,1,20,3,0,0,18,0,0,1,12,5,0,3,
46,112,121,0,1,4,1,5,15,3,4,0,12,6,0,4,
46,116,112,99,0,0,0,0,1,5,1,6,15,4,5,0,
12,7,0,6,101,120,105,115,116,115,0,0,13,6,7,0,
15,7,3,0,31,5,7,1,19,5,6,5,21,5,0,0,
18,0,0,64,11,8,0,0,0,0,0,0,0,0,240,63,
11,5,0,0,0,0,0,0,0,0,0,0,12,11,0,6,
101,120,105,115,116,115,0,0,13,10,11,0,15,11,4,0,
31,9,11,1,19,9,10,9,23,5,5,9,23,7,5,8,
21,7,0,0,18,0,0,2,18,0,0,16,12,11,0,5,
109,116,105,109,101,0,0,0,13,9,11,0,15,11,4,0,
31,5,11,1,19,5,9,5,12,13,0,5,109,116,105,109,
101,0,0,0,13,12,13,0,15,13,3,0,31,11,13,1,
19,11,12,11,25,5,5,11,21,5,0,0,18,0,0,28,
12,14,0,4,108,111,97,100,0,0,0,0,13,13,14,0,
15,14,3,0,31,11,14,1,19,11,13,11,15,5,11,0,
12,16,0,8,95,99,111,109,112,105,108,101,0,0,0,0,
13,15,16,0,15,16,5,0,15,17,3,0,31,14,16,2,
19,14,15,14,15,11,14,0,12,17,0,4,115,97,118,101,
0,0,0,0,13,16,17,0,15,17,4,0,15,18,11,0,
31,14,17,2,19,14,16,14,18,0,0,1,18,0,0,1,
11,14,0,0,0,0,0,0,0,0,0,0,12,19,0,6,
101,120,105,115,116,115,0,0,13,18,19,0,15,19,4,0,
31,17,19,1,19,17,18,17,23,14,14,17,21,14,0,0,
18,0,0,4,28,14,0,0,37,14,0,0,18,0,0,1,
12,19,0,4,108,111,97,100,0,0,0,0,13,17,19,0,
15,19,4,0,31,14,19,1,19,14,17,14,15,11,14,0,
12,20,0,8,95,95,110,97,109,101,95,95,0,0,0,0,
15,21,1,0,12,22,0,8,95,95,99,111,100,101,95,95,
0,0,0,0,15,23,11,0,26,19,20,4,15,14,19,0,
12,20,0,7,77,79,68,85,76,69,83,0,13,19,20,0,
10,19,1,14,12,22,0,4,101,120,101,99,0,0,0,0,
13,21,22,0,15,22,11,0,15,23,14,0,31,20,22,2,
19,20,21,20,20,14,0,0,0,0,0,0,12,7,0,7,
95,105,109,112,111,114,116,0,14,7,6,0,16,7,0,30,
44,6,0,0,12,2,0,8,66,85,73,76,84,73,78,83,
0,0,0,0,13,1,2,0,12,3,0,8,95,99,111,109,
112,105,108,101,0,0,0,0,13,2,3,0,12,3,0,7,
99,111,109,112,105,108,101,0,10,1,3,2,12,4,0,8,
66,85,73,76,84,73,78,83,0,0,0,0,13,2,4,0,
12,5,0,7,95,105,109,112,111,114,116,0,13,4,5,0,
12,5,0,6,105,109,112,111,114,116,0,0,10,2,5,4,
0,0,0,0,12,8,0,5,95,105,110,105,116,0,0,0,
14,8,7,0,16,8,0,51,44,17,0,0,28,2,0,0,
9,1,0,2,28,4,0,0,9,3,0,4,26,6,0,0,
15,5,6,0,12,6,0,8,95,95,110,97,109,101,95,95,
0,0,0,0,10,5,6,3,12,8,0,7,77,79,68,85,
76,69,83,0,13,7,8,0,10,7,3,5,12,11,0,4,
108,111,97,100,0,0,0,0,13,10,11,0,15,11,1,0,
31,9,11,1,19,9,10,9,15,8,9,0,12,13,0,8,
95,99,111,109,112,105,108,101,0,0,0,0,13,12,13,0,
15,13,8,0,15,14,1,0,31,11,13,2,19,11,12,11,
15,9,11,0,12,11,0,8,95,95,99,111,100,101,95,95,
0,0,0,0,10,5,11,9,12,15,0,4,101,120,101,99,
0,0,0,0,13,14,15,0,15,15,9,0,15,16,5,0,
31,13,15,2,19,13,14,13,20,5,0,0,0,0,0,0,
12,9,0,12,105,109,112,111,114,116,95,102,110,97,109,101,
0,0,0,0,14,9,8,0,16,9,0,24,44,6,0,0,
12,3,0,12,105,109,112,111,114,116,95,102,110,97,109,101,
0,0,0,0,13,2,3,0,12,5,0,4,65,82,71,86,
0,0,0,0,13,3,5,0,11,5,0,0,0,0,0,0,
0,0,0,0,9,3,3,5,12,4,0,8,95,95,109,97,
105,110,95,95,0,0,0,0,31,1,3,2,19,1,2,1,
20,1,0,0,0,0,0,0,12,10,0,6,116,105,110,121,
112,121,0,0,14,10,9,0,16,10,0,33,44,13,0,0,
28,2,0,0,9,1,0,2,28,4,0,0,9,3,0,4,
12,8,0,4,108,111,97,100,0,0,0,0,13,7,8,0,
15,8,1,0,31,6,8,1,19,6,7,6,15,5,6,0,
12,10,0,8,95,99,111,109,112,105,108,101,0,0,0,0,
13,9,10,0,15,10,5,0,15,11,1,0,31,8,10,2,
19,8,9,8,15,6,8,0,12,11,0,4,115,97,118,101,
0,0,0,0,13,10,11,0,15,11,3,0,15,12,6,0,
31,8,11,2,19,8,10,8,0,0,0,0,12,11,0,4,
109,97,105,110,0,0,0,0,14,11,10,0,12,12,0,8,
95,95,110,97,109,101,95,95,0,0,0,0,13,11,12,0,
12,12,0,8,95,95,109,97,105,110,95,95,0,0,0,0,
23,11,11,12,21,11,0,0,18,0,0,24,12,13,0,4,
109,97,105,110,0,0,0,0,13,12,13,0,12,15,0,4,
65,82,71,86,0,0,0,0,13,13,15,0,11,15,0,0,
0,0,0,0,0,0,240,63,9,13,13,15,12,15,0,4,
65,82,71,86,0,0,0,0,13,14,15,0,11,15,0,0,
0,0,0,0,0,0,0,64,9,14,14,15,31,11,13,2,
19,11,12,11,18,0,0,1,0,0,0,0,
};
