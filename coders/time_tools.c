# include "codexion.h"

long long   get_time_ms(void)
{
	struct timeval	tv;
	
	gettimeofday(&tv, NULL);
	return ((long long)tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void	ms_to_timespec(long long time_in_ms, struct timespec *ts)
{
	ts->tv_sec = time_in_ms / 1000;
	ts->tv_nsec = (time_in_ms % 1000) * 1000000;
}