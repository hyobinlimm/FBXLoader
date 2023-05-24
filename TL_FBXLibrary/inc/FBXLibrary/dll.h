#pragma once

/* If we are we on Windows, we want a single define for it.*/
#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
#define _WIN32
#endif /* _WIN32 */

#if defined(FBX_LIBRARY_BUILD_DLL)
    /* We are building Hashing as a Win32 DLL */
#define FBX_LIBRARY_API __declspec(dllexport)
#else
    /* We are calling Hashing as a Win32 DLL */
	#define FBX_LIBRARY_API __declspec(dllimport)
#endif