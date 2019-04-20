//==================================================================================================================================
//  UEFI Stub Loader: Main Loader
//==================================================================================================================================
//
// Version 2.1
//
// Author:
//  KNNSpeed
//
// Source Code:
//  https://github.com/KNNSpeed/UEFI-Stub-Loader
//
// About this program:
//
// This program is a 64-bit UEFI program loader for UEFI-based systems. It is
// designed to boot the Linux kernel's EFI stub and pass boot arguments from a
// text file to it. This is especially useful for machines whose vendor firmware
// does not support passing arguments to UEFI applications. It can also be used
// to boot any EFI application that can take command line options.
//
// Usage:
//
// Put this program anywhere you want in the EFI system partition and point your
// UEFI firmware to it as a boot option. The default bootable file that UEFI
// firmware looks for is BOOTX64.EFI (or BOOTAA64.EFI for ARM64) in the directory
// /EFI/boot/, so you can also just rename the stub loader file accordingly and
// put it at that location.
//
// You will also need to put your EFI kernel image, usually called "vmlinuz," and
// its corresponding "initrd" file (if the distro uses one) somewhere on the same
// EFI system partition. Lastly, you will need to make a file called
// Kernelcmd.txt--this should be stored in the same folder as the Stub Loader
// itself. See the next section for how to properly format this file.
//
// NOTE: With V2.0, the location of Kernelcmd.txt is different from previous
// versions in order to allow using multiple Linux OSes. Each OS will need its
// own Stub Loader & Kernelcmd.txt in addition to vmlinuz & initrd (if
// applicable), as this allows using the machine's native UEFI boot manager to
// select between them as desired.
//
// Kernelcmd.txt Format and Contents:
//
// Kernelcmd.txt should be stored in UTF-16 format in the same directory as the
// stub loader on the EFI system partition. Windows Notepad and Wordpad can save
// text files in this format (select "Unicode Text Document" or "UTF-16 LE" as
// the encoding format in the "Save As" dialog). Linux users can use gedit or
// xed, saving as a .txt file with UTF-16 encoding. Also, it does not matter if
// the file uses Windows (CRLF) or Unix (LF) line endings, but the file does
// need a 2-byte identification Byte Order Mark (BOM). Don't worry too much about
// the BOM; it gets added automatically by all of the aforementioned editors when
// saving with the correct encoding.
//
// The contents of the text file are simple: only three lines are needed. The
// first line should be the location of the kernel to be booted relative to the
// root of the EFI system partition, e.g. \EFI\ubuntu\vmlinuz.efi, and the second
// line is the string of boot arguments to be passed to the kernel, e.g.
// "root=/dev/nvme0n1p5 initrd=\\EFI\\ubuntu\\initrd.img ro rootfstype=ext4
// debug ignore_loglevel libata.force=dump_id crashkernel=384M-:128M quiet
// splash acpi_rev_override=1 acpi_osi=Linux" (without quotes!). The third line
// should be blank--and make sure there is a third line, as this program
// expects a line break to denote the end of the kernel arguments.** That's it!
//
// ** Technically you could use the remainder of the text file to contain an
// actual text document. You could put this info in there if you wanted, or your
// favorite song lyrics, though the smaller the file the faster it is to load.
//
// NOTE: If for some reason you need to use this with a big endian system, save
// the text file as "Unicode big endian." You will also need to compile this
// program for your big endian target.
//

#include "Stubloader.h"

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  // ImageHandle is this program's own EFI_HANDLE
  // SystemTable is the EFI system table of the machine

  // Initialize the GNU-EFI library
  InitializeLib(ImageHandle, SystemTable);
/*
  From InitializeLib:

  ST = SystemTable;
  BS = SystemTable->BootServices;
  RT = SystemTable->RuntimeServices;

*/
  EFI_STATUS Status;

#ifdef DISABLE_UEFI_WATCHDOG_TIMER
  // Disable watchdog timer for debugging
  Status = BS->SetWatchdogTimer(0, 0, 0, NULL);
  if(EFI_ERROR(Status))
  {
    Print(L"Error stopping watchdog, timeout still counting down...\r\n");
  }
#endif

#ifdef DEBUG_ENABLED // Lite debug version
  Print(L"UEFI Stub Loader - V%d.%d DEBUG\r\n", MAJOR_VER, MINOR_VER);
#else // Release version
  Print(L"UEFI Stub Loader - V%d.%d\r\n", MAJOR_VER, MINOR_VER);
#endif
  Print(L"Copyright (c) 2018-2019 KNNSpeed\r\n\n");

  Print(L"Loading...\r\n\n");

  // Use the known location of this loader to find the drive and file location of Kernelcmd.txt
  // Note: Loadedimage is an EFI_LOADED_IMAGE_PROTOCOL pointer and the data it refers to are the LOADED IMAGE characteristics of STUBLOAD.EFI
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;

  // Get a pointer to the (loaded image) pointer of STUBLOAD.EFI
  // Pointer 1 -> Pointer 2 -> STUBLOAD.EFI
  // OpenProtocol wants Pointer 1 as input to give you Pointer 2.
  Status = ST->BootServices->OpenProtocol(ImageHandle, &LoadedImageProtocol, (void**)&LoadedImage, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if(EFI_ERROR(Status))
  {
    Print(L"LoadedImage OpenProtocol error. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  // Get ready to get filesystem support
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;

  // Get filesystem support on STUBLOAD.EFI's DeviceHandle, then we can access a directory structure.
  Status = ST->BootServices->OpenProtocol(LoadedImage->DeviceHandle, &FileSystemProtocol, (void**)&FileSystem, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if(EFI_ERROR(Status))
  {
    Print(L"FileSystem OpenProtocol error. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  // Want the root directory of the filesystem
  EFI_FILE *CurrentDriveRoot;

  Status = FileSystem->OpenVolume(FileSystem, &CurrentDriveRoot);
  if(EFI_ERROR(Status))
  {
    Print(L"OpenVolume error. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  // Locate Kernelcmd.txt, which should be in the same directory as this STUBLOAD.EFI program
  // ((FILEPATH_DEVICE_PATH*)LoadedImage->FilePath)->PathName is, e.g., \EFI\BOOT\BOOTX64.EFI

  CHAR16 * BootFilePath = ((FILEPATH_DEVICE_PATH*)LoadedImage->FilePath)->PathName;

#ifdef DEBUG_ENABLED
  Print(L"BootFilePath: %s\r\n", BootFilePath);
#endif

  UINTN TxtFilePathPrefixLength = 0;
  UINTN BootFilePathLength = 0;

  while(BootFilePath[BootFilePathLength] != L'\0')
  {
    if(BootFilePath[BootFilePathLength] == L'\\')
    {
      TxtFilePathPrefixLength = BootFilePathLength;
    }
    BootFilePathLength++;
  }
  BootFilePathLength += 1; // For Null Term
  TxtFilePathPrefixLength += 1; // To account for the last '\' in the file path (file path prefix does not get null-terminated)

#ifdef DEBUG_ENABLED
  Print(L"BootFilePathLength: %llu, TxtFilePathPrefixLength: %llu, BootFilePath Size: %llu \r\n", BootFilePathLength, TxtFilePathPrefixLength, StrSize(BootFilePath));
  Keywait(L"\0");
#endif

  CONST CHAR16 TxtFileName[14] = L"Kernelcmd.txt";

  UINTN TxtFilePathPrefixSize = TxtFilePathPrefixLength * sizeof(CHAR16);
  UINTN TxtFilePathSize = TxtFilePathPrefixSize + sizeof(TxtFileName);

  CHAR16 * TxtFilePath;

  Status = ST->BootServices->AllocatePool(EfiBootServicesData, TxtFilePathSize, (void**)&TxtFilePath);
  if(EFI_ERROR(Status))
  {
    Print(L"TxtFilePathPrefix AllocatePool error. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  // Don't really need this. Data is measured to be the right size, meaning every byte in TxtFilePath gets overwritten.
//  ZeroMem(TxtFilePath, TxtFilePathSize);

  CopyMem(TxtFilePath, BootFilePath, TxtFilePathPrefixSize);
  CopyMem(&TxtFilePath[TxtFilePathPrefixLength], TxtFileName, sizeof(TxtFileName));

#ifdef DEBUG_ENABLED
  Print(L"TxtFilePath: %s, TxtFilePath Size: %llu\r\n", TxtFilePath, TxtFilePathSize);
  Keywait(L"\0");
#endif

  // Get ready to open the Kernelcmd.txt file
  EFI_FILE *KernelcmdFile;

  // Open the kernelcmd.txt file and assign it to the KernelcmdFile EFI_FILE variable
  // It turns out the Open command can support directory trees with "\" like in Windows. Neat!
  Status = CurrentDriveRoot->Open(CurrentDriveRoot, &KernelcmdFile, TxtFilePath, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
  if (EFI_ERROR(Status))
  {
    Keywait(L"Kernelcmd.txt file is missing\r\n");
    return Status;
  }

#ifdef DEBUG_ENABLED
  Keywait(L"Kernelcmd.txt file opened.\r\n");
#endif

  // Now to get Kernelcmd.txt's file size
  UINTN FileInfoSize;
  // Need to know the size of the file metadata to get the file metadata...
  Status = KernelcmdFile->GetInfo(KernelcmdFile, &gEfiFileInfoGuid, &FileInfoSize, NULL);
  // GetInfo will intentionally error out and provide the correct FileInfoSize value

#ifdef DEBUG_ENABLED
  Print(L"FileInfoSize: %llu Bytes\r\n", FileInfoSize);
#endif

  // Prep metadata destination
  EFI_FILE_INFO *FileInfo;
  // Reserve memory for file info/attributes and such, to prevent it from getting run over
  Status = ST->BootServices->AllocatePool(EfiBootServicesData, FileInfoSize, (void**)&FileInfo);
  if(EFI_ERROR(Status))
  {
    Print(L"FileInfo AllocatePool error. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  // Actually get the metadata
  Status = KernelcmdFile->GetInfo(KernelcmdFile, &gEfiFileInfoGuid, &FileInfoSize, FileInfo);
  if(EFI_ERROR(Status))
  {
    Print(L"GetInfo error. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

#ifdef SHOW_KERNEL_METADATA
  // Show metadata
  Print(L"FileName: %s\r\n", FileInfo->FileName);
  Print(L"Size: %llu\r\n", FileInfo->Size);
  Print(L"FileSize: %llu\r\n", FileInfo->FileSize);
  Print(L"PhysicalSize: %llu\r\n", FileInfo->PhysicalSize);
  Print(L"Attribute: %llx\r\n", FileInfo->Attribute);
/*
  NOTE: Attributes:

  #define EFI_FILE_READ_ONLY 0x0000000000000001
  #define EFI_FILE_HIDDEN 0x0000000000000002
  #define EFI_FILE_SYSTEM 0x0000000000000004
  #define EFI_FILE_RESERVED 0x0000000000000008
  #define EFI_FILE_DIRECTORY 0x0000000000000010
  #define EFI_FILE_ARCHIVE 0x0000000000000020
  #define EFI_FILE_VALID_ATTR 0x0000000000000037

*/
  Print(L"Created: %02hhu/%02hhu/%04hu - %02hhu:%02hhu:%02hhu.%u\r\n", FileInfo->CreateTime.Month, FileInfo->CreateTime.Day, FileInfo->CreateTime.Year, FileInfo->CreateTime.Hour, FileInfo->CreateTime.Minute, FileInfo->CreateTime.Second, FileInfo->CreateTime.Nanosecond);
  Print(L"Last Modified: %02hhu/%02hhu/%04hu - %02hhu:%02hhu:%02hhu.%u\r\n", FileInfo->ModificationTime.Month, FileInfo->ModificationTime.Day, FileInfo->ModificationTime.Year, FileInfo->ModificationTime.Hour, FileInfo->ModificationTime.Minute, FileInfo->ModificationTime.Second, FileInfo->ModificationTime.Nanosecond);
  Keywait(L"\0");
#endif

  // Read text file into memory now that we know the file size
  CHAR16 * KernelcmdArray;
  // Reserve memory for text file
  Status = ST->BootServices->AllocatePool(EfiBootServicesData, FileInfo->FileSize, (void**)&KernelcmdArray);
  if(EFI_ERROR(Status))
  {
    Print(L"KernelcmdArray AllocatePool error. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  // Actually read the file
  Status = KernelcmdFile->Read(KernelcmdFile, &FileInfo->FileSize, KernelcmdArray);
  if(EFI_ERROR(Status))
  {
    Print(L"KernelcmdArray read error. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

#ifdef DEBUG_ENABLED
  Keywait(L"KernelcmdFile read into memory.\r\n");
#endif

  // UTF-16 format check
  UINT16 BOM_check = UTF16_BOM_LE;
  if(!compare(KernelcmdArray, &BOM_check, 2))
  {
    BOM_check = UTF16_BOM_BE; // Check endianness
    if(compare(KernelcmdArray, &BOM_check, 2))
    {
      Print(L"Error: Kernelcmd.txt has the wrong endianness for this system.\r\n");
    }
    else // Probably missing a BOM
    {
      Print(L"Error: Kernelcmd.txt not formatted as UTF-16/UCS-2 with BOM.\r\n\n");
      Print(L"Q: What is a BOM?\r\n\n");
      Print(L"A: The BOM (Byte Order Mark) is a 2-byte identification sequence\r\n");
      Print(L"(U+FEFF) at the start of a UTF16/UCS-2-encoded file.\r\n");
      Print(L"Unfortunately not all editors add it in, and without\r\n");
      Print(L"a BOM present programs like this one cannot easily tell that a\r\n");
      Print(L"text file is encoded in UTF16/UCS-2.\r\n\n");
      Print(L"Windows Notepad & Wordpad and Linux gedit & xed all add BOMs when\r\n");
      Print(L"saving files as .txt with encoding set to \"Unicode\" (Windows)\r\n");
      Print(L"or \"UTF16\" (Linux), so use one of them to make Kernelcmd.txt.\r\n\n");
    }
    Keywait(L"Please fix the file and try again.\r\n");
    return Status;
  }
  // Parse Kernelcmd.txt file for location of kernel image and command line
  // Kernel image location line will be of format e.g. \EFI\ubuntu\vmlinuz.efi followed by \n or \r\n
  // Command line will just go until the next \n or \r\n, and should just be loaded as a UTF-16 string

  // Get size of kernel image path & command line and populate the data retention variables
  UINT64 FirstLineLength = 0;
  UINT64 KernelPathSize = 0;

  for(UINT64 i = 1; i < ((FileInfo->FileSize) >> 1); i++) // i starts at 1 to skip the BOM, ((FileInfo->FileSize) >> 1) is the number of 2-byte units in the file
  {
    if(KernelcmdArray[i] == L'\n')
    {
      // Account for the L'\n'
      FirstLineLength = i + 1;
      // The extra +1 is to start the command line parse in the correct place
      break;
    }
    else if(KernelcmdArray[i] == L'\r')
    {
      // There'll be a \n after the \r
      FirstLineLength = i + 1 + 1;
      // The extra +1 is to start the command line parse in the correct place
      break;
    }

    if(KernelcmdArray[i] != L' ') // There might be an errant space or two. Ignore them.
    {
      KernelPathSize++;
    }
  }
  UINT64 KernelPathLen = KernelPathSize; // Need this for later
  // Need to add null terminator. Multiply by size of CHAR16 (2 bytes) to get size.
  KernelPathSize = (KernelPathSize + 1) << 1; // (KernelPathSize + 1) * sizeof(CHAR16)

#ifdef DEBUG_ENABLED
  Print(L"KernelPathSize: %llu\r\n", KernelPathSize);
#endif

  // Command line's turn
  UINT32 CmdlineSize = 0; // Linux kernel only takes 256 to 4096 chars depending on architecture. Here's a couple billion.

  for(UINT64 j = FirstLineLength; j < ((FileInfo->FileSize) >> 1); j++)
  {
    if((KernelcmdArray[j] == L'\n') || (KernelcmdArray[j] == L'\r')) // Reached the end of the line
    {
      break;
    }

    CmdlineSize++;
  }
  UINT64 CmdlineLen = CmdlineSize; // Need this for later
  // Need to add null terminator. Multiply by size of CHAR16 (2 bytes) to get size.
  CmdlineSize = (CmdlineSize + 1) << 1; // (CmdlineSize + 1) * sizeof(CHAR16)

#ifdef DEBUG_ENABLED
  Print(L"CmdlineSize: %llu\r\n", CmdlineSize);
#endif

  CHAR16 * KernelPath; // EFI Kernel file's Path
  Status = ST->BootServices->AllocatePool(EfiBootServicesData, KernelPathSize, (void**)&KernelPath);
  if(EFI_ERROR(Status))
  {
    Print(L"KernelPath AllocatePool error. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  CHAR16 * Cmdline; // Command line to pass to EFI kernel
  Status = ST->BootServices->AllocatePool(EfiLoaderData, CmdlineSize, (void**)&Cmdline);
  if(EFI_ERROR(Status))
  {
    Print(L"Cmdline AllocatePool error. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  for(UINT64 i = 1; i < FirstLineLength; i++)
  {
    if((KernelcmdArray[i] == L'\n') || (KernelcmdArray[i] == L'\r'))
    {
      break;
    }

    if(KernelcmdArray[i] != L' ') // There might be an errant space or two. Ignore them.
    {
      KernelPath[i-1] = KernelcmdArray[i]; // i-1 to ignore the 2 bytes of UTF-16 BOM
    }
  }
  KernelPath[KernelPathLen] = L'\0'; // Need to null-terminate this string

  // Command line's turn
  for(UINT64 j = FirstLineLength; j < ((FileInfo->FileSize) >> 1); j++)
  {
    if((KernelcmdArray[j] == L'\n') || (KernelcmdArray[j] == L'\r')) // Reached the end of the line
    {
      break;
    }

    Cmdline[j-FirstLineLength] = KernelcmdArray[j];
  }
  Cmdline[CmdlineLen] = L'\0'; // Need to null-terminate this string

#ifdef DEBUG_ENABLED
  Print(L"Kernel image path: %s\r\nKernel image path size: %u\r\n", KernelPath, KernelPathSize);
  Print(L"Kernel command line: %s\r\nKernel command line size: %u\r\n", Cmdline, CmdlineSize);
  Keywait(L"Loading image... (might take a second or two after pressing a key)\r\n");
#endif

  // Get UEFI device path that corresponds to STUBLOADER's EFI partition
  // Doesn't seem like we can use EFI_SIMPLE_FILE_SYSTEM_PROTOCOL constructs for BS->LoadImage, instead we need to use EFI_DEVICE_PATH_PROTOCOL
  EFI_DEVICE_PATH_PROTOCOL * FullDevicePath;
  FullDevicePath = FileDevicePath(LoadedImage->DeviceHandle, KernelPath); // This allocates memory for us

  // Free pools allocated from before as they are no longer needed
  Status = BS->FreePool(TxtFilePath);
  if(EFI_ERROR(Status))
  {
    Print(L"Error freeing TxtFilePathPrefix pool. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  Status = BS->FreePool(KernelPath);
  if(EFI_ERROR(Status))
  {
    Print(L"Error freeing KernelPath pool. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  Status = BS->FreePool(KernelcmdArray);
  if(EFI_ERROR(Status))
  {
    Print(L"Error freeing KernelcmdArray pool. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  Status = BS->FreePool(FileInfo);
  if(EFI_ERROR(Status))
  {
    Print(L"Error freeing FileInfo pool. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  // Finally time to get the kernel image, which will need its own EFI_HANDLE
  EFI_HANDLE LoadedKernelImageHandle;
  // Load kernel image from its location
  Status = ST->BootServices->LoadImage(FALSE, ImageHandle, FullDevicePath, NULL, 0, &LoadedKernelImageHandle);
  if(EFI_ERROR(Status))
  {
    Print(L"LoadedKernelImageHandle LoadImage error. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  Status = BS->FreePool(FullDevicePath);
  if(EFI_ERROR(Status))
  {
    Print(L"Error freeing FullDevicePath pool. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  // Now to associate the command line with the kernel, which is done by adding the command line to the load options of the loaded kernel image
  EFI_LOADED_IMAGE_PROTOCOL * LoadedKernelImage; // Well this seems familiar...

  Status = ST->BootServices->OpenProtocol(LoadedKernelImageHandle, &LoadedImageProtocol, (void**)&LoadedKernelImage, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if(EFI_ERROR(Status))
  {
    Print(L"LoadedKernelImage OpenProtocol error. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

  LoadedKernelImage->LoadOptions = Cmdline; // This was allocated pool of EfiLoaderData earlier so that it persists into the kernel.
  LoadedKernelImage->LoadOptionsSize = CmdlineSize;

#ifdef DEBUG_ENABLED
  Print(L"Kernel command line: %s\r\nKernel command line size: %u\r\n\n", Cmdline, CmdlineSize);
  Print(L"Verify loaded command line: %s\r\nCommand line size: %u\r\n", LoadedKernelImage->LoadOptions, LoadedKernelImage->LoadOptionsSize);
  Keywait(L"Starting image...\r\n");
#endif

  // Execute kernel EFI image by StartImage
  Status = ST->BootServices->StartImage(LoadedKernelImageHandle, NULL, NULL);

  // If all goes well, this program should never get here.
  Print(L"Status: 0x%llx\r\n", Status);
  Keywait(L"Kernel image returned...\r\n");
  return Status;
}

//==================================================================================================================================
//  Keywait: Pause
//==================================================================================================================================
//
// A simple pause function that waits for user input before continuing.
// Adapted from http://wiki.osdev.org/UEFI_Bare_Bones
//

EFI_STATUS Keywait(CHAR16 *String)
{
  EFI_STATUS Status;
  EFI_INPUT_KEY Key;
  Print(String);

  Status = ST->ConOut->OutputString(ST->ConOut, L"Press any key to continue...");
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  // Clear keystroke buffer
  Status = ST->ConIn->Reset(ST->ConIn, FALSE);
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  // Poll for key
  while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);

  // Clear keystroke buffer (this is just a pause)
  Status = ST->ConIn->Reset(ST->ConIn, FALSE);
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  Print(L"\r\n");

  return Status;
}

//==================================================================================================================================
//  compare: Memory Comparison
//==================================================================================================================================
//
// A simple memory comparison function.
// Returns 1 if the two items are the same; 0 if they're not.
//

// Variable 'comparelength' is in bytes
UINT8 compare(const void* firstitem, const void* seconditem, UINT64 comparelength)
{
  // Using const since this is a read-only operation: absolutely nothing should be changed here.
  const UINT8 *one = firstitem, *two = seconditem;
  for (UINT64 i = 0; i < comparelength; i++)
  {
    if(one[i] != two[i])
    {
      return 0;
    }
  }
  return 1;
}
