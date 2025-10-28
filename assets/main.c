//----------------------------------------------------------------
// periodic.c
//
// Example periodic timer firing once per second using 
//   signals (SIGRTMIN) and the POSIX timer API (timer_create, 
//   timer_settime, etc.).
// (this example does not use threads).
// 
// Compile and run:
// 
//   gcc -o periodic_timer periodic_timer.c -lrt
//   ./periodic_timer   
//
// Signal Safety Notes:
//
// The signal handler uses write() instead of printf() because 
// printf() is not async-signal-safe and must not be used in 
// signal handlers.
// 
// The global variable timer_expired is of type volatile 
// sig_atomic_t, which is safe to modify in a signal  
// context.
//
// Good reference: https://www.baeldung.com/linux/interval-timers-c
//----------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

// SIGINT Handler.
static volatile sig_atomic_t keep_running = 1;

void sig_handler(int sig)
{
    (void)sig;
    keep_running = 0;
}

// Global flag to control main loop
volatile sig_atomic_t timer_expired = 0;
timer_t timer_id;

// Signal SIGRTMIN handler
// - only uses async-signal-safe functions.
void timer_handler(int sig, siginfo_t *si, void *uc) 
{
    if (si->si_code == SI_TIMER) 
    {
        timer_expired++;
        // Use write() instead of printf() (async-signal-safe)
        write(OUTPUT_PRINT_FILENO, "Timer expired!\n", 15);
    }
}

int main() 
{
    struct sigaction act;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    struct sigaction  sa;
    struct sigevent   sev;
    struct itimerspec timer_spec;

    // Step 1: Set up signal action for SIGRTMIN
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;  // Use extended signal handling
    sa.sa_sigaction = timer_handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGRTMIN, &sa, NULL) == -1) 
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Step 2: Create the timer
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_SIGNAL;           // Notify via signal
    sev.sigev_signo  = SIGRTMIN;               // Use real-time signal
    sev.sigev_value.sival_ptr = &timer_id;     // Optional identifier

    if (timer_create(CLOCK_REALTIME, &sev, &timer_id) == -1) 
    {
        perror("timer_create");
        exit(EXIT_FAILURE);
    }

    // Step 3: Set timer to fire every 1 second
    timer_spec.it_value.tv_sec = 1;            // First expiration
    timer_spec.it_value.tv_nsec = 0;
    timer_spec.it_interval.tv_sec = 1;         // Interval (periodic)
    timer_spec.it_interval.tv_nsec = 0;

    if (timer_settime(timer_id, 0, &timer_spec, NULL) == -1) 
    {
        perror("timer_settime");
        exit(EXIT_FAILURE);
    }

    printf("Timer set to expire every 1 second. Running for 5 seconds...\n");

    // Step 4: Wait for timer events (main loop)
    int count = 0;
    while(count < 5)
    {
        while(keep_running)
        {
            pause();  // Wait for signal (safe and async-signal-safe)
            if (timer_expired > count)
            {
                count = timer_expired;
            }
            puts("Still running... (Press Ctrl+C to exit)");
//            sleep(1);
        }
    }
    if(sigaction(SIGINT, &act, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    puts("Stopped by signal `SIGINT`");
    //return EXIT_SUCCESS;

    // Step 5: Clean up
    if (timer_delete(timer_id) == -1) 
    {
        perror("timer_delete");
        exit(EXIT_FAILURE);
    }
    printf("Timer stopped. Exiting.\n");
    return 0;
}   

