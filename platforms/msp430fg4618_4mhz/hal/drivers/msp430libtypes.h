/*! \file avrlibtypes.h \brief AVRlib global types and typedefines. */
//*****************************************************************************
//
// File Name	: 'avrlibtypes.h'
// Title		: AVRlib global types and typedefines include file
// Author		: Pascal Stang
// Created		: 7/12/2001
// Revised		: 9/30/2002
// Version		: 1.0
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
//	Description : Type-defines required and used by AVRlib.  Most types are also
//						generally useful.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************


#ifndef MSP430LIBTYPES_H
#define MSP430LIBTYPES_H

#include <inttypes.h>
#include <stdbool.h>

// Made redundant w/ stdbool
//#ifndef WIN32
//	// true/false defines
//	#define FALSE	0
//	#define TRUE	-1
//#endif

// datatype definitions macros
typedef uint8_t  u08;
typedef int8_t  s08;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t  u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t s64;

/* use inttypes.h instead
// C99 standard integer type definitions
typedef unsigned char	uint8_t;
typedef   signed char	int8_t;
typedef unsigned short	uint16_t;
typedef   signed short	int16_t;
typedef unsigned long	uint32_t;
typedef   signed long	int32_t;
typedef unsigned long	uint64_t;
typedef   signed long	int64_t;
*/
// maximum value that can be held
// by unsigned data types (8,16,32bits)
#define MAX_U08	255
#define MAX_U16	65535
#define MAX_U32	4294967295

// maximum values that can be held
// by signed data types (8,16,32bits)
#define MIN_S08	-128
#define MAX_S08	127
#define MIN_S16	-32768
#define MAX_S16	32767
#define MIN_S32	-2147483648
#define MAX_S32	2147483647

#ifndef WIN32
	// more type redefinitions
	typedef unsigned char   BOOL;
	typedef unsigned char	BYTE;
	typedef unsigned int	WORD;
	typedef unsigned long	DWORD;

	typedef unsigned char	UCHAR;
	typedef unsigned int	UINT;
	typedef unsigned short  USHORT;
	typedef unsigned long	ULONG;

	//typedef char			CHAR;
	typedef int				INT;
	typedef long			LONG;
#endif

#endif
