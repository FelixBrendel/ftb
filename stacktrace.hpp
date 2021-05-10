#pragma once

#include "platform.hpp"

#if defined FTB_WINDOWS
#  include <dbghelp.h>
#else
#  include <execinfo.h>
#  include <unistd.h>
#  include "stdio.h"
#  include "stdlib.h"
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
    free(strings);

#endif
}
