// Safe Casts
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
