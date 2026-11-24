#include "types.h"
//#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "system_timer.h"

///////////////////////////////////////////////////////////////////////////

static struct timespec ms_start;
static bool_t is_init = FALSE;

///////////////////////////////////////////////////////////////////////////

void system_timer_init(void)
{
    if (is_init == TRUE)
    {
        return;
    }
	is_init = TRUE;

    //gettimeofday(&ms_start, NULL);    
    clock_gettime(CLOCK_MONOTONIC, &ms_start);
}

uint64_t system_timer_get_us(void)
{
    if (is_init == FALSE)
    {
        return 0;
    }

    struct timespec ms_value;
    long int mtime, seconds, n_seconds;
    
    clock_gettime(CLOCK_MONOTONIC, &ms_value);
    
    seconds = (long int)ms_value.tv_sec - (long int)ms_start.tv_sec;
    n_seconds = (long int)ms_value.tv_nsec  - (long int)ms_start.tv_nsec ;
    mtime = (seconds * 1000000L) + n_seconds / 1000L;
    
    return (uint64_t)mtime;    
}

uint32_t system_timer_get_ms(void)
{
    uint32_t value_ms = (uint32_t)(system_timer_get_us() / 1000L);
    return value_ms;
}
