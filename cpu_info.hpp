/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Felix Brendel
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <string.h>

#ifdef _MSC_VER
#  include <intrin.h>
#  include <immintrin.h>
#  define platform_independent_cpuid(function_id, array_of_registers)   \
    __cpuid(array_of_registers, function_id)

#  define platform_independent_cpuidex(function_id, sub_function_id, array_of_registers) \
    __cpuidex(array_of_registers, function_id, sub_function_id)
#else
#  include <cpuid.h>
#  define platform_independent_cpuid(function_id, array_of_registers)   \
    __cpuid(function_id, array_of_registers[0], array_of_registers[1],  \
            array_of_registers[2],array_of_registers[3])

#  define platform_independent_cpuidex(function_id, sub_function_id, array_of_registers) \
    __cpuid_count (function_id, sub_function_id, array_of_registers[0], \
                   array_of_registers[1], array_of_registers[2],        \
                   array_of_registers[3])
#endif


enum struct Edx_1_Feature_Flags {
    fpu    = 1<<0,  // Onboard x87 FPU
    vme    = 1<<1,  // Virtual 8086 mode extensions (such as VIF, VIP, PIV)
    de     = 1<<2,  // Debugging extensions (CR4 bit 3)
    pse    = 1<<3,  // Page Size Extension
    tsc    = 1<<4,  // Time Stamp Counter
    msr    = 1<<5,  // Model-specific registers
    pae    = 1<<6,  // Physical Address Extension
    mce    = 1<<7,  // Machine Check Exception
    cx8    = 1<<8,  // CMPXCHG8 (compare-and-swap) instruction
    apic   = 1<<9,  // Onboard Advanced Programmable Interrupt Controller
    _resv1 = 1<<10, // (reserved)
    sep    = 1<<11, // SYSENTER and SYSEXIT instructions
    mtrr   = 1<<12, // Memory Type Range Registers
    pge    = 1<<13, // Page Global Enable bit in CR4
    mca    = 1<<14, // Machine check architecture
    cmov   = 1<<15, // Conditional move and FCMOV instructions
    pat    = 1<<16, // Page Attribute Table
    pse_36 = 1<<17, // 36-bit page size extension
    psn    = 1<<18, // Processor Serial Number
    clfsh  = 1<<19, // CLFLUSH instruction (SSE2)
    _resv2 = 1<<20, // (reserved)
    ds     = 1<<21, // Debug store: save trace of executed jumps
    acpi   = 1<<22, // Onboard thermal control MSRs for ACPI
    mmx    = 1<<23, // MMX instructions
    fxsr   = 1<<24, // FXSAVE, FXRESTOR instructions, CR4 bit 9
    sse    = 1<<25, // SSE instructions (a.k.a. Katmai New Instructions)
    sse2   = 1<<26, // SSE2 instructions
    ss     = 1<<27, // CPU cache implements self-snoop
    htt    = 1<<28, // Hyper-threading
    tm     = 1<<29, // Thermal monitor automatically limits temperature
    ia64   = 1<<30, // IA64 processor emulating x86
    pbe    = 1<<31, // Pending Break Enable (PBE# pin) wakeup capability
};

enum struct Ecx_1_Feature_Flags {
    sse3         = 1<<0,  // Prescott New Instructions-SSE3 (PNI)
    pclmulqdq    = 1<<1,  // PCLMULQDQ
    dtes64       = 1<<2,  // 64-bit debug store (edx bit 21)
    monitor      = 1<<3,  // MONITOR and MWAIT instructions (SSE3)
    ds_cpl       = 1<<4,  // CPL qualified debug store
    vmx          = 1<<5,  // Virtual Machine eXtensions
    smx          = 1<<6,  // Safer Mode Extensions (LaGrande)
    est          = 1<<7,  // Enhanced SpeedStep
    tm2          = 1<<8,  // Thermal Monitor 2
    ssse3        = 1<<9,  // Supplemental SSE3 instructions
    cnxt_id      = 1<<10, // L1 Context ID
    sdbg         = 1<<11, // Silicon Debug interface
    fma          = 1<<12, // Fused multiply-add (FMA3)
    cx16         = 1<<13, // CMPXCHG16B instruction
    xtpr         = 1<<14, // Can disable sending task priority messages
    pdcm         = 1<<15, // Perfmon & debug capability
    _resv1       = 1<<16, // (reserved)
    pcid         = 1<<17, // Process context identifiers (CR4 bit 17)
    dca          = 1<<18, // Direct cache access for DMA writes
    sse4_1       = 1<<19, // SSE4.1 instructions
    sse4_2       = 1<<20, // SSE4.2 instructions
    x2apic       = 1<<21, // x2APIC
    movbe        = 1<<22, // MOVBE instruction (big-endian)
    popcnt       = 1<<23, // POPCNT instruction
    tsc_deadline = 1<<24, // APIC implements one-shot operation using a TSC deadline value
    aes          = 1<<25, // AES instruction set
    xsave        = 1<<26, // XSAVE, XRESTOR, XSETBV, XGETBV
    osxsave      = 1<<27, // XSAVE enabled by OS
    avx          = 1<<28, // Advanced Vector Extensions
    f16c         = 1<<29, // F16C (half-precision) FP feature
    rdrnd        = 1<<30, // RDRAND (on-chip random number generator) feature
    hypervisor   = 1<<31, // Hypervisor present (always zero on physical CPUs)
};

enum struct Ebx_7_Extended_Feature_Flags {
    fsgsbase    = 1<<0,  // Access to base of %fs and %gs
    _idk1       = 1<<1,  //
    sgx         = 1<<2,  // Software Guard Extensions
    bmi1        = 1<<3,  // Bit Manipulation Instruction Set 1
    hle         = 1<<4,  // TSX Hardware Lock Elision
    avx2        = 1<<5,  // Advanced Vector Extensions 2
    _idk2       = 1<<6,  //
    smep        = 1<<7,  // Supervisor Mode Execution Prevention
    bmi2        = 1<<8,  // Bit Manipulation Instruction Set 2
    erms        = 1<<9,  // Enhanced REP MOVSB/STOSB
    invpcid     = 1<<10, // INVPCID instruction
    rtm         = 1<<11, // TSX Restricted Transactional Memory
    pqm         = 1<<12, // Platform Quality of Service Monitoring
    _idk3       = 1<<13, //
    mpx         = 1<<14, // Intel MPX (Memory Protection Extensions)
    pqe         = 1<<15, // Platform Quality of Service Enforcement
    avx512_f    = 1<<16, // AVX-512 Foundation
    avx512_dq   = 1<<17, // AVX-512 Doubleword and Quadword Instructions
    rdseed      = 1<<18, // RDSEED instruction
    adx         = 1<<19, // Intel ADX (Multi-Precision Add-Carry Instruction Extensions)
    smap        = 1<<20, // Supervisor Mode Access Prevention
    avx512_ifma = 1<<21, // AVX-512 Integer Fused Multiply-Add Instructions
    pcommit     = 1<<22, // PCOMMIT instruction
    clflushopt  = 1<<23, // CLFLUSHOPT instruction
    clwb        = 1<<24, // CLWB instruction
    intel_pt    = 1<<25, // Intel Processor Trace
    avx512_pf   = 1<<26, // AVX-512 Prefetch Instructions
    avx512_er   = 1<<27, // AVX-512 Exponential and Reciprocal Instructions
    avx512_cd   = 1<<28, // AVX-512 Conflict Detection Instructions
    sha         = 1<<29, // Intel SHA extensions
    avx512_bw   = 1<<30, // AVX-512 Byte and Word Instructions
    avx512_vl   = 1<<31, // AVX-512 Vector Length Extensions
};

enum struct Ecx_7_Extended_Feature_Flags {
    prefetchwt1      = 1<<0,  // PREFETCHWT1 instruction
    avx512_vbmi      = 1<<1,  // AVX-512 Vector Bit Manipulation Instructions
    umip             = 1<<2,  // User-mode Instruction Prevention
    pku              = 1<<3,  // Memory Protection Keys for User-mode pages
    ospke            = 1<<4,  // PKU enabled by OS
    waitpkg          = 1<<5,  // Timed pause and user-level monitor/wait
    avx512_vbmi2     = 1<<6,  // AVX-512 Vector Bit Manipulation Instructions 2
    cet_ss           = 1<<7,  // Control flow enforcement (CET) shadow stack
    gfni             = 1<<8,  // Galois Field instructions
    vaes             = 1<<9,  // Vector AES instruction set (VEX-256/EVEX)
    vpclmulqdq       = 1<<10, // CLMUL instruction set (VEX-256/EVEX)
    avx512_vnni      = 1<<11, // AVX-512 Vector Neural Network Instructions
    avx512_bitalg    = 1<<12, // AVX-512 BITALG instructions
    _resv1           = 1<<13, // (reserved)
    avx512_vpopcntdq = 1<<14, // AVX-512 Vector Population Count Double and Quad-word
    _resv2           = 1<<15, // (reserved)
    intel_5lp        = 1<<16, // 5-level paging
    mawau1           = 1<<17, // The value of userspace MPX Address-Width Adjust used ...
    mawau2           = 1<<18, // ... by the BNDLDX and BNDSTX Intel MPX instructions  ...
    mawau3           = 1<<19, // ... in 64-bit mode
    mawau4           = 1<<20, //
    mawau5           = 1<<21, //
    rdpid            = 1<<22, // Read Processor ID and IA32_TSC_AUX
    _resv3           = 1<<23, // (reserved)
    _resv4           = 1<<24, // (reserved)
    cldemote         = 1<<25, // Cache line demote
    _resv5           = 1<<26, // (reserved)
    movdiri          = 1<<27, //
    movdir64b        = 1<<28, //
    enqcmd           = 1<<29, // Enqueue Stores
    sgx_lc           = 1<<30, // SGX Launch Configuration
    pks              = 1<<31, // Protection keys for supervisor-mode pages
};

enum struct Edx_7_Extended_Feature_Flags {
    _resv1                 = 1<<0,  // (reserved)
    _resv2                 = 1<<1,  // (reserved)
    avx512_4vnniw          = 1<<2,  // AVX-512 4-register Neural Network Instructions
    avx512_4fmaps          = 1<<3,  // AVX-512 4-register Multiply Accumulation Single precision
    fsrm                   = 1<<4,  // Fast Short REP MOVSB
    _resv3                 = 1<<5,  // (reserved)
    _resv4                 = 1<<6,  // (reserved)
    _resv5                 = 1<<7,  // (reserved)
    avx512_vp2intersect    = 1<<8,  // AVX-512 VP2INTERSECT Doubleword and Quadword Instructions
    SRBDS_CTRL             = 1<<9,  // Special Register Buffer Data Sampling Mitigations
    md_clear               = 1<<10, // VERW instruction clears CPU buffers
    _resv6                 = 1<<11, // (reserved)
    _resv7                 = 1<<12, // (reserved)
    tsx_force_abort        = 1<<13, //
    serialize              = 1<<14, // Serialize instruction execution
    hybrid                 = 1<<15, //
    TSXLDTRK               = 1<<16, // TSX suspend load address tracking
    _resv8                 = 1<<17, // (reserved)
    pconfig                = 1<<18, // Platform configuration (Memory Encryption Technologies Instructions)
    lbr                    = 1<<19, // Architectural Last Branch Records
    cet_ibt                = 1<<20, // Control flow enforcement (CET) indirect branch tracking
    _resv9                 = 1<<21, // (reserved)
    amx_bf16               = 1<<22, // Tile computation on bfloat16 numbers
    _resv10                = 1<<23, // (reserved)
    amx_tile               = 1<<24, // Tile architecture
    amx_int8               = 1<<25, // Tile computation on 8-bit integers
    spec_ctrl              = 1<<26,
    // Speculation Control, part of Indirect Branch Control (IBC):
    // Indirect Branch Restricted Speculation (IBRS) and
    // Indirect Branch Prediction Barrier (IBPB)
    stibp                  = 1<<27, // Single Thread Indirect Branch Predictor, part of IBC
    l1d_flush              = 1<<28, // IA32_FLUSH_CMD MSR
    ia32_arch_capabilities = 1<<29, // Speculative Side Channel Mitigations
    ia32_core_capabilities = 1<<30, // Support for a MSR listing model-specific core capabilities
    ssbd                   = 1<<31, // Speculative Store Bypass Disable, as mitigation for Speculative Store Bypass (IA32_SPEC_CTRL)
};

// NOTE(Felix): Left out flags are duplicates from Edx_1_Feature_Flags
enum struct Edx_81_Extended_Feature_Flags {
    syscall   = 1<<11, // SYSCALL and SYSRET instructions
    mp        = 1<<19, // Multiprocessor Capable
    nx        = 1<<20, // NX (no-execute) bit
    mmxext    = 1<<22, // Extended MMX
    fxsr_opt  = 1<<25, // FXSAVE/FXRSTOR optimizations
    pdpe1gb   = 1<<26, // Gibibyte pages
    rdtscp    = 1<<27, // RDTSCP instruction
    l1d_flush = 1<<28, // IA32_FLUSH_CMD MSR
    lm        = 1<<29, // Long mode
    _3dnowext = 1<<30, // Extended 3DNow!
    _3dnow    = 1<<31, // 3DNow!
};

enum struct Ecx_81_Extended_Feature_Flags {
    lahf_lm       = 1<<0,  // LAHF/SAHF in long mode
    cmp_legacy    = 1<<1,  // Hyperthreading not valid
    svm           = 1<<2,  // Secure Virtual Machine
    extapic       = 1<<3,  // Extended APIC space
    cr8_legacy    = 1<<4,  // CR8 in 32-bit mode
    abm           = 1<<5,  // Advanced bit manipulation (lzcnt and popcnt)
    sse4a         = 1<<6,  // SSE4a
    misalignsse   = 1<<7,  // Misaligned SSE mode
    _3dnowprefetch= 1<<8,  // PREFETCH and PREFETCHW instructions
    osvw          = 1<<9,  // OS Visible Workaround
    ibs           = 1<<10, // Instruction Based Sampling
    xop           = 1<<11, // XOP instruction set
    skinit        = 1<<12, // SKINIT/STGI instructions
    wdt           = 1<<13, // Watchdog timer
    _resv1        = 1<<14, // (reserved)
    lwp           = 1<<15, // Light Weight Profiling
    fma4          = 1<<16, // 4 operands fused multiply-add
    tce           = 1<<17, // Translation Cache Extension
    _resv2        = 1<<18, // (reserved)
    nodeid_msr    = 1<<19, // NodeID MSR
    _resv3        = 1<<20, // (reserved)
    tbm           = 1<<21, // Trailing Bit Manipulation
    topoext       = 1<<22, // opology Extensions
    perfctr_core  = 1<<23, // Core performance counter extensions
    perfctr_nb    = 1<<24, // NB performance counter extensions
    _resv4        = 1<<25, // (reserved)
    dbx           = 1<<26, // Data breakpoint extensions
    perftsc       = 1<<27, // Performance TSC
    pcx_l2i       = 1<<28, // L2I perf counter extensions
    _resv5        = 1<<29, // (reserved)
    _resv6        = 1<<30, // (reserved)
    _resv7        = 1<<31, // (reserved)
};

struct Cpu_Info {
    char vendor[0x20];
    char brand[0x40];

    bool is_intel;
    bool is_amd;

    int f_1_ECX;
    int f_1_EDX;
    int f_7_EBX;
    int f_7_ECX;
    int f_7_EDX;
    int f_81_ECX;
    int f_81_EDX;
};

#ifndef FTB_CPU_INFO_IMPL

inline auto query_cpu_feature(Cpu_Info* info, Edx_1_Feature_Flags flag) -> bool;
inline auto query_cpu_feature(Cpu_Info* info, Ecx_1_Feature_Flags flag) -> bool;
inline auto query_cpu_feature(Cpu_Info* info, Ebx_7_Extended_Feature_Flags flag) -> bool;
inline auto query_cpu_feature(Cpu_Info* info, Ecx_7_Extended_Feature_Flags flag) -> bool;
inline auto query_cpu_feature(Cpu_Info* info, Edx_7_Extended_Feature_Flags flag) -> bool;
inline auto query_cpu_feature(Cpu_Info* info, Edx_81_Extended_Feature_Flags flag) -> bool;
inline auto query_cpu_feature(Cpu_Info* info, Ecx_81_Extended_Feature_Flags flag) -> bool;
auto get_cpu_info(Cpu_Info* info) -> void;

#else // implementations

inline auto query_cpu_feature(Cpu_Info* info, Edx_1_Feature_Flags flag) -> bool {
    return info->f_1_EDX & (int)flag;
}

inline auto query_cpu_feature(Cpu_Info* info, Ecx_1_Feature_Flags flag) -> bool {
    return info->f_1_ECX & (int)flag;
}

inline auto query_cpu_feature(Cpu_Info* info, Ebx_7_Extended_Feature_Flags flag) -> bool {
    return info->f_7_EBX & (int)flag;
}

inline auto query_cpu_feature(Cpu_Info* info, Ecx_7_Extended_Feature_Flags flag) -> bool {
    return info->f_7_ECX & (int)flag;
}

inline auto query_cpu_feature(Cpu_Info* info, Edx_7_Extended_Feature_Flags flag) -> bool {
    return info->f_7_EDX & (int)flag;
}

inline auto query_cpu_feature(Cpu_Info* info, Edx_81_Extended_Feature_Flags flag) -> bool {
    return info->f_81_EDX & (int)flag;
}

inline auto query_cpu_feature(Cpu_Info* info, Ecx_81_Extended_Feature_Flags flag) -> bool {
    return info->f_81_ECX & (int)flag;
}

auto get_cpu_info(Cpu_Info* info) -> void {
    *info = {};

    int nIds_ = 0;
    int nExIds_ = 0;

    int register_sets[3][4];

    info->is_intel = false;
    info->is_amd   = false;

    // Calling __cpuid with 0x0 as the function_id argument
    // gets the number of the highest valid function ID.
    platform_independent_cpuid(0, register_sets[0]);
    nIds_ = register_sets[0][0];

    // Capture vendor string
    memset(info->vendor, 0, sizeof(info->vendor));
    memcpy(info->vendor + 0, register_sets[0]+1, sizeof(int));
    memcpy(info->vendor + 4, register_sets[0]+3, sizeof(int));
    memcpy(info->vendor + 8, register_sets[0]+2, sizeof(int));

    if (strcmp(info->vendor, "GenuineIntel") == 0) {
        info->is_intel = true;
    } else if (strcmp(info->vendor, "AuthenticAMD") == 0) {
        info->is_amd = true;
    }

    if (nIds_ >= 1) {
        platform_independent_cpuidex(1, 0, register_sets[0]);
        info->f_1_ECX = register_sets[0][2];
        info->f_1_EDX = register_sets[0][3];

        if (nIds_ >= 7) {
            platform_independent_cpuidex(7, 0, register_sets[1]);
            info->f_7_EBX = register_sets[1][1];
            info->f_7_ECX = register_sets[1][2];
            info->f_7_EDX = register_sets[1][3];
        }
    }

    // Calling __cpuid with 0x80000000 as the function_id argument
    // gets the number of the highest valid extended ID.
    platform_independent_cpuid(0x80000000, register_sets[2]);
    nExIds_ = register_sets[2][0];

    memset(info->brand, 0, sizeof(info->brand));

    // load bitset with flags for function 0x80000001
    if (nExIds_ >= 0x80000001) {
        platform_independent_cpuidex(0x80000001, 0, register_sets[2]);
        info->f_81_ECX = register_sets[2][2];
        info->f_81_EDX = register_sets[2][3];

        // Interpret CPU brand string if reported
        if (nExIds_ >= 0x80000004) {
            platform_independent_cpuidex(0x80000002, 0, register_sets[0]);
            platform_independent_cpuidex(0x80000003, 0, register_sets[1]);
            platform_independent_cpuidex(0x80000004, 0, register_sets[2]);

            memcpy(info->brand +  0, register_sets[0], sizeof(register_sets[0]));
            memcpy(info->brand + 16, register_sets[1], sizeof(register_sets[1]));
            memcpy(info->brand + 32, register_sets[2], sizeof(register_sets[2]));
        }
    }
}

#endif // FTB_CPU_INFO_IMPL
