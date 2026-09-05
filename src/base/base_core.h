#ifndef BASE_CORE_H
#define BASE_CORE_H

#if !defined(BUILD_DEBUG)
#   define BUILD_DEBUG 0
#endif

// ak: External Includes
//=============================================================================

#include <stdarg.h>

// ak: Code Keywords
//=============================================================================

#if COMPILER_CLANG || COMPILER_GCC
#   define FILE_NAME __FILE_NAME__
#else
#   define FILE_NAME __FILE__
#endif
#define LINE_NUMBER __LINE__

#if COMPILER_MSVC
#   define thread_static __declspec(thread)
#elif COMPILER_CLANG || COMPILER_GCC
#   define thread_static __thread
#else
#   error thread_static not defined for this compiler.
#endif

#if COMPILER_MSVC
#   define force_inline __forceinline
#elif COMPILER_CLANG || COMPILER_GCC
#   define force_inline __attribute__((always_inline))
#else
#   error force_inline not defined for this compiler.
#endif

#if COMPILER_MSVC
#   define no_inline __declspec(noinline)
#elif COMPILER_CLANG || COMPILER_GCC
#   define no_inline __attribute__((noinline))
#else
#   error no_inline not defined for this compiler.
#endif

#if COMPILER_MSVC
#   define PragmaPop() __pragma(warning(pop))
#elif COMPILER_CLANG || COMPILER_GCC
#   define PragmaPop() _Pragma("GCC diagnostic pop")
#else
#   error pragma pop not defined for this compiler
#endif

#if COMPILER_MSVC
#   define PragmaNoWarnMissingFieldInitPush() \
        __pragma(warning(push))
#elif COMPILER_CLANG || COMPILER_GCC
#   define PragmaNoWarnMissingFieldInitPush() \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"")
#else
#   error pragma missing field initializers not defined for this compiler
#endif

// ak: Constants
//=============================================================================

global const uint32_t bitmask1  = 0x00000001;
global const uint32_t bitmask2  = 0x00000003;
global const uint32_t bitmask3  = 0x00000007;
global const uint32_t bitmask4  = 0x0000000f;
global const uint32_t bitmask5  = 0x0000001f;
global const uint32_t bitmask6  = 0x0000003f;
global const uint32_t bitmask7  = 0x0000007f;
global const uint32_t bitmask8  = 0x000000ff;
global const uint32_t bitmask9  = 0x000001ff;
global const uint32_t bitmask10 = 0x000003ff;
global const uint32_t bitmask11 = 0x000007ff;
global const uint32_t bitmask12 = 0x00000fff;
global const uint32_t bitmask13 = 0x00001fff;
global const uint32_t bitmask14 = 0x00003fff;
global const uint32_t bitmask15 = 0x00007fff;
global const uint32_t bitmask16 = 0x0000ffff;
global const uint32_t bitmask17 = 0x0001ffff;
global const uint32_t bitmask18 = 0x0003ffff;
global const uint32_t bitmask19 = 0x0007ffff;
global const uint32_t bitmask20 = 0x000fffff;
global const uint32_t bitmask21 = 0x001fffff;
global const uint32_t bitmask22 = 0x003fffff;
global const uint32_t bitmask23 = 0x007fffff;
global const uint32_t bitmask24 = 0x00ffffff;
global const uint32_t bitmask25 = 0x01ffffff;
global const uint32_t bitmask26 = 0x03ffffff;
global const uint32_t bitmask27 = 0x07ffffff;
global const uint32_t bitmask28 = 0x0fffffff;
global const uint32_t bitmask29 = 0x1fffffff;
global const uint32_t bitmask30 = 0x3fffffff;
global const uint32_t bitmask31 = 0x7fffffff;
global const uint32_t bitmask32 = 0xffffffff;
global const uint64_t bitmask33 = 0x00000001ffffffffull;
global const uint64_t bitmask34 = 0x00000003ffffffffull;
global const uint64_t bitmask35 = 0x00000007ffffffffull;
global const uint64_t bitmask36 = 0x0000000fffffffffull;
global const uint64_t bitmask37 = 0x0000001fffffffffull;
global const uint64_t bitmask38 = 0x0000003fffffffffull;
global const uint64_t bitmask39 = 0x0000007fffffffffull;
global const uint64_t bitmask40 = 0x000000ffffffffffull;
global const uint64_t bitmask41 = 0x000001ffffffffffull;
global const uint64_t bitmask42 = 0x000003ffffffffffull;
global const uint64_t bitmask43 = 0x000007ffffffffffull;
global const uint64_t bitmask44 = 0x00000fffffffffffull;
global const uint64_t bitmask45 = 0x00001fffffffffffull;
global const uint64_t bitmask46 = 0x00003fffffffffffull;
global const uint64_t bitmask47 = 0x00007fffffffffffull;
global const uint64_t bitmask48 = 0x0000ffffffffffffull;
global const uint64_t bitmask49 = 0x0001ffffffffffffull;
global const uint64_t bitmask50 = 0x0003ffffffffffffull;
global const uint64_t bitmask51 = 0x0007ffffffffffffull;
global const uint64_t bitmask52 = 0x000fffffffffffffull;
global const uint64_t bitmask53 = 0x001fffffffffffffull;
global const uint64_t bitmask54 = 0x003fffffffffffffull;
global const uint64_t bitmask55 = 0x007fffffffffffffull;
global const uint64_t bitmask56 = 0x00ffffffffffffffull;
global const uint64_t bitmask57 = 0x01ffffffffffffffull;
global const uint64_t bitmask58 = 0x03ffffffffffffffull;
global const uint64_t bitmask59 = 0x07ffffffffffffffull;
global const uint64_t bitmask60 = 0x0fffffffffffffffull;
global const uint64_t bitmask61 = 0x1fffffffffffffffull;
global const uint64_t bitmask62 = 0x3fffffffffffffffull;
global const uint64_t bitmask63 = 0x7fffffffffffffffull;
global const uint64_t bitmask64 = 0xffffffffffffffffull;

global const uint32_t bit1  = (1<<0);
global const uint32_t bit2  = (1<<1);
global const uint32_t bit3  = (1<<2);
global const uint32_t bit4  = (1<<3);
global const uint32_t bit5  = (1<<4);
global const uint32_t bit6  = (1<<5);
global const uint32_t bit7  = (1<<6);
global const uint32_t bit8  = (1<<7);
global const uint32_t bit9  = (1<<8);
global const uint32_t bit10 = (1<<9);
global const uint32_t bit11 = (1<<10);
global const uint32_t bit12 = (1<<11);
global const uint32_t bit13 = (1<<12);
global const uint32_t bit14 = (1<<13);
global const uint32_t bit15 = (1<<14);
global const uint32_t bit16 = (1<<15);
global const uint32_t bit17 = (1<<16);
global const uint32_t bit18 = (1<<17);
global const uint32_t bit19 = (1<<18);
global const uint32_t bit20 = (1<<19);
global const uint32_t bit21 = (1<<20);
global const uint32_t bit22 = (1<<21);
global const uint32_t bit23 = (1<<22);
global const uint32_t bit24 = (1<<23);
global const uint32_t bit25 = (1<<24);
global const uint32_t bit26 = (1<<25);
global const uint32_t bit27 = (1<<26);
global const uint32_t bit28 = (1<<27);
global const uint32_t bit29 = (1<<28);
global const uint32_t bit30 = (1<<29);
global const uint32_t bit31 = (1<<30);
global const uint32_t bit32 = (1<<31);
global const uint64_t bit33 = (1ull<<32);
global const uint64_t bit34 = (1ull<<33);
global const uint64_t bit35 = (1ull<<34);
global const uint64_t bit36 = (1ull<<35);
global const uint64_t bit37 = (1ull<<36);
global const uint64_t bit38 = (1ull<<37);
global const uint64_t bit39 = (1ull<<38);
global const uint64_t bit40 = (1ull<<39);
global const uint64_t bit41 = (1ull<<40);
global const uint64_t bit42 = (1ull<<41);
global const uint64_t bit43 = (1ull<<42);
global const uint64_t bit44 = (1ull<<43);
global const uint64_t bit45 = (1ull<<44);
global const uint64_t bit46 = (1ull<<45);
global const uint64_t bit47 = (1ull<<46);
global const uint64_t bit48 = (1ull<<47);
global const uint64_t bit49 = (1ull<<48);
global const uint64_t bit50 = (1ull<<49);
global const uint64_t bit51 = (1ull<<50);
global const uint64_t bit52 = (1ull<<51);
global const uint64_t bit53 = (1ull<<52);
global const uint64_t bit54 = (1ull<<53);
global const uint64_t bit55 = (1ull<<54);
global const uint64_t bit56 = (1ull<<55);
global const uint64_t bit57 = (1ull<<56);
global const uint64_t bit58 = (1ull<<57);
global const uint64_t bit59 = (1ull<<58);
global const uint64_t bit60 = (1ull<<59);
global const uint64_t bit61 = (1ull<<60);
global const uint64_t bit62 = (1ull<<61);
global const uint64_t bit63 = (1ull<<62);
global const uint64_t bit64 = (1ull<<63);

// ak: Macros
//=============================================================================

#define Unused(var)     (void)var
#define TempUnused(var) Unused(var)
#define Min(A,B)        (((A)<(B))?(A):(B))
#define Max(A,B)        (((A)>(B))?(A):(B))
#define Clamp(A,X,B)    (((X)<(A))?(A):((X)>(B))?(B):(X))
#define IsPow2(x)       ((x != 0) && ((x & (x - 1)) == 0))
#define AlignPow2(x,b)  (((x) + (b) - 1)&(~((b) - 1)))
#define OffsetOf(T,m)   offsetof(T, m)

// ak: Branch Predictor Hints =================================================

#if COMPILER_CLANG || COMPILER_GCC
#   define Expect(expr, val) __builtin_expect((expr), (val))
#else
#   define Expect(expr, val) (expr)
#endif

#define Likely(expr)            Expect(expr,1)
#define Unlikely(expr)          Expect(expr,0)

// ak: For-Loop Construct =====================================================

#define DeferLoop(begin, end)      for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))
#define DeferLoopCheck(begin, end) for(int _i_ = 2 * !(begin); (_i_ == 2 ? ((end), 0) : !_i_); _i_ += 1, (end))

#define EachIndex(it, count)       (size_t it = 0; it < (count); it += 1)
#define EachElement(it, array)     (size_t it = 0; it < ArrayLength(array); it += 1)
#define EachEnumVal(T, it)         (T it = (T)0; it < T##_COUNT; it = (T)(it+1))
#define EachNonZeroEnumVal(T, it)  (T it = (T)1; it < T##_COUNT; it = (T)(it+1))
#define EachInRange(it, range)     (size_t it = (range).min; it < (range).max; it += 1)
#define EachNode(it, T, first)     (T *it = first; it != 0; it = it->next)
#define EachBit(it, flags)         (size_t (_i_) = (flags), it = (flags) & -(flags); (_i_) != 0; (_i_) &= ((_i_) - 1), it = (flags) & -(flags))

// ak: Alignment ==============================================================

#if COMPILER_MSVC
#   define AlignOf(T) __alignof(T)
#elif COMPILER_CLANG
#   define AlignOf(T) __alignof(T)
#elif COMPILER_GCC
#   define AlignOf(T) __alignof__(T)
#else
#   error AlignOf not defined for this compiler.
#endif

// ak: Asserts ================================================================

#if COMPILER_MSVC
#   define Break() __debugbreak()
#elif COMPILER_CLANG || COMPILER_GCC
#   define Break() __builtin_trap()
#elif COMPILER_TCC
#   define Break() asm volatile("ud2")
#else
#   error Unknown trap intrinsic for this compiler.
#endif

#define AssertAlways(x) ((void)(!(x) ? (Break(), 0) : 0))
#if BUILD_DEBUG
#   define Assert(x) AssertAlways(x)
#else
#   define Assert(x) (void)(x)
#endif
#define UNREACHABLE(msg) Assert(!"" msg)
#define TODO(msg)        Assert(!msg)

// ak: ak: Linkage Keyword Macros =============================================

#if OS_WINDOWS
#   define shared_function C_LINKAGE __declspec(dllexport)
#else
#   define shared_function C_LINKAGE
#endif

#if LANGUAGE_CPP
#   define C_LINKAGE_BEGIN extern "C"{
#   define C_LINKAGE_END }
#   define C_LINKAGE extern "C"
#else
#   define C_LINKAGE_BEGIN
#   define C_LINKAGE_END
#   define C_LINKAGE
#endif

// ak: Address Sanitizer Markup ===============================================

#if ASAN_ENABLED
    C_LINKAGE void __asan_poison_memory_region(void const volatile *addr, size_t size);
    C_LINKAGE void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
#   define AsanPoisonMemoryRegion(addr, size)   __asan_poison_memory_region((addr), (size))
#   define AsanUnpoisonMemoryRegion(addr, size) __asan_unpoison_memory_region((addr), (size))
#else
#   define AsanPoisonMemoryRegion(addr, size)   ((void)(addr), (void)(size))
#   define AsanUnpoisonMemoryRegion(addr, size) ((void)(addr), (void)(size))
#endif

// ak: Image
//=============================================================================

typedef enum Img_Format
{
    Img_Format_R8,
    Img_Format_RG8,
    Img_Format_RGBA8,
    Img_Format_BGRA8,
    Img_Format_R16,
    Img_Format_RGBA16,
    Img_Format_R32,
    Img_Format_RG32,
    Img_Format_RGBA32,
} Img_Format;

typedef struct Img Img;
struct Img
{
    void *pixels;
    unsigned int width;
    unsigned int height;
    Img_Format format;
};

typedef enum Corner
{
    Corner_Invalid = -1,
    Corner_TopLeft,
    Corner_BottomLeft,
    Corner_TopRight,
    Corner_BottomRight,
    Corner_COUNT
} Corner;

// ak: Text 2D Coordinates & Ranges ===========================================

typedef struct Txt_Pt Txt_Pt;
struct Txt_Pt
{
    size_t line;
    size_t column;
};

typedef struct Txt_Rng Txt_Rng;
struct Txt_Rng
{
    Txt_Pt min;
    Txt_Pt max;
};

// ak: Axis ===================================================================

typedef enum Axis_2d
{
    Axis_2d_X,
    Axis_2d_Y,
    Axis_2d_COUNT,
}
Axis_2d;
#define Axis_2d_Flip(a) ((Axis_2d)(!(a)))

typedef enum Axis_3d
{
    Axis_3d_X,
    Axis_3d_Y,
    Axis_3d_Z,
    Axis_3d_COUNT,
}
Axis_3d;

// ak: Functions
//=============================================================================

internal void base_main(void);

#endif // BASE_CORE_H
