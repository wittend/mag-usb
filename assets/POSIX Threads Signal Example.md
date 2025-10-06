POSIX Threads Signal Example

Here is a program that uses POSIX threads to print sensor data every 1000 ms and handles standard signals like SIGHUP and SIGABRT using a dedicated signal handling thread:

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

// Shared state between threads
volatile sig_atomic_t shutdown_requested = 0; // Signal-safe flag
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER; // Protect shared data
int sensor_data = 0; // Simulated sensor data

// Function to simulate reading sensor data
void* read_sensor(void* arg) 
{
    while (!shutdown_requested) 
    {
        // Simulate sensor reading (replace with actual sensor code)
        sensor_data = rand() % 100; // Random data between 0 and 99

        // Sleep for 1000 ms (1 second)
        usleep(1000000);

        // Check for shutdown request
        if (shutdown_requested) 
        {
            break;
        }
    }
    return NULL;
}

// Function to print sensor data every 1000 ms
void* print_data(void* arg) 
{
    while (!shutdown_requested) 
    {
        // Wait for 1000 ms or until shutdown is requested
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1; // Add 1 second

        // Use condition variable with timeout to wait
        pthread_mutex_lock(&data_mutex);
        int local_data = sensor_data;
        pthread_mutex_unlock(&data_mutex);

        // Print data to stdout
        printf("Sensor Data: %d\n", local_data);

        // Sleep for 1000 ms or until interrupted by signal
        struct timespec sleep_time = {1, 0}; // 1 second
        nanosleep(&sleep_time, NULL);
    }

    return NULL;
}

// Signal handler thread function
void* signal_handler_thread(void* arg) 
{
    sigset_t sigset;
    int signum;

    // Block SIGHUP, SIGABRT, and SIGINT in this thread
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGHUP);
    sigaddset(&sigset, SIGABRT);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    // Wait for any of the blocked signals
    while (1) 
    {
        if (sigwait(&sigset, &signum) == 0) 
        {
            switch (signum) 
            {
                case SIGHUP:
                    printf("Received SIGHUP. Shutting down gracefully.\n");
                    break;
                case SIGABRT:
                    printf("Received SIGABRT. Shutting down gracefully.\n");
                    break;
                case SIGINT:
                    printf("Received SIGINT. Shutting down gracefully.\n");
                    break;
                default:
                    printf("Received unexpected signal %d.\n", signum);
                    break;
            }
            // Set shutdown flag
            shutdown_requested = 1;
            break;
        }
    }
    return NULL;
}

int main() 
{
    pthread_t sensor_thread, print_thread, signal_thread;

    // Create threads
    if (pthread_create(&sensor_thread, NULL, read_sensor, NULL) != 0) 
    {
        perror("pthread_create sensor");
        exit(1);
    }
    if (pthread_create(&print_thread, NULL, print_data, NULL) != 0) 
    {
        perror("pthread_create print");
        exit(1);
    }
    if (pthread_create(&signal_thread, NULL, signal_handler_thread, NULL) != 0) 
    {
        perror("pthread_create signal");
        exit(1);
    }
    // Wait for signal handler thread to complete (it will only return on signal)
    pthread_join(signal_thread, NULL);

    // Signal handler has set shutdown_requested, so wait for other threads to finish
    pthread_join(sensor_thread, NULL);
    pthread_join(print_thread, NULL);

    // Clean up
    pthread_mutex_destroy(&data_mutex);

    printf("Program terminated.\n");
    return 0;
}
```

* This program uses a dedicated signal handling thread to safely receive signals using `sigwait`, which is the recommended approach for multithreaded applications to avoid the pitfalls of signal handlers  
* The main thread creates two worker threads: one to read simulated sensor data and another to print it every 1000 ms. 
* The signal handler thread blocks SIGHUP, SIGABRT, and SIGINT, waits for any of them using `sigwait`, and sets a global `shutdown_requested` flag when a signal is received  
* The worker threads check this flag periodically and terminate gracefully. 
* The main thread waits for the signal handler to complete before joining the other threads and exiting  
* This design ensures safe signal handling in a multithreaded environment 