LZHAM Lossless Data Compression Codec - Alpha8 - March 6, 2012
Copyright (c) 2009-2012 Richard Geldreich, Jr. <richgel99@gmail.com>
MIT License - http://code.google.com/p/lzham/

lzhamtest_x86/x64 is a simple command line test program that uses the LZHAM codec DLL to compress/decompress single files.

-- Usage examples:

- Compress single file "source_filename" to "compressed_filename":
	lzhamtest_x64 c source_filename compressed_filename
	
- Decompress single file "compressed_filename" to "decompressed_filename":
    lzhamtest_x64 d compressed_filename decompressed_filename

- Compress single file "source_filename" to "compressed_filename", then verify the compressed file decompresses properly to the source file:
	lzhamtest_x64 -v c source_filename compressed_filename

- Recursively compress all files under specified directory and verify that each file decompresses properly:
	lzhamtest_x64 -v a c:\source_path
	
-- Options	
	
- Set dictionary size used during compressed to 1MB (2^20):
	lzhamtest_x64 -d20 c source_filename compressed_filename
	
Valid dictionary sizes are [15,26] for x86, and [15,29] for x64. (See LZHAM_MIN_DICT_SIZE_LOG2, etc. defines in include/lzham.h.)
The x86 version defaults to 64MB (26), and the x64 version defaults to 256MB (28). I wouldn't recommend setting the dictionary size to 
512MB unless your machine has more than 4GB of physical memory.

- Set compression level to fastest:
	lzhamtest_x64 -m0 c source_filename compressed_filename
	
- Set compression level to uber (the default):
	lzhamtest_x64 -m4 c source_filename compressed_filename
	
- For best possible compression, use -d29 to enable the largest dictionary size (512MB) and the -x option which enables more rigorous (but ~4X slower!) parsing:
	lzhamtest_x64 -d29 -x -m4 c source_filename compressed_filename

See lzhamtest_x86/x64.exe's help text for more command line parameters.

-- Compiling LZHAM

- Linux
Alpha5 now supports the pthreads API for threading and GCC built-ins for atomic operations. I've included a Codeblocks project 
"lzhamtest_linux.workspace" that's been tested under 32-bit Ubuntu (Lucid Lynx), The Linux version currently only supports linking statically 
against the LZHAM comp/decomp libs, hasn't been built/tested under 64-bit Linux, and doesn't include makefiles yet.

I've successfully used cbp2mak to create makefiles from the lzham .cbp files:
http://bblean.berlios.de/cbp2mak-0.2.zip

- Windows/Xbox 360
LZHAM can be compiled with Visual Studio 2008 (preferred) or with Codeblocks 10.05 using TDM-GCC x64 (GCC 4.5.0). 
http://www.codeblocks.org/
http://tdm-gcc.tdragon.net/

The Codeblocks workspace is "lzhamtest.workspace". The codec runs a bit slower when compiled with GCC, but the difference is less than 5%.

Visual Studio 2008 solution is "lzham.sln". The codec seems to compile and run fine with Visual Studio 2010 in my limited testing.

The codec compiles for Xbox 360 as well: lzham_x360.sln. Note that I barely spent any time verifying the codec on this platform. 
I made sure Alpha5 compiles for Xbox 360 but has not been retested in a while. I plan on throughly testing the codec on Xbox 360 when time permits.

- ANSI C/C++

LZHAM also supports plain vanilla ANSI C/C++. To see how the codec configures itself check out lzham_core.h and search for "LZHAM_ANSI_CPLUSPLUS". 
All platform specific stuff (unaligned loads, threading, atomic ops, etc.) should be disabled when this macro is defined. Note, the compressor doesn't use threads 
or atomic operations when built this way so it's going to be pretty slow. (The compressor was built from the ground up to be threaded.)

-- Known Problems

I've only compiled/tested LZHAM using the x64 version of TDM-GCC. Even though the Codeblocks project contains both x86 and x64 projects, I've had a few problems
compiling LZHAM using the 32-bit (only) version of TDM-GCC. This will be fixed in the next alpha release.
