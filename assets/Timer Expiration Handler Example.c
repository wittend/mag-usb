#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

// Timer expiration handler function
void timer_handler(union sigval sv)
{
    int id = *(int*)sv.sival_ptr;
    printf("Timer %d expired\n", id);
}

int main()
{
    timer_t timer_id;
    struct sigevent sev;
    struct itimerspec timer_spec;
    int timer_id_data = 1;

    // Configure timer to call timer_handler in a new thread
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timer_handler;
    sev.sigev_value.sival_ptr = &timer_id_data;
    sev.sigev_notify_attributes = NULL;

    // Create timer
    if (timer_create(CLOCK_REALTIME, &sev, &timer_id) == -1)
    {
        perror("timer_create");
        exit(EXIT_FAILURE);
    }

    // Set timer to fire every 2 seconds
    timer_spec.it_value.tv_sec     = 2;
    timer_spec.it_value.tv_nsec    = 0;
    timer_spec.it_interval.tv_sec  = 2;  // Repeating
    timer_spec.it_interval.tv_nsec = 0;

    if (timer_settime(timer_id, 0, &timer_spec, NULL) == -1)
    {
        perror("timer_settime");
        exit(EXIT_FAILURE);
    }

    printf("Timer set. Waiting 10 seconds...\n");
    sleep(10000);  // Let timer fire multiple times

    // Clean up
    timer_delete(timer_id);
    return 0;
}
