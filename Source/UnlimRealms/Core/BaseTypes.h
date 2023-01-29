///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

typedef int					ur_int;
typedef unsigned int		ur_uint;
typedef __int16				ur_int16;
typedef unsigned __int16	ur_uint16;
typedef __int32				ur_int32;
typedef unsigned __int32	ur_uint32;
typedef __int64				ur_int64;
typedef unsigned __int64	ur_uint64;
typedef bool				ur_bool;
typedef unsigned char		ur_byte;
typedef size_t				ur_size;
typedef float				ur_float;
typedef double				ur_double;

#define ur_null nullptr

#define ur_array_size(a) (sizeof(a) / sizeof(*a))

#define ur_align(value, alignment) ((value + alignment - 1) / alignment * alignment)
