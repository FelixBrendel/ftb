#pragma once
#include <stdlib.h>
#include <stdio.h>

#include "types.hpp"
#include "macros.hpp"

#ifdef USE_FTB_MALLOC
namespace Ftb_Malloc_Stats {
    u32 malloc_calls  = 0;
    u32 free_calls    = 0;
    u32 realloc_calls = 0;
    u32 calloc_calls  = 0;
    u32 alloca_calls  = 0;
}

#define ftb_malloc(size)       (++Ftb_Malloc_Stats::malloc_calls,  malloc(size))
#define ftb_free(ptr)          (++Ftb_Malloc_Stats::free_calls,    free(ptr))
#define ftb_realloc(ptr, size) (++Ftb_Malloc_Stats::realloc_calls, realloc(ptr, size))
#define ftb_calloc(num, size)  (++Ftb_Malloc_Stats::calloc_calls,  calloc(num, size))
#define ftb_alloca(size)       (++Ftb_Malloc_Stats::alloca_calls,  alloca(size))

void print_malloc_stats() {
    printf("\n"
           "Global Malloc Stats:\n"
           "--------------------\n"
           "  ftb_malloc  calls: %u\n"
           "  ftb_free    calls: %u\n"
           "  ftb_realloc calls: %u\n"
           "  ftb_calloc  calls: %u\n"
           "  ftb_alloca  calls: %u\n" ,
           Ftb_Malloc_Stats::malloc_calls,
           Ftb_Malloc_Stats::free_calls,
           Ftb_Malloc_Stats::realloc_calls,
           Ftb_Malloc_Stats::calloc_calls,
           Ftb_Malloc_Stats::alloca_calls);
}

#define profile_mallocs                                                 \
    MPP_DECLARE(0, u32 MPI_LABEL(profile_mallocs, old_malloc_calls)  = Ftb_Malloc_Stats::malloc_calls) \
    MPP_DECLARE(1, u32 MPI_LABEL(profile_mallocs, old_free_calls)    = Ftb_Malloc_Stats::free_calls) \
    MPP_DECLARE(2, u32 MPI_LABEL(profile_mallocs, old_realloc_calls) = Ftb_Malloc_Stats::realloc_calls) \
    MPP_DECLARE(3, u32 MPI_LABEL(profile_mallocs, old_calloc_calls)  = Ftb_Malloc_Stats::calloc_calls) \
    MPP_DECLARE(4, u32 MPI_LABEL(profile_mallocs, old_alloca_calls)  = Ftb_Malloc_Stats::alloca_calls) \
    MPP_AFTER(5, {                                                      \
            printf("\n"                                                 \
                   "Local Malloc Stats: (%s %s %d)\n"                   \
                   "-------------------\n"                              \
                   "  ftb_malloc  calls: %u\n"                          \
                   "  ftb_free    calls: %u\n"                          \
                   "  ftb_realloc calls: %u\n"                          \
                   "  ftb_calloc  calls: %u\n"                          \
                   "  ftb_alloca  calls: %u\n" ,                        \
                   __func__, __FILE__, __LINE__,                        \
                   Ftb_Malloc_Stats::malloc_calls  - MPI_LABEL(profile_mallocs, old_malloc_calls)  , \
                   Ftb_Malloc_Stats::free_calls    - MPI_LABEL(profile_mallocs, old_free_calls)    , \
                   Ftb_Malloc_Stats::realloc_calls - MPI_LABEL(profile_mallocs, old_realloc_calls) , \
                   Ftb_Malloc_Stats::calloc_calls  - MPI_LABEL(profile_mallocs, old_calloc_calls)  , \
                   Ftb_Malloc_Stats::alloca_calls  - MPI_LABEL(profile_mallocs, old_alloca_calls)); \
        })

#else
# define ftb_malloc  malloc
# define ftb_realloc realloc
# define ftb_calloc  calloc
# define ftb_free    free
# define ftb_alloca  alloca
#endif
