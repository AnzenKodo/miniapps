// Time Functions
//=============================================================================

internal DenseTime
dense_time_from_date_time(DateTime date_time)
{
    DenseTime result = 0;
    result += date_time.year;
    result *= 12;
    result += date_time.mon;
    result *= 31;
    result += date_time.day;
    result *= 24;
    result += date_time.hour;
    result *= 60;
    result += date_time.min;
    result *= 61;
    result += date_time.sec;
    result *= 1000;
    result += date_time.msec;
    return(result);
}

internal DateTime
date_time_from_dense_time(DenseTime time)
{
    DateTime result = STRUCT_ZERO;
    result.msec = time%1000;
    time /= 1000;
    result.sec  = time%61;
    time /= 61;
    result.min  = time%60;
    time /= 60;
    result.hour = time%24;
    time /= 24;
    result.day  = time%31;
    time /= 31;
    result.mon  = time%12;
    time /= 12;
    Assert(time <= max_u32);
    result.year = (uint32_t)time;
    return(result);
}

internal DateTime
date_time_from_micro_seconds(uint64_t time)
{
    DateTime result = STRUCT_ZERO;
    result.micro_sec = time%1000;
    time /= 1000;
    result.msec = time%1000;
    time /= 1000;
    result.sec = time%60;
    time /= 60;
    result.min = time%60;
    time /= 60;
    result.hour = time%24;
    time /= 24;
    result.day = time%31;
    time /= 31;
    result.mon = time%12;
    time /= 12;
    Assert(time <= max_u32);
    result.year = (uint32_t)time;
    return(result);
}

internal DateTime
date_time_from_unix_time(uint64_t unix_time)
{
    DateTime date = STRUCT_ZERO;
    date.year     = 1970;
    date.day      = (uint16_t)(1 + (unix_time / 86400));
    date.sec      = (uint32_t)unix_time % 60;
    date.min      = (uint32_t)(unix_time / 60) % 60;
    date.hour     = (uint32_t)(unix_time / 3600) % 24;

    for (;;)
    {
    for (date.month = Month_Jan; date.month < Month_COUNT; date.month = (Month)(date.month + 1))
        {
            uint64_t c = 0;
            switch(date.month)
            {
                case Month_Jan: c = 31; break;
                case Month_Feb:
                {
                    if (
                        (date.year % 4 == 0) &&
                        ((date.year % 100) != 0 || (date.year % 400) == 0)
                    )
                    {
                        c = 29;
                    }
                    else
                    {
                        c = 28;
                    }
                } break;
                case Month_Mar: c = 31; break;
                case Month_Apr: c = 30; break;
                case Month_May: c = 31; break;
                case Month_Jun: c = 30; break;
                case Month_Jul: c = 31; break;
                case Month_Aug: c = 31; break;
                case Month_Sep: c = 30; break;
                case Month_Oct: c = 31; break;
                case Month_Nov: c = 30; break;
                case Month_Dec: c = 31; break;
                case Month_COUNT: UNREACHABLE();
            }
            if (date.day <= c)
            {
                goto exit;
            }
            date.day -= (uint16_t)c;
        }
        ++date.year;
    }
    exit:;

    return date;
}

