//==================================================================================================================================
//  UEFI Stub Loader: Main Header
//==================================================================================================================================
//
// Version 2.0
//
// Author:
//  KNNSpeed
//
// Source Code:
//  https://github.com/KNNSpeed/UEFI-Stub-Loader
//
// This file provides inclusions, #define switches, structure definitions, and function prototypes for the stub loader.
// See Stubloader.c for further details about this program.
//

#ifndef _Stubloader_H
#define _Stubloader_H

#include <efi.h>
#include <efilib.h>

//==================================================================================================================================
// Useful Debugging Code
//==================================================================================================================================
//
// Enable useful debugging prints and convenient key-awaiting pauses
//
//NOTE: Due to little endianness, all printed data at dereferenced pointers is in LITTLE ENDIAN, so each byte (0xXX) is read
// left to right while the byte order is reversed (right to left)!!
//
// Debug binary has this uncommented, release has it commented
//#define ENABLE_DEBUG // Master debug enable switch

#ifdef ENABLE_DEBUG
    #define DISABLE_UEFI_WATCHDOG_TIMER
    #define DEBUG_ENABLED
    #define SHOW_KERNEL_METADATA
#endif


//==================================================================================================================================
// Text File UCS-2 Definitions
//==================================================================================================================================
//
// LE - Little endian
// BE - Big endian
//

#define UTF8_BOM_LE 0xBFBBEF
#define UTF8_BOM_BE 0xEFBBBF

#define UTF16_BOM_LE 0xFEFF
#define UTF16_BOM_BE 0xFFFE

//==================================================================================================================================
// Function Prototypes
//==================================================================================================================================
//
// Function prototypes for functions used in the loader
//

EFI_STATUS Keywait(CHAR16 *String);
UINT8 compare(const void* firstitem, const void* seconditem, UINT64 comparelength);

#endif
