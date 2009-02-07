#ifndef _TINYRB_H_
#define _TINYRB_H_

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include "kvec.h"
#include "khash.h"
#include "config.h"

#define TR_ALLOC(T)          (T *)malloc(sizeof(T))
#define TR_ALLOC_N(T,N)      (T *)malloc(sizeof(T)*(N))

#define TR_MEMZERO(X,T)      memset((X), 0, sizeof(T))
#define TR_MEMZERO_N(X,T,N)  memset((X), 0, sizeof(T)*(N))
#define TR_MEMCPY(X,Y,T)     memcpy((X), (Y), sizeof(T))
#define TR_MEMCPY_N(X,Y,T,N) memcpy((X), (Y), sizeof(T)*(N))

#define TR_COBJECT(X)        ((TrObject*)X)
#define TR_TYPE(X)           (TR_IS_FIX(X) ? TR_T_Fixnum : TR_COBJECT(X)->type)
#define TR_IS_A(X,T)         (TR_TYPE(X) == TR_T_##T)
#define TR_CTYPE(X,T)        (assert(TR_IS_A(X,T)),(Tr##T*)(X))
#define TR_CCLASS(X)         TR_CTYPE(X,Class)
#define TR_CFIXNUM(X)        TR_CTYPE(X,Fixnum)
#define TR_CARRAY(X)         TR_CTYPE(X,Array)
#define TR_CSTRING(X)        (assert(TR_IS_A(X,String)||TR_IS_A(X,Symbol)),(TrString*)(X))
#define TR_CMETHOD(X)        ((TrMethod*)X)

#define TR_STR_PTR(S)        (TR_CSTRING(S)->ptr)
#define TR_STR_LEN(S)        (TR_CSTRING(S)->len)
#define TR_ARRAY_PUSH(X,I)   kv_push(OBJ, (TR_CARRAY(X))->kv, (I))
#define TR_ARRAY_AT(X,I)     kv_A((TR_CARRAY(X))->kv, (I))
#define TR_ARRAY_SIZE(X)     kv_size(TR_CARRAY(X)->kv)
#define TR_ARRAY_EACH(T,I,V,B) ({ \
    TrArray *__a##V = TR_CARRAY(T); \
    if (kv_size(__a##V->kv) != 0) { \
      size_t I; \
      for (I = 0; I < kv_size(__a##V->kv); I++) { \
        OBJ V = kv_A(__a##V->kv, I); \
        B \
      } \
    } \
  })

#define VM                   struct TrVM *vm
#define FRAME                (&vm->frames[vm->cf])

#define TR_NIL               ((OBJ)0)
#define TR_FALSE             ((OBJ)1)
#define TR_TRUE              ((OBJ)2)
#define TR_TEST(X)           ((X) == TR_NIL || (X) == TR_FALSE ? 0 : 1)
#define TR_BOOL(X)           ((X) ? TR_TRUE : TR_FALSE)
#define TR_SHIFT             8
#define TR_NUM_FLAG          0x01
#define INT2FIX(i)           (OBJ)((i) << TR_SHIFT | TR_NUM_FLAG)
#define FIX2INT(i)           (int)((i) >> TR_SHIFT)
#define TR_IS_FIX(x)         (((x) & TR_NUM_FLAG) == TR_NUM_FLAG)
#define TR_BOX(x)            (TR_IS_FIX(x) ? TrFixnum_new(vm, FIX2INT(x)) : (x))

#define TR_OBJECT_HEADER \
  TR_T type; \
  OBJ class; \
  OBJ metaclass; \
  OBJ ivars
#define TR_INIT_OBJ(T) ({ \
  Tr##T *o = TR_ALLOC(Tr##T); \
  o->type  = TR_T_##T; \
  o->class = vm->classes[TR_T_##T]; \
  o; \
})
#define TR_CLASS(T)           vm->classes[TR_T_##T]
#define TR_INIT_CLASS(T,S)    TR_CLASS(T) = TrClass_new(vm, tr_intern(#T), TR_CLASS(S))

#define tr_intern(S)         TrSymbol_new(vm, (S))
#define tr_raise(M,A...)     (printf("Error: "), printf(M, ##A), assert(0))
#define tr_def(C,N,F)        TrClass_add_method(vm, (C), tr_intern(N), TrMethod_new(vm, (TrFunc *)(F), TR_NIL))
#define tr_send(R,MSG,A...)  ({ \
  OBJ r = TR_BOX(R); \
  TrMethod *m = TR_CMETHOD(TrObject_method(vm, r, (MSG))); \
  if (!m) tr_raise("Method not found: %s\n", TR_STR_PTR(MSG)); \
  FRAME->method = m; \
  m->func(vm, r, ##A); \
})
#define tr_send2(R,STR,A...) tr_send((R), tr_intern(STR), ##A)

typedef unsigned long OBJ;

KHASH_MAP_INIT_STR(str, OBJ);
KHASH_MAP_INIT_INT(OBJ, OBJ);

typedef enum {
  TR_T_Object, TR_T_Class, TR_T_Method,
  TR_T_Symbol, TR_T_String,
  TR_T_Fixnum,
  TR_T_Array, TR_T_Hash,
  TR_T_Node,
  TR_T_MAX /* keep last */
} TR_T;

struct TrVM;

typedef struct {
  unsigned char i, a, b, c;
} TrInst;

typedef struct TrBlock {
  kvec_t(OBJ) k; /* TODO rename to values ? */
  kvec_t(char *) strings;
  kvec_t(OBJ) locals; /* TODO rename to local_names */
  kvec_t(TrInst) code;
  kvec_t(struct TrBlock *) blocks;
  size_t regc;
} TrBlock;

typedef OBJ (TrFunc)(VM, OBJ receiver, ...);
typedef struct {
  TR_OBJECT_HEADER;
  TrFunc *func;
  OBJ data;
} TrMethod;

typedef struct {
  struct TrBlock *block;
  TrMethod *method;  /* current called method */
  OBJ *params;
  OBJ *regs;
  OBJ *locals;
  OBJ self;
  OBJ class;
  OBJ fname;
  size_t line;
  TrInst *ip;
} TrFrame;

typedef struct TrVM {
  khash_t(str) *symbols;
  OBJ classes[TR_T_MAX];
  TrFrame frames[TR_MAX_FRAMES];
  size_t cf; /* current frame */
} TrVM;

typedef struct {
  TR_OBJECT_HEADER;
} TrObject;

typedef struct {
  TR_OBJECT_HEADER;
  OBJ name;
  OBJ super;
  khash_t(OBJ) *methods;
} TrClass;

typedef struct {
  TR_OBJECT_HEADER;
  char *ptr;
  size_t len;
  unsigned char interned:1;
} TrString;
typedef TrString TrSymbol;

typedef struct {
  TR_OBJECT_HEADER;
  int value;
} TrFixnum;

typedef struct {
  TR_OBJECT_HEADER;
  kvec_t(OBJ) kv;
} TrArray;

/* vm */
TrVM *TrVM_new();
void TrVM_destroy(TrVM *vm);
OBJ TrVM_run(VM, TrBlock *code);

/* string */
OBJ TrSymbol_new(VM, const char *str);
OBJ TrString_new(VM, const char *str, size_t len);
OBJ TrString_new2(VM, const char *str);
OBJ tr_sprintf(VM, const char *fmt, ...);
void TrSymbol_init(VM);
void TrString_init(VM);

/* number */
OBJ TrFixnum_new(VM, int value);
void TrFixnum_init(VM);

/* array */
OBJ TrArray_new(VM);
OBJ TrArray_new2(VM, int argc, ...);
void TrArray_init(VM);

/* object */
OBJ TrObject_new(VM);
OBJ TrObject_method(VM, OBJ self, OBJ name);
void TrObject_init(VM);

/* class */
OBJ TrClass_new(VM, OBJ name, OBJ super);
OBJ TrClass_lookup(VM, OBJ self, OBJ name);
OBJ TrClass_add_method(VM, OBJ self, OBJ name, OBJ method);
void TrClass_init(VM);
OBJ TrMethod_new(VM, TrFunc *func, OBJ data);

/* compiler */
TrBlock *TrBlock_compile(VM, char *code, char *fn, int trace);
void TrBlock_dump(VM, TrBlock *b);
void TrBlock_destroy(VM, TrBlock *b);

#endif /* _TINYRB_H_ */
