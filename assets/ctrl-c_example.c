//----------------------------------------------------------------
// ctrl-c_example.c
//
//   An example program to catch SIGINT (Ctrl+C) and exit.
//----------------------------------------------------------------
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

static volatile sig_atomic_t keep_running = 1;

void sig_handler(int sig) 
{
    (void)sig;
    keep_running = 0;
}

int main(void) 
{
    struct sigaction act;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if(sigaction(SIGINT, &act, NULL) == -1) 
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    while (keep_running) 
    {
        puts("Still running... (Press Ctrl+C to exit)");
        sleep(1);
    }

    puts("Stopped by signal `SIGINT`");
    return EXIT_SUCCESS;
}

