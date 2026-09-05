#ifndef BASE_TYPE_H
#define BASE_TYPE_H

// ak: External Includes
//=============================================================================

#include <stdint.h>
#include <stddef.h>

// ak: Base Types
//=============================================================================

#if LANGUAGE_C
    typedef _Bool bool;
    #define true  1
    #define false 0
#endif
typedef void Void_Proc(void);

// ak: Base Type Array ========================================================

typedef union U128 U128;
union U128
{
    uint8_t  u8[16];
    uint16_t u16[8];
    uint32_t u32[4];
    uint64_t u64[2];
    float    f32[4];
    double   f64[2];
};

typedef struct U8Array U8Array;
struct U8Array
{
    size_t length;
    size_t size;
    uint8_t  *v;
};
typedef struct U16Array U16Array;
struct U16Array
{
    size_t length;
    size_t size;
    uint16_t *v;
};
typedef struct U32Array U32Array;
struct U32Array
{
    size_t length;
    size_t size;
    uint32_t *v;
};
typedef struct U64Array U64Array;
struct U64Array
{
    size_t length;
    size_t size;
    uint64_t *v;
};

typedef struct I8Array I8Array;
struct I8Array
{
    size_t length;
    size_t size;
    int8_t  *v;
};
typedef struct I16Array I16Array;
struct I16Array
{
    size_t length;
    size_t size;
    int16_t *v;
};
typedef struct I32Array I32Array;
struct I32Array
{
    size_t length;
    size_t size;
    int32_t *v;
};
typedef struct I64Array I64Array;
struct I64Array
{
    size_t length;
    size_t size;
    int64_t *v;
};

typedef struct F32Array F32Array;
struct F32Array
{
    size_t length;
    size_t size;
    float   *v;
};
typedef struct F64Array F64Array;
struct F64Array
{
    size_t length;
    size_t size;
    double  *v;
};

// ak: Macros
//=============================================================================

#define Cast(T) (T)
#define Swap(T,a,b)   do{T t__ = a; a = b; b = t__;}while(0)
#define TypeOf(T)     __typeof__(T)

#if LANGUAGE_C
#   define STRUCT_ZERO {0}
#   define StructZeroType(T) (T)STRUCT_ZERO
#else
#   define STRUCT_ZERO {}
#   define StructZeroType(T) (T)STRUCT_ZERO
#endif

// NOTE(ak): MSVC C++ compiler does not support compound literals
// (C99 feature) Plain structures in C++ (without constructors) can be
// initialized with { } This is called aggregate initialization (C++11 feature)
#if LANGUAGE_CPP
    #define Literal(type)      type
#else
    #define Literal(type)      (type)
#endif

#define PtrFromInt(i) (void*)((uint8_t*)0 + (i))
#define IntFromPtr(ptr) ((uintptr_t)(ptr))

#define internal        static
#define global          static
#define local_persist   static

#if (COMPILER_MSVC || (COMPILER_CLANG && OS_WINDOWS)) && !(TOOLCHAIN_MINGW)
#   pragma section(".rdata$", read)
#   define read_only __declspec(allocate(".rdata$"))
#elif COMPILER_CLANG && OS_LINUX
#   define read_only __attribute__((section(".rodata")))
#else
#   define read_only
#endif

// ak: Array Macros ===========================================================

#define ArrayLength(a) (sizeof(a) / sizeof((a)[0]))
#if LANGUAGE_CPP
    #define array_alloc(a, T, s) ({ \
        T _arr = {}; \
        _arr.size = (size_t)(s); \
        _arr.length = 0; \
        _arr.v = (decltype(_arr.v))_arena_push((a), sizeof(_arr.v[0]) * (size_t)(s), AlignOf(decltype(_arr.v[0])), true); \
        _arr; \
    })
#else
    #define array_alloc(a, T, s) \
        (T){ \
            .size = (s), \
            .length = 0, \
            .v = arena_push((a), TypeOf(StructZeroType(T).v[0]), (s)) \
        }
#endif
#define array_append(array, data) (Assert((array)->length < (array)->size), (array)->v[(array)->length++] = (data))
#define array_get(array, index) (Assert((index) <= (array)->size), (array)->v[(index)])
#define carray_get(array, index) (Assert((size_t)index < ArrayLength(array)), array[(size_t)index])

// ak: Link Lists =============================================================

// ak: Linked List helper macros
#define CheckNil(nil,p) ((p) == 0 || (p) == nil)
#define SetNil(nil,p) ((p) = nil)

// ak: Singly-Linked, Singly-Headed List helpers
#define SLLStackPush(f,n) SLLStackPush_N(f,n,next)
#define SLLStackPop(f) SLLStackPop_N(f,next)

// ak: Singly-Linked, Singly-Headed Lists (Stacks)
#define SLLStackPush_N(f,n,next) ((n)->next=(f), (f)=(n))
#define SLLStackPop_N(f,next) ((f)=(f)->next)

// ak: Singly-Linked, Doubly-Headed List helpers
#define SLLQueuePush_N(f,l,n,next) SLLQueuePush_NZ(0,f,l,n,next)
#define SLLQueuePushFront_N(f,l,n,next) SLLQueuePushFront_NZ(0,f,l,n,next)
#define SLLQueuePop_N(f,l,next) SLLQueuePop_NZ(0,f,l,next)
#define SLLQueuePush(f,l,n) SLLQueuePush_NZ(0,f,l,n,next)
#define SLLQueuePushFront(f,l,n) SLLQueuePushFront_NZ(0,f,l,n,next)
#define SLLQueuePop(f,l) SLLQueuePop_NZ(0,f,l,next)

// ak: Doubly-Linked-Lists
#define DLLInsert_NPZ(nil,f,l,p,n,next,prev) (CheckNil(nil,f) ? \
((f) = (l) = (n), SetNil(nil,(n)->next), SetNil(nil,(n)->prev)) :\
CheckNil(nil,p) ? \
((n)->next = (f), (f)->prev = (n), (f) = (n), SetNil(nil,(n)->prev)) :\
((p)==(l)) ? \
((l)->next = (n), (n)->prev = (l), (l) = (n), SetNil(nil, (n)->next)) :\
(((!CheckNil(nil,p) && CheckNil(nil,(p)->next)) ? (0) : ((p)->next->prev = (n))), ((n)->next = (p)->next), ((p)->next = (n)), ((n)->prev = (p))))
#define DLLPushBack_NPZ(nil,f,l,n,next,prev) DLLInsert_NPZ(nil,f,l,l,n,next,prev)
#define DLLPushFront_NPZ(nil,f,l,n,next,prev) DLLInsert_NPZ(nil,l,f,f,n,prev,next)
#define DLLRemove_NPZ(nil,f,l,n,next,prev) (((n) == (f) ? (f) = (n)->next : (0)),\
((n) == (l) ? (l) = (l)->prev : (0)),\
(CheckNil(nil,(n)->prev) ? (0) :\
((n)->prev->next = (n)->next)),\
(CheckNil(nil,(n)->next) ? (0) :\
((n)->next->prev = (n)->prev)))

// ak: Singly-Linked, Doubly-Headed Lists (Queues)
#define SLLQueuePush_NZ(nil,f,l,n,next) (CheckNil(nil,f)?\
((f)=(l)=(n),SetNil(nil,(n)->next)):\
((l)->next=(n),(l)=(n),SetNil(nil,(n)->next)))
#define SLLQueuePushFront_NZ(nil,f,l,n,next) (CheckNil(nil,f)?\
((f)=(l)=(n),SetNil(nil,(n)->next)):\
((n)->next=(f),(f)=(n)))
#define SLLQueuePop_NZ(nil,f,l,next) ((f)==(l)?\
(SetNil(nil,f),SetNil(nil,l)):\
((f)=(f)->next))

// ak: Doubly-Linked-List helpers
#define DLLInsert_NP(f,l,p,n,next,prev) DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define DLLPushBack_NP(f,l,n,next,prev) DLLPushBack_NPZ(0,f,l,n,next,prev)
#define DLLPushFront_NP(f,l,n,next,prev) DLLPushFront_NPZ(0,f,l,n,next,prev)
#define DLLRemove_NP(f,l,n,next,prev) DLLRemove_NPZ(0,f,l,n,next,prev)
#define DLLInsert(f,l,p,n) DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define DLLPushBack(f,l,n) DLLPushBack_NPZ(0,f,l,n,next,prev)
#define DLLPushFront(f,l,n) DLLPushFront_NPZ(0,f,l,n,next,prev)
#define DLLRemove(f,l,n) DLLRemove_NPZ(0,f,l,n,next,prev)

// ak: Constants
//=============================================================================

global uint64_t max_u64 = 0xffffffffffffffffull;
global uint32_t max_u32 = 0xffffffff;
global uint16_t max_u16 = 0xffff;
global uint8_t  max_u8  = 0xff;

global uint64_t min_u64 = 0;
global uint32_t min_u32 = 0;
global uint16_t min_u16 = 0;
global uint8_t  min_u8  = 0;

global int64_t max_i64 = (int64_t)0x7fffffffffffffffull;
global int32_t max_i32 = (int32_t)0x7fffffff;
global int16_t max_i16 = (int16_t)0x7fff;
global int8_t  max_i8  =  (int8_t)0x7f;

global int64_t min_i64 = (int64_t)0xffffffffffffffffull;
global int32_t min_i32 = (int32_t)0xffffffff;
global int16_t min_i16 = (int16_t)0xffff;
global int8_t  min_i8  =  (int8_t)0xff;

read_only global uint8_t integer_symbol_reverse[128] =
{
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
};

// ak: Functions
//=============================================================================

// ak: Safe Casts =============================================================

internal uint16_t safe_cast_u16(uint32_t x);
internal uint32_t safe_cast_u32(uint64_t x);
internal int32_t  safe_cast_i32(int64_t x);
internal uint32_t u32_from_u64_saturate(uint64_t x);

#endif // BASE_TYPE_H
