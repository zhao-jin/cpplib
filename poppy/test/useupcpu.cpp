// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Dongping HUANG <dphuang@tencent.com>
// Created: 06/28/11
// Description:

#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>

#include "common/system/concurrency/this_thread.h"
#include "common/system/time/timestamp.h"

bool g_quit = false;

static void SignalIntHandler(int sig)
{
    g_quit = true;
}

void bind_processor()
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);

    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
        printf("warning: could not set CPU affinity, continuing...\n");
    }
}

int get_processor_usage()
{
    static struct rusage start;
    static bool inited = false;
    static int64_t starttime;

    if (!inited) {
        getrusage(RUSAGE_SELF, &start);
        starttime = GetTimeStampInMs();
        inited = true;
    }

    struct rusage end;

    getrusage(RUSAGE_SELF, &end);

    int64_t endtime = GetTimeStampInMs();

    double usertime, kerneltime;

    usertime = (end.ru_utime.tv_sec - start.ru_utime.tv_sec) * 1000
               + (end.ru_utime.tv_usec - start.ru_utime.tv_usec) / 1000;

    kerneltime = (end.ru_stime.tv_sec - start.ru_stime.tv_sec) * 1000
                 + (end.ru_stime.tv_usec - start.ru_stime.tv_usec) / 1000;

    int percentage = (usertime + kerneltime) * 100 / (endtime - starttime);

    return percentage;
}

uint64_t do_useless_staff()
{
    uint64_t fib[64];
    fib[0] = 1;
    fib[1] = 1;

    for (int k = 2; k < 64; ++k) {
        fib[k] = fib[k - 2] + fib[k - 1];
    }

    return fib[63];
}

int main(int argc, char** argv)
{
    signal(SIGINT, SignalIntHandler);
    signal(SIGTERM, SignalIntHandler);
    bind_processor();

    while (!g_quit) {
        int percentage = get_processor_usage();

        if (percentage < 99) {
            do_useless_staff();
        } else {
            ThisThread::Sleep(1);
        }
    }

    return 0;
}

