/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    basetsd.h

Abstract:

    Type definitions for the basic sized types.

Author:

Revision History:

--*/

#ifndef _BASETSD_H_
#define _BASETSD_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef signed char INT8, *PINT8;
typedef signed short INT16, *PINT16;
typedef signed int INT32, *PINT32;
typedef signed __int64 INT64, *PINT64;
typedef unsigned char UINT8, *PUINT8;
typedef unsigned short UINT16, *PUINT16;
typedef unsigned int UINT32, *PUINT32;
typedef unsigned __int64 UINT64, *PUINT64;

//
// The following types are guaranteed to be signed and 32 bits wide.
//

typedef signed int LONG32, *PLONG32;

//
// The following types are guaranteed to be unsigned and 32 bits wide.
//

typedef unsigned int ULONG32, *PULONG32;
typedef unsigned int DWORD32, *PDWORD32;

#if !defined(_W64)
#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300
#define _W64 __w64
#else
#define _W64
#endif
#endif

//
// The INT_PTR is guaranteed to be the same size as a pointer.  Its
// size with change with pointer size (32/64).  It should be used
// anywhere that a pointer is cast to an integer type. UINT_PTR is
// the unsigned variation.
//
// __int3264 is intrinsic to 64b MIDL but not to old MIDL or to C compiler.
//
#if (501 < __midl)

typedef[public] __int3264 INT_PTR, *PINT_PTR;
typedef[public] unsigned __int3264 UINT_PTR, *PUINT_PTR;

typedef[public] __int3264 LONG_PTR, *PLONG_PTR;
typedef[public] unsigned __int3264 ULONG_PTR, *PULONG_PTR;

#else // midl64
// old midl and C++ compiler

#if defined(_WIN64)
typedef __int64 INT_PTR, *PINT_PTR;
typedef unsigned __int64 UINT_PTR, *PUINT_PTR;

typedef __int64 LONG_PTR, *PLONG_PTR;
typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

#define __int3264 __int64

#else
typedef _W64 int INT_PTR, *PINT_PTR;
typedef _W64 unsigned int UINT_PTR, *PUINT_PTR;

typedef _W64 long LONG_PTR, *PLONG_PTR;
typedef _W64 unsigned long ULONG_PTR, *PULONG_PTR;

#define __int3264 __int32

#endif
#endif // midl64

//
// HALF_PTR is half the size of a pointer it intended for use with
// within strcuture which contain a pointer and two small fields.
// UHALF_PTR is the unsigned variation.
//

#ifdef _WIN64

#define ADDRESS_TAG_BIT 0x40000000000UI64

typedef __int64 SHANDLE_PTR;
typedef unsigned __int64 HANDLE_PTR;
typedef unsigned int UHALF_PTR, *PUHALF_PTR;
typedef int HALF_PTR, *PHALF_PTR;

#if !defined(__midl)
__inline unsigned long HandleToULong(const void *h) { return ((unsigned long)(ULONG_PTR)h); }

__inline long HandleToLong(const void *h) { return ((long)(LONG_PTR)h); }

__inline void *ULongToHandle(const unsigned long h) { return ((void *)(UINT_PTR)h); }

__inline void *LongToHandle(const long h) { return ((void *)(INT_PTR)h); }

__inline unsigned long PtrToUlong(const void *p) { return ((unsigned long)(ULONG_PTR)p); }

__inline unsigned int PtrToUint(const void *p) { return ((unsigned int)(UINT_PTR)p); }

__inline unsigned short PtrToUshort(const void *p) {
    return ((unsigned short)(unsigned long)(ULONG_PTR)p);
}

__inline long PtrToLong(const void *p) { return ((long)(LONG_PTR)p); }

__inline int PtrToInt(const void *p) { return ((int)(INT_PTR)p); }

__inline short PtrToShort(const void *p) { return ((short)(long)(LONG_PTR)p); }

__inline void *IntToPtr(const int i)
// Caution: IntToPtr() sign-extends the int value.
{
    return ((void *)(INT_PTR)i);
}

__inline void *UIntToPtr(const unsigned int ui)
// Caution: UIntToPtr() zero-extends the unsigned int value.
{
    return ((void *)(UINT_PTR)ui);
}

__inline void *LongToPtr(const long l)
// Caution: LongToPtr() sign-extends the long value.
{
    return ((void *)(LONG_PTR)l);
}

__inline void *ULongToPtr(const unsigned long ul)
// Caution: ULongToPtr() zero-extends the unsigned long value.
{
    return ((void *)(ULONG_PTR)ul);
}

#endif // !_midl

#else // !_WIN64

#define ADDRESS_TAG_BIT 0x80000000UL

typedef unsigned short UHALF_PTR, *PUHALF_PTR;
typedef short HALF_PTR, *PHALF_PTR;
typedef _W64 long SHANDLE_PTR;
typedef _W64 unsigned long HANDLE_PTR;

#define HandleToULong(h) ((ULONG)(ULONG_PTR)(h))
#define HandleToLong(h) ((LONG)(LONG_PTR)(h))
#define ULongToHandle(ul) ((HANDLE)(ULONG_PTR)(ul))
#define LongToHandle(h) ((HANDLE)(LONG_PTR)(h))
#define PtrToUlong(p) ((ULONG)(ULONG_PTR)(p))
#define PtrToLong(p) ((LONG)(LONG_PTR)(p))
#define PtrToUint(p) ((UINT)(UINT_PTR)(p))
#define PtrToInt(p) ((INT)(INT_PTR)(p))
#define PtrToUshort(p) ((unsigned short)(ULONG_PTR)(p))
#define PtrToShort(p) ((short)(LONG_PTR)(p))
#define IntToPtr(i) ((VOID *)(INT_PTR)((int)i))
#define UIntToPtr(ui) ((VOID *)(UINT_PTR)((unsigned int)ui))
#define LongToPtr(l) ((VOID *)(LONG_PTR)((long)l))
#define ULongToPtr(ul) ((VOID *)(ULONG_PTR)((unsigned long)ul))

#endif // !_WIN64

#define HandleToUlong(h) HandleToULong(h)
#define UlongToHandle(ul) ULongToHandle(ul)
#define UlongToPtr(ul) ULongToPtr(ul)
#define UintToPtr(ui) UIntToPtr(ui)

#define MAXUINT_PTR (~((UINT_PTR)0))
#define MAXINT_PTR ((INT_PTR)(MAXUINT_PTR >> 1))
#define MININT_PTR (~MAXINT_PTR)

#define MAXULONG_PTR (~((ULONG_PTR)0))
#define MAXLONG_PTR ((LONG_PTR)(MAXULONG_PTR >> 1))
#define MINLONG_PTR (~MAXLONG_PTR)

#define MAXUHALF_PTR ((UHALF_PTR)~0)
#define MAXHALF_PTR ((HALF_PTR)(MAXUHALF_PTR >> 1))
#define MINHALF_PTR (~MAXHALF_PTR)

//
// SIZE_T used for counts or ranges which need to span the range of
// of a pointer.  SSIZE_T is the signed variation.
//

typedef ULONG_PTR SIZE_T, *PSIZE_T;
typedef LONG_PTR SSIZE_T, *PSSIZE_T;

//
// Add Windows flavor DWORD_PTR types
//

typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

//
// The following types are guaranteed to be signed and 64 bits wide.
//

typedef __int64 LONG64, *PLONG64;

//
// The following types are guaranteed to be unsigned and 64 bits wide.
//

typedef unsigned __int64 ULONG64, *PULONG64;
typedef unsigned __int64 DWORD64, *PDWORD64;

//
// Thread affinity.
//

typedef ULONG_PTR KAFFINITY;
typedef KAFFINITY *PKAFFINITY;

#ifdef __cplusplus
}
#endif

#endif // _BASETSD_H_
