#pragma once

#if defined(_WIN32) || defined(_WIN64)
#pragma message("Compiling for Windows")
#define FTB_WINDOWS
#elseif
#pragma message("Compiling for Linux")
#define FTB_LINUX
#endif
