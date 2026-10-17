// ak: Array Make
//=============================================================================

internal inline U8Array u8array(uint8_t  *p, size_t n)
{
    return (U8Array) {p, n, n};
}
internal inline U16Array u16array(uint16_t *p, size_t n)
{
    return (U16Array){p, n, n};
}
internal inline U32Array u32array(uint32_t *p, size_t n)
{
    return (U32Array){p, n, n};
}
internal inline U64Array u64array(uint64_t *p, size_t n)
{
    return (U64Array){p, n, n};
}
internal inline I8Array  i8array(int8_t   *p, size_t n)
{
    return (I8Array) {p, n, n};
}
internal inline I16Array i16array(int16_t  *p, size_t n)
{
    return (I16Array){p, n, n};
}
internal inline I32Array i32array(int32_t  *p, size_t n)
{
    return (I32Array){p, n, n};
}
internal inline I64Array i64array(int64_t  *p, size_t n)
{
    return (I64Array){p, n, n};
}
internal inline F32Array f32array(float    *p, size_t n)
{
    return (F32Array){p, n, n};
}
internal inline F64Array f64array(double   *p, size_t n)
{
    return (F64Array){p, n, n};
}


// ak: Safe Casts
//=============================================================================

internal uint16_t safe_cast_u16(uint32_t x)
{
    AssertAlways(x <= max_u16);
    uint16_t result = (uint16_t)x;
    return result;
}

internal uint32_t safe_cast_u32(uint64_t x)
{
    AssertAlways(x <= max_u32);
    uint32_t result = (uint32_t)x;
    return result;
}

internal int32_t safe_cast_s32(int64_t x)
{
    AssertAlways(x <= max_i32);
    int32_t result = (int32_t)x;
    return result;
}

internal uint32_t u32_from_u64_saturate(uint64_t x)
{
  uint32_t x32 = (x > max_u32)?max_u32:(uint32_t)x;
  return x32;
}
