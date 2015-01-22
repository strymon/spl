/*-------------------------------------------------------------------------
	    Copyright 2013 Damage Control Engineering, LLC

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*-------------------------------------------------------------------------*/
/*!
 \file DcGlobals.h
 \brief Cross-platform global includes, macros, misc tools, etc...
--------------------------------------------------------------------------*/

#define DCMAX(a, b)	((b) < (a) ? (a) : (b))
#define DCMIN(a, b)	((a) < (b) ? (a) : (b))
#define DCABS(a) ((a) >= 0  ? (a) : -(a))
#define DCDIV_UP(N,D) (((N)+((D)-1))/(D))

#define DCDIM(ARRAY)	(sizeof(ARRAY) / sizeof(ARRAY[0]))

typedef float DcSample;
typedef float DcFloat;

typedef unsigned char           uchar;
typedef signed char             int8;
typedef unsigned char           uint8;
typedef short                   int16;
typedef unsigned short          uint16;
typedef int                     int32;

#ifndef _UINT32   // Avoid conflict with typedef in OS/X Leopard system header
typedef unsigned int           uint32;
#define _UINT32  // Avoid conflict with typedef in OS/X Leopard system header
#endif

#ifndef byte
typedef unsigned char byte;
#endif



#define RAW_MAX_POT_VAL 127