#pragma once

#include <basetsd.h>

/*
 * Char and String
 */
#include <tchar.h>
#include <xstring>

using tchar		= TCHAR;

/*
* Common Type Add
* Author	: LWT
* Date		: 2022. 10. 17 
*/
#ifndef __PLATFORM_INDEPENDENT_TYPE_DEFINED__
#define __PLATFORM_INDEPENDENT_TYPE_DEFINED__
using int8		= signed char;
using uint8		= unsigned char;
using int16		= short;
using uint16	= unsigned short;
using int32		= int;
using uint32	= unsigned int;
using int64		= int64_t;	// __int64
using uint64	= uint64_t;	// unsigned __int64
#endif

using uint		= unsigned;
using byte		= uint8;
using UID		= uint64;

using TLHandle	= void*;

#if defined(UNICODE)
using tstring = std::wstring;
#else
using tstring = std::string;
#endif

using uuid = tstring;

using GameObjectTag = uint32;
using Layer = uint32;