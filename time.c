#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

int main()
{
    struct  timeval start;
    struct timeval end;

    gettimeofday(&start, NULL);
    usleep(500000);
    gettimeofday(&end, NULL);
    long    ms_start = start.tv_sec * 1000 + start.tv_usec / 1000;
    long    ms_end = end.tv_sec * 1000 + end.tv_usec / 1000;
    long    diff =  ms_end - ms_start;
    printf("Temps écoulé : %li ms", diff);
}