internal inline void *mem_copy(void *dest, void const *source, size_t n)
{
    if (dest == NULL)
    {
        return NULL;
    }
#if COMPILER_MSVC
    __movsb((uint8_t *)dest, (uint8_t *)source, n);
#elif defined(OS_MAC) || defined(OS_LINUX)
    #if defined(__x86_64__) || defined(__i386__)
        uint8_t *d = (uint8_t *)dest;
        uint8_t const *s = (uint8_t const *)source;
        size_t c = n;

        __asm__ __volatile__(
            "rep movsb"
            : "+D" (d), "+S" (s), "+c" (c)
            :
            : "memory"
        );
    #endif
#elif ARCH_X86
    void *dest_copy = dest;
    __asm__ __volatile__("rep movsb" : "+D"(dest_copy), "+S"(source), "+c"(n) : : "memory");
#endif
    return dest;
}

internal inline void *mem_move(void *dest, void const *source, size_t n)
{
    uint8_t *d = (uint8_t *)dest;
    uint8_t const *s = (uint8_t const *)source;
    if (dest == NULL)
    {
        return NULL;
    }
    if (d == s)
    {
        return d;
    }
    if (s+n <= d || d+n <= s)
    {
        //- ak: Non-overlapping
        return mem_copy(d, s, n);
    }
    if (d < s)
    {
        if (IntFromPtr(s) % sizeof(int64_t) == IntFromPtr(d) % sizeof(int64_t))
        {
            while (IntFromPtr(d) % sizeof(int64_t))
            {
                if (!n--) return dest;
                *d++ = *s++;
            }
            while (n >= (int64_t)sizeof(int64_t))
            {
                *(int64_t *)d = *(int64_t *)s;
                n -= sizeof(int64_t);
                d += sizeof(int64_t);
                s += sizeof(int64_t);
            }
        }
        for (; n; n--) *d++ = *s++;
    }
    else
    {
        if ((IntFromPtr(s) % sizeof(int64_t)) == (IntFromPtr(d) % sizeof(int64_t)))
        {
            while (IntFromPtr(d+n) % sizeof(int64_t))
            {
                if (!n--)
                    return dest;
                d[n] = s[n];
            }
            while (n >= (int64_t)sizeof(int64_t))
            {
                n -= sizeof(int64_t);
                *(int64_t *)(d+n) = *(int64_t *)(s+n);
            }
        }
        while (n) n--, d[n] = s[n];
    }
    return dest;
}

internal inline void *mem_set(void *dest, uint8_t c, size_t n)
{
    uint8_t *s = (uint8_t *)dest;
    int64_t k;
    uint32_t c32 = ((uint32_t)-1)/255 * c;
    if (dest == NULL)
    {
        return NULL;
    }
    if (n == 0)
    { return dest; }
    s[0] = s[n-1] = c;
    if (n < 3)
    { return dest; }
    s[1] = s[n-2] = c;
    s[2] = s[n-3] = c;
    if (n < 7)
    { return dest; }
    s[3] = s[n-4] = c;
    if (n < 9)
    { return dest; }
    k = -(int64_t)s & 3;
    s += k;
    n -= k;
    n &= -4;
    *(uint32_t *)(s+0) = c32;
    *(uint32_t *)(s+n-4) = c32;
    if (n < 9)
    { return dest; }
    *(uint32_t *)(s +  4)    = c32;
    *(uint32_t *)(s +  8)    = c32;
    *(uint32_t *)(s+n-12) = c32;
    *(uint32_t *)(s+n- 8) = c32;
    if (n < 25)
    { return dest; }
    *(uint32_t *)(s + 12) = c32;
    *(uint32_t *)(s + 16) = c32;
    *(uint32_t *)(s + 20) = c32;
    *(uint32_t *)(s + 24) = c32;
    *(uint32_t *)(s+n-28) = c32;
    *(uint32_t *)(s+n-24) = c32;
    *(uint32_t *)(s+n-20) = c32;
    *(uint32_t *)(s+n-16) = c32;
    k = 24 + (IntFromPtr(s) & 4);
    s += k;
    n -= k;
    {
        uint64_t c64 = ((uint64_t)c32 << 32) | c32;
        while (n > 31)
        {
            *(uint64_t *)(s+0) = c64;
            *(uint64_t *)(s+8) = c64;
            *(uint64_t *)(s+16) = c64;
            *(uint64_t *)(s+24) = c64;
            n -= 32;
            s += 32;
        }
    }
    return dest;
}

internal inline int32_t mem_cmp(void const *s1, void const *s2, size_t size)
{
    int32_t result = 0;
	uint8_t const *s1p8 = Cast(uint8_t const *)s1;
	uint8_t const *s2p8 = Cast(uint8_t const *)s2;

	while (size-- && s1 && s2) {
		if (*s1p8 != *s2p8) {
			result = (*s1p8 - *s2p8);
			break;
		}
		s1p8++, s2p8++;
	}
	return result;
}

internal int32_t mem_is_zero(void *ptr, size_t size)
{
    int32_t result = 1;
    //- ak: break down size
    uint64_t extra = (size&0x7);
    uint64_t count8 = (size >> 3);
    //- ak: check with 8-byte stride
    uint64_t *p64 = (uint64_t*)ptr;
    if (result)
    {
        for (uint64_t i = 0; i < count8; i += 1, p64 += 1)
        {
            if (*p64 != 0)
            {
                result = 0;
                goto done;
            }
        }
    }
    //- ak: check extra
    if (result)
    {
        uint8_t *p8 = (uint8_t*)p64;
        for (uint64_t i = 0; i < extra; i += 1, p8 += 1)
        {
            if (*p8 != 0)
            {
                result = 0;
                goto done;
            }
        }
    }
done:;
     return(result);
}

