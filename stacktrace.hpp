#pragma once

#include "platform.hpp"
#include "allocation_stats.hpp"

#if defined FTB_WINDOWS
#  include <dbghelp.h>
#else
#  ifdef FTB_LINUX_STACK_TRACE_USE_GDB
#    include <stdio.h>
#    include <stdlib.h>
#    include <sys/wait.h>
#    include <unistd.h>
#    include <sys/prctl.h>
#  else
#    include <execinfo.h>
#    include <unistd.h>
#    include "stdio.h"
#    include "stdlib.h"
#  endif
#endif

auto print_stacktrace() -> void {
#if defined FTB_WINDOWS
    printf("Stacktrace: \n");
    unsigned int   i;
    void         * stack[ 100 ];
    HANDLE         process;
    SYMBOL_INFO  * symbol;
    symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
    symbol->MaxNameLen   = 255;
    symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

    unsigned short frames;
    frames = CaptureStackBackTrace( 1, 100, stack, NULL );

    process = GetCurrentProcess();
    SymInitialize( process, NULL, TRUE );

    for( i = 0; i < frames; i++ ) {
        SymFromAddr( process, ( DWORD64 )( stack[ i ] ), 0, symbol );
        printf( "  %3i: %s\n", frames - i - 1, symbol->Name);
    }
    fflush(stdout);
#else
#ifdef FTB_LINUX_STACK_TRACE_USE_GDB
    printf("Stacktrace: \n");
    char pid_buf[30];
    sprintf(pid_buf, "%d", getpid());
    char name_buf[512];
    name_buf[readlink("/proc/self/exe", name_buf, 511)]=0;
    prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);
    int child_pid = fork();
    if (!child_pid) {
        dup2(2,1); // redirect output to stderr - edit: unnecessary?
        execl("/usr/bin/gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt", name_buf, pid_buf, NULL);
        abort(); /* If gdb failed to start */
    } else {
        waitpid(child_pid,NULL,0);
   }
#else
    // NOTE(Felix): Don't forget to compile with "-rdynamic"
    printf("Stacktrace (this is unmagled -- sorry): \n");
    char **strings;
    size_t i, size;
    enum Constexpr { MAX_SIZE = 1024 };
    void *array[MAX_SIZE];
    size = backtrace(array, MAX_SIZE);
    strings = backtrace_symbols(array, size);
    for (i = 0; i < size; i++)
        printf("  %3lu: %s\n", size - i - 1, strings[i]);
    puts("");
    ftb_free(strings);
#endif
#endif
}
