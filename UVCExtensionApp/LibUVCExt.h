#pragma once

#include <windows.h>

#ifdef _WIN32
#    ifdef LIBRARY_EXPORTS
#        define LIBRARY_API __declspec(dllexport)
#    else
#        define LIBRARY_API __declspec(dllimport)
#    endif
#elif
#    define LIBRARY_API
#endif

#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)

EXTERN_DLL_EXPORT bool LibUVCInit(void);
EXTERN_DLL_EXPORT bool LibUVCWriteControl(BYTE* controlPacket, int length, ULONG* pReadCount);
EXTERN_DLL_EXPORT bool LibUVCReadControl(BYTE* controlPacket, int length, ULONG* pReadCount);
EXTERN_DLL_EXPORT bool LibUVCDeInit(void);