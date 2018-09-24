## UEFI Stub Loader
Load the Linux EFI Stub (or any EFI application) on systems that don't support UEFI firmware command lines.

**Version 1.0**

**About this program**  

 This program is a 64-bit UEFI program loader for UEFI-based systems. It is designed to boot the Linux kernel's EFI stub and pass boot arguments from a text file to it. This is especially useful for machines whose vendor firmware does not support passing arguments to UEFI applications. It can also be used to boot any EFI application that can take command line options.  

**Usage**

 Put this program anywhere you want in the EFI system partition and point your UEFI firmware to it as a boot option. You will also need to put your EFI kernel image somewhere on the same partition, and you will need to make a file called Kernelcmd.txt, which should be stored at the root of the partition.  

**Kernelcmd.txt Format and Contents**

 Kernelcmd.txt should be stored in UTF-16 format at the root of the EFI system parition. Windows Notepad and Wordpad can save text files in this format (select "Unicode" as the format in the "Save As" dialog). Linux users can use gedit or xed, saving as a .txt file with UTF16 encoding. Also, it does not matter if the file uses Windows (CRLF) or Unix (LF) line endings, but the file does need a 2-byte identification Byte Order Mark (BOM). Don't worry too much about the BOM; it gets added automatically by all of the aforementioned editors when saving with the correct encoding.  

 The contents of the text file are simple: only three lines are needed. The first line should be the location of the kernel to be booted relative to the root of the EFI system partition, e.g. \EFI\ubuntu\vmlinuz.efi, and the second line is the string of boot arguments to be passed to the kernel, e.g. "root=/dev/nvme0n1p5 initrd=\\EFI\\ubuntu\\initrd.img ro rootfstype=ext4 debug ignore_loglevel libata.force=dump_id crashkernel=384M-:128M quiet splash acpi_rev_override=1 acpi_osi=Linux" (without quotes!). The third line should be blank--and make sure there is a third line, as this program expects a line break to denote the end of the kernel arguments.** That's it!  

 ** Technically you could use the remainder of the text file to contain an actual text document. You could put this info in there if you wanted, or your favorite song lyrics, though the smaller the file the faster it is to load.  

 NOTE: If for some reason you need to use this with a big endian system, save the text file as "Unicode big endian." You will also need to compile this program for your big endian target.  

***See the "Releases" tab for usage information and downloads, and please post any bugs, feature requests, etc. in "Issues."***  

Derived from Simple-UEFI-Bootloader, which is designed for booting custom kernels: https://github.com/KNNSpeed/Simple-UEFI-Bootloader/  

**Loader Features**  

- UEFI 2.x support
- Loads and executes kernels compiled as native 64-bit UEFI applications (like the Linux kernel)
- Passes user-written commands (from a plain UTF16 text file) to loaded EFI applications
- Allows arbitrary placement of itself in addition to kernel images on the EFI system partition
- Fits on a floppy diskette, and some systems can actually boot it from a floppy
- Minimal UEFI development environment tuned for Windows, Mac, and Linux included in repository ***(1)***

***(1)*** *See the below "How to Build from Source" section for complete compilation instructions for each platform, and then all you need to do is put your code in "src" and "inc" in place of mine. Once compiled, your program can be run in the same way as described in "Releases" using a UEFI-supporting VM like Hyper-V or on actual hardware.*  

**Target System Requirements**  

- 64-Bit architecture (only little-endian binaries are provided)  
- Secure Boot must be disabled  

**License and Crediting**  

***TL;DR:***  

Effectively PD (Public Domain) for all code in this repository not already covered by a license (i.e. my original source code), **as long as you give proper credit to this project.** See below for an example of what that might look like--more examples are included in the LICENSE file. If you don't give credit to this project, per the license you aren't allowed to use it. That's pretty much it (and why it's "effectively" or "almost" PD, or "PD with Credit" if I have to give it a nickname).  

Please see the LICENSE file for further information on all licenses covering code created for and used in this project.  

*Example Citation:*  

From KNNSpeed's "UEFI Stub Loader":  
https://github.com/KNNSpeed/UEFI-Stub-Loader  
V1.0, [Date you got it]  

***Slightly More Detailed License Summary:***

If you want to use, copy, modify, and/or distribute this project's original source code, in other words the code in this repository not already covered under any license, simply copy & paste the below 3 lines somewhere reasonable like in an acknowledgements or references section, as a comment in the code, at the bottom of a README or in a LICENSE file, etc. Then, change "[Date you got it]" to the date you acquired the code, and don't sue me if something goes wrong - especially since there's no warranty (and sometimes firmware vendors just don't follow the UEFI spec in unforeseen ways, but it would be great if you posted an issue so I could fix it!). Thanks!

From KNNSpeed's "UEFI Stub Loader":  
https://github.com/KNNSpeed/UEFI-Stub-Loader  
V1.0, [Date you got it]  

(As mentioned in the TL;DR, please see the LICENSE file for further information on all licenses covering code created for and used in this project.)  

**How to Build from Source**  

Requires GCC 7.1.0 or later and Binutils 2.29.1 or later. I cannot make any guarantees whatsoever for earlier versions, especially with the number of compilation and linking flags used.  

***Windows:***  
1. Download and extract or clone this repository into a dedicated folder, preferably somewhere easy like C:\BareMetalx64

2. Download MinGW-w64 "x86_64-posix-seh" from https://sourceforge.net/projects/mingw-w64/ (click "Files" and scroll down - pay attention to the version numbers!).

3. Extract the archive into the "Backend" folder.

4. Open Windows PowerShell or the Command Prompt in the "UEFI-Stub-Loader" folder and type ".\Compile.bat"

    *That's it! It should compile and a binary called "STUBLOAD.EFI" will be output into the "Backend" folder.*

***Mac:***  
1. Download and extract or clone this repository into a dedicated folder, preferably somewhere easy like ~/BareMetalx64

2. Install the latest MacPorts: https://www.macports.org/

3. In Terminal, get the MinGW-w64 package via "sudo port install mingw-w64" ("sudo port install x86_64-w64-mingw32-gcc" might also work)

    NOTE: Make sure that MacPorts downloads a version using the correct GCC and Binutils! You may need to run "sudo port selfupdate" if you aren't freshly installing MacPorts before running the above install command.

4. Once MinGW-w64 is installed, open Terminal in the "UEFI-Stub-Loader" folder and run "./Compile-Mac.sh"

    *That's it! It should compile and a binary called "STUBLOAD.EFI" will be output into the "Backend" folder.*

***Linux:***  

1. Download and extract or clone this repository into a dedicated folder, preferably somewhere easy like ~/BareMetalx64

2. If, in the terminal, "gcc --version" reports GCC 7.1.0 or later and "ld --version" reports 2.29.1 or later, do steps 2a, 2b, and 2c. Otherwise go to step 3.

    2a. Type "which gcc" in the terminal, and make a note of what it says (something like /usr/bin/gcc or /usr/local/bin/gcc)

    2b. Open Compile.sh in an editor of your choice (nano, gedit, vim, etc.) and set the GCC_FOLDER_NAME variable at the top to be the part before "bin" (e.g. /usr or /usr/local, without the last slash). Do the same thing for BINUTILS_FOLDER_NAME, except use the output of "which ld" to get the directory path preceding "bin" instead.

    2c. Now set the terminal to the "UEFI-Stub-Loader" folder and run "./Compile.sh", which should work and output "STUBLOAD.EFI" in the Backend folder. *That's it!*

3. Looks like we need to build GCC & Binutils. Navigate to the "Backend" folder in terminal and do "git clone git://gcc.gnu.org/git/gcc.git" there. This will download a copy of GCC 8.0.0, which is what I have been using (need this version for the Simple-Kernel). If that git link ever changes, you'll need to find wherever the official GCC git repository ran off to.

4. Once GCC has been cloned, in the cloned folder do "contrib/download_prerequisites" and then "./configure -v --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu --prefix=$PWD/../gcc-8 --enable-checking=release --enable-languages=c --disable-multilib"

    NOTE: If you want, you can enable other languages like c++, fortran, objective-c (objc), go, etc. with enable-languages. You can also change the name of the folder it will built into by changing --prefix=[desired folder]. The above command line will configure GCC to be made in a folder called gcc-8 inside the "Backend" folder. Be aware that --prefix requires an absolute path.

5. After configuration completes, do "make -j [twice the number of cores of your CPU]" and go eat lunch. Unfortunately, sometimes building the latest GCC produces build-time errors; I ran into an "aclocal-1.15" issue when building via Linux on Windows (fixed by installing the latest version of Ubuntu on Windows and using the latest autoconf).

6. Now just do "make install" and GCC will be put into the gcc-8 folder from step 4.

7. Next, grab binutils 2.29.1 or later from https://ftp.gnu.org/gnu/binutils/ and extract the archive to Backend.

8. In the extracted Binutils folder, do "mkdir build" and "cd build" before configuring with "../configure --prefix=$PWD/../binutils-binaries --enable-gold --enable-ld=default --enable-plugins --enable-shared --disable-werror"

    NOTE: The "prefix" flag means the same thing as GCC's.

9. Once configuration is finished, do "make -j [twice the number of CPU cores]" and go have dinner.

10. Once make is done making, do "make -k check" and do a crossword or something. There should be a very small number of errors, if any.

11. Finally, do "make install" to install the package into binutils-binaries. Congratulations, you've just built some of the biggest Linux sources ever!

12. Open Compile.sh in an editor of your choice (nano, gedit, vim, etc.) and set the GCC_FOLDER_NAME variable at the top (e.g. gcc-8 without any slashes). Do the same thing for the BINUTILS_FOLDER_NAME, except use the binutils-binaries folder.

13. At long last, you should be able to run "./Compile.sh" from within the "UEFI-Stub-Loader" folder.

    *That's it! It should compile and a binary called "STUBLOAD.EFI" will be output into the "Backend" folder.*

    For more information about building GCC and Binutils, see these: http://www.linuxfromscratch.org/blfs/view/cvs/general/gcc.html & http://www.linuxfromscratch.org/lfs/view/development/chapter06/binutils.html  

**Change Log**

V1.0 (9/23/2018) - Initial release. No bugs with this release so far.

**Acknowledgements**  

- [Nigel Croxon](https://sourceforge.net/u/noxorc/profile/) for [GNU-EFI](https://sourceforge.net/projects/gnu-efi/)
- [UEFI Forum](http://www.uefi.org/) for the [UEFI Specification Version 2.7 (Errata A)](http://www.uefi.org/sites/default/files/resources/UEFI%20Spec%202_7_A%20Sept%206.pdf), as well as for [previous UEFI 2.x specifications](http://www.uefi.org/specifications)
- [OSDev Wiki](http://wiki.osdev.org/Main_Page) for its wealth of available information
- [PhoenixWiki](http://wiki.phoenix.com/wiki/index.php/Category:UEFI) for very handy documentation on UEFI functions
- [Matthew Garrett](https://mjg59.dreamwidth.org/) for detailed information about UEFI firmware behavior (e.g. ["Getting started with UEFI development"](https://mjg59.dreamwidth.org/18773.html?thread=767573))
- [The GNU project](https://www.gnu.org/home.en.html) for [GCC](https://gcc.gnu.org/), a fantastic and versatile compiler, and [Binutils](https://www.gnu.org/software/binutils/), equally fantastic binary utilities
- [MinGW-w64](https://mingw-w64.org/doku.php) for porting GCC & Binutils to Windows
- [Mojca Miklavec](https://github.com/mojca) for [porting MinGW-w64 to MacPorts](https://github.com/macports/macports-ports/tree/master/cross/mingw-w64)
