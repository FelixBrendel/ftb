#pragma once
#include <stdlib.h>
#include <stdio.h>

#include "types.hpp"
#include "macros.hpp"

#ifdef FTB_TRACK_MALLOCS
namespace Ftb_Malloc_Stats {
    extern u32 malloc_calls  = 0;
    extern u32 free_calls    = 0;
    extern u32 realloc_calls = 0;
    extern u32 calloc_calls  = 0;
}

#define malloc(size)       (++Ftb_Malloc_Stats::malloc_calls,  malloc(size))
#define free(ptr)          (++Ftb_Malloc_Stats::free_calls,    free(ptr))
#define realloc(ptr, size) (++Ftb_Malloc_Stats::realloc_calls, realloc(ptr, size))
#define calloc(num, size)  (++Ftb_Malloc_Stats::calloc_calls,  calloc(num, size))

#define profile_mallocs                                                 \
    MPP_DECLARE(0, u32 MPI_LABEL(profile_mallocs, old_malloc_calls)  = Ftb_Malloc_Stats::malloc_calls) \
    MPP_DECLARE(1, u32 MPI_LABEL(profile_mallocs, old_free_calls)    = Ftb_Malloc_Stats::free_calls) \
    MPP_DECLARE(2, u32 MPI_LABEL(profile_mallocs, old_realloc_calls) = Ftb_Malloc_Stats::realloc_calls) \
    MPP_DECLARE(3, u32 MPI_LABEL(profile_mallocs, old_calloc_calls)  = Ftb_Malloc_Stats::calloc_calls) \
    MPP_AFTER(4, {                                                      \
            printf("\n"                                                 \
                   "Local Malloc Stats: (%s %s %d)\n"                   \
                   "-------------------\n"                              \
                   "   malloc calls: %u\n"                              \
                   "     free calls: %u\n"                              \
                   "  realloc calls: %u\n"                              \
                   "   calloc calls: %u\n",                             \
                   __func__, __FILE__, __LINE__,                        \
                   Ftb_Malloc_Stats::malloc_calls  - MPI_LABEL(profile_mallocs, old_malloc_calls)  , \
                   Ftb_Malloc_Stats::free_calls    - MPI_LABEL(profile_mallocs, old_free_calls)    , \
                   Ftb_Malloc_Stats::realloc_calls - MPI_LABEL(profile_mallocs, old_realloc_calls) , \
                   Ftb_Malloc_Stats::calloc_calls  - MPI_LABEL(profile_mallocs, old_calloc_calls));  \
        })
#endif // FTB_TRACK_MALLOCS

#ifndef FTB_ALLOCATION_STATS_IMPL

auto print_malloc_stats() -> void;

#else

auto print_malloc_stats() -> void {
    printf("\n"
           "Global Malloc Stats:\n"
           "--------------------\n"
           "   malloc calls: %u\n"
           "     free calls: %u\n"
           "  realloc calls: %u\n"
           "   calloc calls: %u\n",
           Ftb_Malloc_Stats::malloc_calls,
           Ftb_Malloc_Stats::free_calls,
           Ftb_Malloc_Stats::realloc_calls,
           Ftb_Malloc_Stats::calloc_calls);
}

#endif //FTB_ALLOCATION_STATS_IMPL
