#pragma once

#if defined(_WIN32) || defined(_WIN64)
// #pragma message ("Compiling for Windows")
# define FTB_WINDOWS
# define _CRT_SECURE_NO_WARNINGS
# define WIN32_LEAN_AND_MEAN
#else
// #pragma message ("Compiling for Linux")
#define FTB_LINUX
#endif
