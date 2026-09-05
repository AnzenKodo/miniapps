#ifndef BASE_MEM_H
#define BASE_MEM_H

// NOTE(ak): references taken from:
//  - [Ginger Bill](https://www.gingerbill.org): [gb.h](https://github.com/gingerBill/gb/blob/master/gb.h)

internal inline void *mem_copy(void *dest, void const *source, size_t n);
#define MemCopyStruct(d,s)  mem_copy((d),(s),sizeof(*(d)))
#define MemCopyArray(d,s)   mem_copy((d),(s),sizeof(d))
#define MemCopyTyped(d,s,c) mem_copy((d),(s),sizeof(*(d))*(c))
#define MemCopyStr8(dst, s) mem_copy(dst, (s).str, (s).size)

internal inline void *mem_set(void *dest, uint8_t c, size_t n);
#define MemSetZero(s,z)       mem_set((s),0,(z))
#define MemSetZeroStruct(s)   MemSetZero((s),sizeof(*(s)))
#define MemSetZeroArray(a)    MemSetZero((a),sizeof(a))
#define MemSetZeroTyped(m,c)  MemSetZero((m),sizeof(*(m))*(c))

internal inline int32_t mem_cmp(void const *s1, void const *s2, size_t size);
#define MemMatch(a,b,z)       (mem_cmp((a),(b),(z)) == 0)
#define MemMatchStruct(a,b)   MemMatch((a),(b),sizeof(*(a)))
#define MemMatchStructZero(a) ({ TypeOf(a) _z = STRUCT_ZERO; MemMatchStruct(&a, &_z); })
#define MemMatchArray(a,b)    MemMatch((a),(b),sizeof(a))

#define MemFromOffset(T,ptr,off) (T)((((uint8_t *)ptr)+(off)))
#define MemFromPtr(T,ptr,m)      (void*)((((uint8_t *)ptr)+OffsetOf(T,m)))

internal inline void *mem_move(void *dest, void const *source, size_t n);
internal int32_t mem_is_zero(void *ptr, size_t size);

#endif // BASE_MEM_H
