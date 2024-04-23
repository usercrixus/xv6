/*
represent the real-time clock date and time. It's typically used in system calls
that need to fetch or set the current date and time from the system's real-time
clock. Each field in the structure represents a component of the date and time
*/
struct rtcdate {
    uint second;
    uint minute;
    uint hour;
    uint day;
    uint month;
    uint year;
};
