#ifndef BASE_MATH_H
#define BASE_MATH_H

// NOTE(ak): the code reference taken from:
// - https://github.com/gingerBill/gb/blob/master/gb_math.h
// - https://github.com/HandmadeMath/HandmadeMath
// - https://github.com/EpicGamesExt/raddebugger

// ak: Base
//=============================================================================

typedef enum Base
{
    Base_2  = 2,
    Base_8  = 8,
    Base_10 = 10,
    Base_16 = 16,
} Base;

// ak: Vector Types
//=============================================================================

// ak: 2-Vectors ==============================================================

typedef union Vec2_I16 Vec2_I16;
union Vec2_I16
{
    struct
    {
        int16_t x;
        int16_t y;
    };
    int16_t v[2];
};

typedef union Vec2_I32 Vec2_I32;
union Vec2_I32
{
    struct
    {
        int32_t x;
        int32_t y;
    };
    int32_t v[2];
};

typedef union Vec2_I64 Vec2_I64;
union Vec2_I64
{
    struct
    {
        int64_t x;
        int64_t y;
    };
    int64_t v[2];
};

typedef union Vec2_F32 Vec2_F32;
union Vec2_F32
{
    struct
    {
        float x;
        float y;
    };
    float v[2];
};

// ak: 3-Vectors ==============================================================

typedef union Vec3_I32 Vec3_I32;
union Vec3_I32
{
    struct
    {
        int32_t x;
        int32_t y;
        int32_t z;
    };
    struct
    {
        Vec2_I32 xy;
        int32_t _z0;
    };
    struct
    {
        int32_t _x0;
        Vec2_I32 yz;
    };
    int32_t v[3];
};

typedef union Vec3_F32 Vec3_F32;
union Vec3_F32
{
    struct
    {
        float x;
        float y;
        float z;
    };
    struct
    {
        Vec2_F32 xy;
        float _z0;
    };
    struct
    {
        float _x0;
        Vec2_F32 yz;
    };
    float v[3];
};

// ak: 4-vectors ==============================================================

typedef union Vec4_I32 Vec4_I32;
union Vec4_I32
{
    struct
    {
        int32_t x;
        int32_t y;
        int32_t z;
        int32_t w;
    };
    struct
    {
        Vec2_I32 xy;
        Vec2_I32 zw;
    };
    struct
    {
        Vec3_I32 xyz;
        int32_t _z0;
    };
    struct
    {
        int32_t _x0;
        Vec3_I32 yzw;
    };
    int32_t v[4];
};

typedef union Vec4_F32 Vec4_F32;
union Vec4_F32
{
    struct
    {
        float x;
        float y;
        float z;
        float w;
    };
    struct
    {
        Vec2_F32 xy;
        Vec2_F32 zw;
    };
    struct
    {
        Vec3_F32 xyz;
        float _z0;
    };
    struct
    {
        float _x0;
        Vec3_F32 yzw;
    };
    float v[4];
};

// ak: Matrix Types
//=============================================================================

typedef struct Mat3x3_F32 Mat3x3_F32;
struct Mat3x3_F32
{
    float v[3][3];
};

typedef struct Mat4x4_F32 Mat4x4_F32;
struct Mat4x4_F32
{
    float v[4][4];
};

// ak: Range Types
//=============================================================================

// ak: 1D Range ===============================================================

typedef union Rng1_U8 Rng1_U8;
union Rng1_U8
{
    struct
    {
        uint32_t min;
        uint32_t max;
    };
    uint32_t v[2];
};

typedef union Rng1_U16 Rng1_U16;
union Rng1_U16
{
    struct
    {
        uint32_t min;
        uint32_t max;
    };
    uint32_t v[2];
};

typedef union Rng1_U32 Rng1_U32;
union Rng1_U32
{
    struct
    {
        uint32_t min;
        uint32_t max;
    };
    uint32_t v[2];
};

typedef union Rng1_I32 Rng1_I32;
union Rng1_I32
{
    struct
    {
        int32_t min;
        int32_t max;
    };
    int32_t v[2];
};

typedef union Rng1_U64 Rng1_U64;
union Rng1_U64
{
    struct
    {
        uint64_t min;
        uint64_t max;
    };
    uint64_t v[2];
};

typedef union Rng1_I64 Rng1_I64;
union Rng1_I64
{
    struct
    {
        int64_t min;
        int64_t max;
    };
    int64_t v[2];
};

typedef union Rng1_F32 Rng1_F32;
union Rng1_F32
{
    struct
    {
        float min;
        float max;
    };
    float v[2];
};

// ak: 2D Range (Rectangles) ===================================================

typedef union Rng2_I16 Rng2_I16;
union Rng2_I16
{
    struct
    {
        Vec2_I16 min;
        Vec2_I16 max;
      };
    struct
    {
        Vec2_I16 p0;
        Vec2_I16 p1;
    };
    struct
    {
        int16_t x0;
        int16_t y0;
        int16_t x1;
        int16_t y1;
    };
    Vec2_I16 v[2];
};

typedef union Rng2_I32 Rng2_I32;
union Rng2_I32
{
    struct
    {
        Vec2_I32 min;
        Vec2_I32 max;
    };
    struct
    {
        Vec2_I32 p0;
        Vec2_I32 p1;
    };
    struct
    {
        int32_t x0;
        int32_t y0;
        int32_t x1;
        int32_t y1;
    };
    Vec2_I32 v[2];
};

typedef union Rng2_F32 Rng2_F32;
union Rng2_F32
{
    struct
    {
        Vec2_F32 min;
        Vec2_F32 max;
    };
    struct
    {
        Vec2_F32 p0;
        Vec2_F32 p1;
    };
    struct
    {
        float x0;
        float y0;
        float x1;
        float y1;
    };
    Vec2_F32 v[2];
};

typedef union Rng2_I64 Rng2_I64;
union Rng2_I64
{
    struct
    {
        Vec2_I64 min;
        Vec2_I64 max;
    };
    struct
    {
        Vec2_I64 p0;
        Vec2_I64 p1;
    };
    struct
    {
        int64_t x0;
        int64_t y0;
        int64_t x1;
        int64_t y1;
    };
    Vec2_I64 v[2];
};

// ak: Units
//=============================================================================

// ak: Numerical Units
#define Thousand(n)   ((n)*1000)
#define Million(n)    ((n)*1000000)
#define Billion(n)    ((n)*1000000000)

// ak: Unit of Information
#define KB(n)  (((uint64_t)(n)) << 10)
#define MB(n)  (((uint64_t)(n)) << 20)
#define GB(n)  (((uint64_t)(n)) << 30)
#define TB(n)  (((uint64_t)(n)) << 40)

// ak: Constants
//=============================================================================

#define EPSILON_F32      1.19209290e-7f
#define ZERO_F32         0.0f
#define ONE_F32          1.0f
#define TWO_THIRDS_F32   0.666666666666666666666666666666666666667f
#define TAU_F32          6.28318530717958647692528676655900576f
#define PI_F32           3.14159265358979323846264338327950288f
#define ONE_OVER_TAU_F32 0.159154943091895335768883763372514362f
#define ONE_OVER_PI_F32  0.318309886183790671537767526745028724f
#define TAU_OVER_2_F32   3.14159265358979323846264338327950288f
#define TAU_OVER_4_F32   1.570796326794896619231321691639751442f
#define TAU_OVER_8_F32   0.785398163397448309615660845819875721f
#define E_F32            2.7182818284590452353602874713526625f
#define SQRT_TWO_F32     1.41421356237309504880168872420969808f
#define SQRT_THREE_F32   1.73205080756887729352744634150587236f
#define SQRT_FIVE_F32    2.23606797749978969640917366873127623f
#define LOG_TWO_F32      0.693147180559945309417232121458176568f
#define LOG_TEN_F32      2.30258509299404568401799145468436421f

#define EPSILON_F64      1.19209290e-7
#define ZERO_F64         0.0
#define ONE_F64          1.0
#define TWO_THIRDS_F64   0.666666666666666666666666666666666666667
#define TAU_F64          6.28318530717958647692528676655900576
#define PI_F64           3.14159265358979323846264338327950288
#define ONE_OVER_TAU_F64 0.159154943091895335768883763372514362
#define ONE_OVER_PI_F64  0.318309886183790671537767526745028724
#define TAU_OVER_2_F64   3.14159265358979323846264338327950288
#define TAU_OVER_4_F64   1.570796326794896619231321691639751442
#define TAU_OVER_8_F64   0.785398163397448309615660845819875721
#define E_F64            2.7182818284590452353602874713526625
#define SQRT_TWO_F64     1.41421356237309504880168872420969808
#define SQRT_THREE_F64   1.73205080756887729352744634150587236
#define SQRT_FIVE_F64    2.23606797749978969640917366873127623
#define LOG_TWO_F64      0.693147180559945309417232121458176568
#define LOG_TEN_F64      2.30258509299404568401799145468436421

// ak: Scalar Math Ops
//=============================================================================

#define IsPow2(x)      ((x != 0) && ((x & (x - 1)) == 0))
#define Min(A,B)       (((A)<(B))?(A):(B))
#define Max(A,B)       (((A)>(B))?(A):(B))
#define Min3(a, b, c)  Min(Min(a, b), c)
#define Max3(a, b, c)  Max(Max(a, b), c)
#define Clamp(A,X,B)   (((X)<(A))?(A):((X)>(B))?(B):(X))
#define Square(x)      ((x)*(x))
#define Cube(x)        ((x)*(x)*(x))
#define Abs(x)         ((x) > 0 ? (x) : -(x))
#define Sign(x)        ((x) >= 0 ? 1 : -1)
#define Mod(a, m)      (((a) % (m)) >= 0 ? ((a) % (m)) : (((a) % (m)) + (m)))

// NOTE(ak): function list comes: https://en.wikipedia.org/wiki/C_mathematical_functions

internal float remainder_f32(float x, float y);
internal float mod_f32(float x, float y);

internal double remainder_f64(double x, double y);
internal double mod_f64(double x, double y);

// TODO(ak): more scalar math ops functions
// - div       computes the quotient and remainder of integer division
// - remquo    signed remainder as well as the three last bits of the division operation
// - fma       fused multiply-add operation
// - fdim      positive difference of two floating-point values

// ak: Exponential functions ==================================================

internal float exp_f32(float a);
internal float exp2_f32(float x);
internal float log_f32(float a);
internal float log2_f32(float x);

internal double exp_f64(double a);
internal double exp2_f64(double x);
internal double log_f64(double a);
internal double log2_f64(double x);

// TODO(ak): more exponential functions
// - expm1	returns e raised to the given power, minus one
// - log10	computes common logarithm (to base 10)
// - log1p	computes natural logarithm (to base e) of 1 plus the given number
// - ilogb	extracts exponent of the number
// - logb	extracts exponent of the number

// ak: Power functions ========================================================

internal float sqrt_f32(float number);
internal float rsqrt_f32(float a);
internal float pow_f32(float a, float b);

internal double sqrt_f64(double number);
internal double rsqrt_f64(double a);
internal double pow_f64(double a, double b);

// TODO(ak): more power functions
// - cbrt
// - hypot

// ak: Trigonometric functions ================================================

internal float sin_f32(float a);
internal float cos_f32(float a);
internal float tan_f32(float radians);
internal float asin_f32(float a);
internal float acos_f32(float a);
internal float atan_f32(float a);
internal float atan2_f32(float y, float x);

internal double sin_f64(double a);
internal double cos_f64(double a);
internal double tan_f64(double radians);
internal double asin_f64(double a);
internal double acos_f64(double a);
internal double atan_f64(double a);
internal double atan2_f64(double y, double x);

// ak: Hyperbolic functions ===================================================

// TODO(ak): more hyperbolic functions
// - sinh	computes hyperbolic sine
// - cosh	computes hyperbolic cosine
// - tanh	computes hyperbolic tangent
// - asinh	computes hyperbolic arc sine
// - acosh	computes hyperbolic arc cosine
// - atanh	computes hyperbolic arc tangent

// ak: Gamma functions ========================================================

// TODO(ak): more gamma functions
// - lgamma	computes natural logarithm of the absolute value of the gamma function
// - tgamma	computes gamma function

// ak: Nearest integer floating-point operations ==============================

internal float round_f32(float x);
internal float floor_f32(float x);
internal float ceil_f32(float x);

internal double round_f64(double x);
internal double floor_f64(double x);
internal double ceil_f64(double x);

// TODO(ak): more nearest integer floating-point operations functions
// - trunc     returns the nearest integer not greater in magnitude than the given value
// - nearbyint returns the nearest integer using current rounding mode
// - rint      returns the nearest integer using current rounding mode with exception if the result differs

// ak: Floating-point manipulation functions ==================================

internal float copysign_f32(float x, float y);

internal double copysign_f64(double x, double y);

// TODO(ak): more floating-point manipulation functions functions
// - frexp	decomposes a number into significand and a power of 2
// - ldexp	multiplies a number by 2 raised to a power
// - modf	decomposes a number into integer and fractional parts
// - scalbn
// - scalbln	multiplies a number by FLT_RADIX raised to a power
// - nextafter
// - nexttoward	returns next representable floating-point value towards the given value
// ak: Vector Ops
//=============================================================================


// ak: 2-Vectors ==============================================================

// 2-Vector I16
internal Vec2_I16 scale_vec2_i16(Vec2_I16 v, int16_t s);
internal Vec2_I16 add_vec2_i16(Vec2_I16 a, Vec2_I16 b);

// 2-Vector I32
internal Vec2_I32 scale_vec2_i32(Vec2_I32 v, int32_t s);
internal Vec2_I32 add_vec2_i32(Vec2_I32 v, Vec2_I32 s);

// 2-Vector I64
internal Vec2_I64 scale_vec2_i64(Vec2_I64 v, int64_t s);
internal Vec2_I64 add_vec2_i64(Vec2_I64 a, Vec2_I64 b);

// 2-Vector F32
internal Vec2_F32 scale_vec2_f32(Vec2_F32 v, float s);
internal Vec2_F32 add_vec2_f32(Vec2_F32 a, Vec2_F32 b);

#if LANGUAGE_CPP
    inline Vec2_I16 scale_vec2(Vec2_I16 v, int16_t s) { return scale_vec2_i16(v, s); }
    inline Vec2_I32 scale_vec2(Vec2_I32 v, int32_t s) { return scale_vec2_i32(v, s); }
    inline Vec2_I64 scale_vec2(Vec2_I64 v, int64_t s) { return scale_vec2_i64(v, s); }
    inline Vec2_F32 scale_vec2(Vec2_F32 v, float s)   { return scale_vec2_f32(v, s); }
    
    inline Vec2_I16 add_vec2(Vec2_I16 a, Vec2_I16 b) { return add_vec2_i16(a, b); }
    inline Vec2_I32 add_vec2(Vec2_I32 a, Vec2_I32 b) { return add_vec2_i32(a, b); }
    inline Vec2_I64 add_vec2(Vec2_I64 a, Vec2_I64 b) { return add_vec2_i64(a, b); }
    inline Vec2_F32 add_vec2(Vec2_F32 a, Vec2_F32 b) { return add_vec2_f32(a, b); }
#else
    #define scale_vec2(v, s) _Generic((v), \
        Vec2_I16: scale_vec2_i16,  \
        Vec2_I32: scale_vec2_i32, \
        Vec2_I64: scale_vec2_i64, \
        Vec2_F32: scale_vec2_f32  \
    )(v, s)
    #define add_vec2(v, s) _Generic((v), \
        Vec2_I16: add_vec2_i16,  \
        Vec2_I32: add_vec2_i32, \
        Vec2_I64: add_vec2_i64, \
        Vec2_F32: add_vec2_f32  \
    )(v, s)
#endif

// ak: 3-Vectors ==============================================================

// ak: 4-vectors ==============================================================

// ak: Range Ops
//=============================================================================

// ak: 1D Range ===============================================================

// ak: 1D Range U8
internal Rng1_U8 rng1_u8(uint8_t min, uint8_t max);
internal uint8_t dim_rng1_u8(Rng1_U8 r);

// ak: 1D Range U16
internal Rng1_U16 rng1_u16(uint16_t min, uint16_t max);
internal uint16_t dim_rng1_u16(Rng1_U16 r);

// ak: 1D Range U32
internal Rng1_U32 rng1_u32(uint32_t min, uint32_t max);
internal uint32_t dim_rng1_u32(Rng1_U32 r);

// ak: 1D Range I32
internal Rng1_I32 rng1_i32(int32_t min, int32_t max);
internal int32_t dim_rng1_i32(Rng1_I32 r);

// ak: 1D Range U64
internal Rng1_U64 rng1_u64(uint64_t min, uint64_t max);
internal uint64_t dim_rng1_u64(Rng1_U64 r);

// ak: 1D Range I64
internal Rng1_I64 rng1_i64(int64_t min, int64_t max);
internal int64_t dim_rng1_i64(Rng1_I64 r);

// ak: 1D Range F32
internal Rng1_F32 rng1_f32(float min, float max);
internal float dim_rng1_f32(Rng1_F32 r);

// ak: 1D Rng Macros

#if LANGUAGE_CPP
    inline Rng1_U8  rng1(uint8_t min, uint8_t max)   { return rng1_u8(min, max); }
    inline Rng1_U16 rng1(uint16_t min, uint16_t max) { return rng1_u16(min, max); }
    inline Rng1_U32 rng1(uint32_t min, uint32_t max) { return rng1_u32(min, max); }
    inline Rng1_U64 rng1(uint64_t min, uint64_t max) { return rng1_u64(min, max); }
    inline Rng1_I32 rng1(int32_t min, int32_t max)   { return rng1_i32(min, max); }
    inline Rng1_I64 rng1(int64_t min, int64_t max)   { return rng1_i64(min, max); }
    inline Rng1_F32 rng1(float min, float max)       { return rng1_f32(min, max); }

    inline uint8_t  dim_rng1(Rng1_U8 r)  { return dim_rng1_u8(r); }
    inline uint16_t dim_rng1(Rng1_U16 r) { return dim_rng1_u16(r); }
    inline uint32_t dim_rng1(Rng1_U32 r) { return dim_rng1_u32(r); }
    inline uint64_t dim_rng1(Rng1_U64 r) { return dim_rng1_u64(r); }
    inline int32_t  dim_rng1(Rng1_I32 r) { return dim_rng1_i32(r); }
    inline int64_t  dim_rng1(Rng1_I64 r) { return dim_rng1_i64(r); }
    inline float    dim_rng1(Rng1_F32 r) { return dim_rng1_f32(r); }
#else
    #define rng1(min, max) _Generic((min), \
        uint8_t:  rng1_u8,  \
        uint16_t: rng1_u16, \
        uint32_t: rng1_u32, \
        uint64_t: rng1_u64, \
        int32_t:  rng1_i32, \
        int64_t:  rng1_i64, \
        float:    rng1_f32  \
    )(min, max)
    
    #define dim_rng1(r) _Generic((r), \
        Rng1_U8:  dim_rng1_u8,  \
        Rng1_U16: dim_rng1_u16, \
        Rng1_U32: dim_rng1_u32, \
        Rng1_U64: dim_rng1_u64, \
        Rng1_I32: dim_rng1_i32, \
        Rng1_I64: dim_rng1_i64, \
        Rng1_F32: dim_rng1_f32  \
    )(r)
#endif

// ak: 2D Range (Rectangles) ==================================================

// ak: 2D Rng I16
internal Rng2_I16 rng2_i16(Vec2_I16 min, Vec2_I16 max);
internal Vec2_I16 dim_rng2_i16(Rng2_I16 r);
internal Vec2_I16 center_rng2_i16(Rng2_I16 rng);

// ak: 2D Rng I32
internal Rng2_I32 rng2_i32(Vec2_I32 min, Vec2_I32 max);
internal Vec2_I32 dim_rng2_i32(Rng2_I32 r);
internal Vec2_I32 center_rng2_i32(Rng2_I32 rng);

// ak: 2D Rng I64
internal Rng2_I64 rng2_i64(Vec2_I64 min, Vec2_I64 max);
internal Vec2_I64 dim_rng2_i64(Rng2_I64 r);
internal Vec2_I64 center_rng2_i64(Rng2_I64 rng);

// ak: 2D Rng F32
internal Rng2_F32 rng2_f32(Vec2_F32 min, Vec2_F32 max);
internal Vec2_F32 dim_rng2_f32(Rng2_F32 r);
internal Vec2_F32 center_rng2_f32(Rng2_F32 rng);

// ak: 2D Rng Macros
#if LANGUAGE_CPP
    inline Rng2_I16 rng2p(int16_t x, int16_t y, int16_t z, int16_t w) { return
  rng2_i16(Vec2_I16{x, y}, Vec2_I16{z, w}); }
    inline Rng2_I32 rng2p(int32_t x, int32_t y, int32_t z, int32_t w) { return
  rng2_i32(Vec2_I32{x, y}, Vec2_I32{z, w}); }
    inline Rng2_I64 rng2p(int64_t x, int64_t y, int64_t z, int64_t w) { return
  rng2_i64(Vec2_I64{x, y}, Vec2_I64{z, w}); }
    inline Rng2_F32 rng2p(float x, float y, float z, float w)         { return
  rng2_f32(Vec2_F32{x, y}, Vec2_F32{z, w}); }
    
    inline Rng2_I16 rng2(Vec2_I16 min, Vec2_I16 max) { return rng2_i16(min, max); }
    inline Rng2_I32 rng2(Vec2_I32 min, Vec2_I32 max) { return rng2_i32(min, max); }
    inline Rng2_I64 rng2(Vec2_I64 min, Vec2_I64 max) { return rng2_i64(min, max); }
    inline Rng2_F32 rng2(Vec2_F32 min, Vec2_F32 max) { return rng2_f32(min, max); }
    
    inline Vec2_I16 dim_rng2(Rng2_I16 r) { return dim_rng2_i16(r); }
    inline Vec2_I32 dim_rng2(Rng2_I32 r) { return dim_rng2_i32(r); }
    inline Vec2_I64 dim_rng2(Rng2_I64 r) { return dim_rng2_i64(r); }
    inline Vec2_F32 dim_rng2(Rng2_F32 r) { return dim_rng2_f32(r); }
    
    inline Vec2_I16 center_rng2(Rng2_I16 r) { return center_rng2_i16(r); }
    inline Vec2_I32 center_rng2(Rng2_I32 r) { return center_rng2_i32(r); }
    inline Vec2_I64 center_rng2(Rng2_I64 r) { return center_rng2_i64(r); }
    inline Vec2_F32 center_rng2(Rng2_F32 r) { return center_rng2_f32(r); }
#else
    #define rng2p(x, y, z, w) _Generic((x), \
        int16_t: rng2_i16((Vec2_I16){(x), (y)}, (Vec2_I16){(z), (w)}), \
        int32_t: rng2_i32((Vec2_I32){(x), (y)}, (Vec2_I32){(z), (w)}), \
        int64_t: rng2_i64((Vec2_I64){(x), (y)}, (Vec2_I64){(z), (w)}), \
        float:   rng2_f32((Vec2_F32){(x), (y)}, (Vec2_F32){(z), (w)})  \
    )
    
    #define rng2(min, max) _Generic((min), \
        Vec2_I16: rng2_i16, \
        Vec2_I32: rng2_i32, \
        Vec2_I64: rng2_i64, \
        Vec2_F32: rng2_f32  \
    )(min, max)
    
    #define dim_rng2(r) _Generic((r), \
        Rng2_I16: dim_rng2_i16, \
        Rng2_I32: dim_rng2_i32, \
        Rng2_I64: dim_rng2_i64, \
        Rng2_F32: dim_rng2_f32  \
    )(r)
    
    #define center_rng2(r) _Generic((r), \
        Rng2_I16: center_rng2_i16, \
        Rng2_I32: center_rng2_i32, \
        Rng2_I64: center_rng2_i64, \
        Rng2_F32: center_rng2_f32  \
    )(r)
#endif

// ak: Random-number generation
//=============================================================================

internal uint32_t rand_u32(uint32_t seed);

// ak: Color Operations
//=============================================================================

// ak: srgb <-> linear
internal Vec3_F32 linear_from_srgb(Vec3_F32 srgb);
internal Vec3_F32 srgb_from_linear(Vec3_F32 linear);
internal Vec4_F32 linear_from_srgba(Vec4_F32 srgba);
internal Vec4_F32 srgba_from_linear(Vec4_F32 linear);

// ak: rgba <-> u32
internal uint32_t u32_from_rgba(Vec4_F32 rgba);
#define rgba_from_u32(h) (Vec4_F32){ (((h)&0xff000000)>>24)/255.f, (((h)&0x00ff0000)>>16)/255.f, (((h)&0x0000ff00)>> 8)/255.f, (((h)&0x000000ff)>> 0)/255.f }

#endif // BASE_H
