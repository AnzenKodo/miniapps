#ifndef BASE_TIME_H
#define BASE_TIME_H

// Types
//=============================================================================

typedef enum WeekDay
{
    WeekDay_Sun,
    WeekDay_Mon,
    WeekDay_Tue,
    WeekDay_Wed,
    WeekDay_Thu,
    WeekDay_Fri,
    WeekDay_Sat,
    WeekDay_COUNT,
} WeekDay;

typedef enum Month
{
    Month_Jan,
    Month_Feb,
    Month_Mar,
    Month_Apr,
    Month_May,
    Month_Jun,
    Month_Jul,
    Month_Aug,
    Month_Sep,
    Month_Oct,
    Month_Nov,
    Month_Dec,
    Month_COUNT,
} Month;

typedef struct DateTime DateTime;
struct DateTime
{
    uint16_t micro_sec; // [0,999]
    uint16_t msec; // [0,999]
    uint16_t sec;  // [0,60]
    uint16_t min;  // [0,59]
    uint16_t hour; // [0,24]
    uint16_t day;  // [0,30]
    union
    {
        WeekDay week_day;
        uint32_t wday;
    };
    union
    {
        Month month;
        uint32_t mon;
    };
    uint32_t year; // 1 = 1 CE, 0 = 1 BC
};

typedef uint64_t DenseTime;

// Time Functions
//=============================================================================


internal DenseTime dense_time_from_date_time(DateTime date_time);
internal DateTime date_time_from_dense_time(DenseTime time);
internal DateTime date_time_from_micro_seconds(uint64_t time);
internal DateTime date_time_from_unix_time(uint64_t unix_time);

#endif // BASE_TIME_H
