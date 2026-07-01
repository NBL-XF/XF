# Comments extracted from `src/core/stb/stb_image.h`

Version: `v1.0.3`

Source: `src/core/stb/stb_image.h`

## Comment 1

stb_image - v2.30 - public domain image loader - http://nothings.org/stb
no warranty implied; use at your own risk

Do this:
#define STB_IMAGE_IMPLEMENTATION
before you include this file in *one* C or C++ file to create the implementation.

// i.e. it should look like this:
#include ...
#include ...
#include ...
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

You can #define STBI_ASSERT(x) before the #include to avoid using assert.h.
And #define STBI_MALLOC, STBI_REALLOC, and STBI_FREE to avoid using malloc,realloc,free


QUICK NOTES:
Primarily of interest to game developers and other people who can
avoid problematic images and only need the trivial interface

JPEG baseline & progressive (12 bpc/arithmetic not supported, same as stock IJG lib)
PNG 1/2/4/8/16-bit-per-channel

TGA (not sure what subset, if a subset)
BMP non-1bpp, non-RLE
PSD (composited view only, no extra channels, 8/16 bit-per-channel)

GIF (*comp always reports as 4-channel)
HDR (radiance rgbE format)
PIC (Softimage PIC)
PNM (PPM and PGM binary only)

Animated GIF still needs a proper API, but here's one way to do it:
http://gist.github.com/urraka/685d9a6340b26b830d49

- decode from memory or through FILE (define STBI_NO_STDIO to remove code)
- decode from arbitrary I/O callbacks
- SIMD acceleration on x86/x64 (SSE2) and ARM (NEON)

Full documentation under "DOCUMENTATION" below.


LICENSE

See end of file for license information.

RECENT REVISION HISTORY:

2.30  (2024-05-31) avoid erroneous gcc warning
2.29  (2023-05-xx) optimizations
2.28  (2023-01-29) many error fixes, security errors, just tons of stuff
2.27  (2021-07-11) document stbi_info better, 16-bit PNM support, bug fixes
2.26  (2020-07-13) many minor fixes
2.25  (2020-02-02) fix warnings
2.24  (2020-02-02) fix warnings; thread-local failure_reason and flip_vertically
2.23  (2019-08-11) fix clang static analysis warning
2.22  (2019-03-04) gif fixes, fix warnings
2.21  (2019-02-25) fix typo in comment
2.20  (2019-02-07) support utf8 filenames in Windows; fix warnings and platform ifdefs
2.19  (2018-02-11) fix warning
2.18  (2018-01-30) fix warnings
2.17  (2018-01-29) bugfix, 1-bit BMP, 16-bitness query, fix warnings
2.16  (2017-07-23) all functions have 16-bit variants; optimizations; bugfixes
2.15  (2017-03-18) fix png-1,2,4; all Imagenet JPGs; no runtime SSE detection on GCC
2.14  (2017-03-03) remove deprecated STBI_JPEG_OLD; fixes for Imagenet JPGs
2.13  (2016-12-04) experimental 16-bit API, only for PNG so far; fixes
2.12  (2016-04-02) fix typo in 2.11 PSD fix that caused crashes
2.11  (2016-04-02) 16-bit PNGS; enable SSE2 in non-gcc x64
RGB-format JPEG; remove white matting in PSD;
allocate large structures on the stack;
correct channel count for PNG & BMP
2.10  (2016-01-22) avoid warning introduced in 2.09
2.09  (2016-01-16) 16-bit TGA; comments in PNM files; STBI_REALLOC_SIZED

See end of file for full revision history.


============================    Contributors    =========================

Image formats                          Extensions, features
Sean Barrett (jpeg, png, bmp)          Jetro Lauha (stbi_info)
Nicolas Schulz (hdr, psd)              Martin "SpartanJ" Golini (stbi_info)
Jonathan Dummer (tga)                  James "moose2000" Brown (iPhone PNG)
Jean-Marc Lienher (gif)                Ben "Disch" Wenger (io callbacks)
Tom Seddon (pic)                       Omar Cornut (1/2/4-bit PNG)
Thatcher Ulrich (psd)                  Nicolas Guillemot (vertical flip)
Ken Miller (pgm, ppm)                  Richard Mitton (16-bit PSD)
github:urraka (animated gif)           Junggon Kim (PNM comments)
Christopher Forseth (animated gif)     Daniel Gibson (16-bit TGA)
socks-the-fox (16-bit PNG)
Jeremy Sawicki (handle all ImageNet JPGs)
Optimizations & bugfixes                  Mikhail Morozov (1-bit BMP)
Fabian "ryg" Giesen                    Anael Seghezzi (is-16-bit query)
Arseny Kapoulkine                      Simon Breuss (16-bit PNM)
John-Mark Allen
Carmelo J Fdez-Aguera

Bug & warning fixes
Marc LeBlanc            David Woo          Guillaume George     Martins Mozeiko
Christpher Lloyd        Jerry Jansson      Joseph Thomson       Blazej Dariusz Roszkowski
Phil Jordan                                Dave Moore           Roy Eltham
Hayaki Saito            Nathan Reed        Won Chun
Luke Graham             Johan Duparc       Nick Verigakis       the Horde3D community
Thomas Ruf              Ronny Chevalier                         github:rlyeh
Janez Zemva             John Bartholomew   Michal Cichon        github:romigrou
Jonathan Blow           Ken Hamada         Tero Hanninen        github:svdijk
Eugene Golushkov        Laurent Gomila     Cort Stratton        github:snagar
Aruelien Pocheville     Sergio Gonzalez    Thibault Reuille     github:Zelex
Cass Everitt            Ryamond Barbiero                        github:grim210
Paul Du Bois            Engin Manap        Aldo Culquicondor    github:sammyhw
Philipp Wiesemann       Dale Weiler        Oriol Ferrer Mesia   github:phprus
Josh Tobin              Neil Bickford      Matthew Gregan       github:poppolopoppo
Julian Raschke          Gregory Mullen     Christian Floisand   github:darealshinji
Baldur Karlsson         Kevin Schmidt      JR Smith             github:Michaelangel007
Brad Weinberger    Matvey Cherevko      github:mosra
Luca Sas                Alexander Veselov  Zack Middleton       [reserved]
Ryan C. Gordon          [reserved]                              [reserved]
DO NOT ADD YOUR NAME HERE

Jacko Dirks

To add your name to the credits, pick a random blank space in the middle and fill it.
80% of merge conflicts on stb PRs are due to people adding their name at the end
of the credits.

## Comment 2

have to read a byte to reset feof()'s flag

## Comment 3

push byte back onto stream if valid.

## Comment 4

UTF8

## Comment 5

UTF8

## Comment 6

UTF8

## Comment 7

even part

## Comment 8

odd part

## Comment 9

even part

## Comment 10

odd part

## Comment 11

treat this as EOF so we fail.

## Comment 12

int cinfo = cmf >> 4;

## Comment 13

Init algorithm:
{
int i;   // use <= to match clearly with spec
for (i=0; i <= 143; ++i)     stbi__zdefault_length[i]   = 8;
for (   ; i <= 255; ++i)     stbi__zdefault_length[i]   = 9;
for (   ; i <= 279; ++i)     stbi__zdefault_length[i]   = 7;
for (   ; i <= 287; ++i)     stbi__zdefault_length[i]   = 8;

for (i=0; i <=  31; ++i)     stbi__zdefault_distance[i] = 5;
}

## Comment 14

pixels

## Comment 15

filter mode per row

## Comment 16

>>=  1;

## Comment 17

0b11111111

## Comment 18

0b01010101

## Comment 19

0b01001001

## Comment 20

0b00010001

## Comment 21

0b00100001

## Comment 22

0b01000001

## Comment 23

0b10000001

## Comment 24

0b00000001

## Comment 25

bpp = 32 and pad = 0

## Comment 26

you have to have at least one entry!

## Comment 27

Image Descriptor

## Comment 28

fallthrough

## Comment 29

fallthrough

## Comment 30

revision history:
2.20  (2019-02-07) support utf8 filenames in Windows; fix warnings and platform ifdefs
2.19  (2018-02-11) fix warning
2.18  (2018-01-30) fix warnings
2.17  (2018-01-29) change sbti__shiftsigned to avoid clang -O2 bug
1-bit BMP
_is_16_bit api
avoid warnings
2.16  (2017-07-23) all functions have 16-bit variants;
STBI_NO_STDIO works again;
compilation fixes;
fix rounding in unpremultiply;
optimize vertical flip;
disable raw_len validation;
documentation fixes
2.15  (2017-03-18) fix png-1,2,4 bug; now all Imagenet JPGs decode;
warning fixes; disable run-time SSE detection on gcc;
uniform handling of optional "return" values;
thread-safe initialization of zlib tables
2.14  (2017-03-03) remove deprecated STBI_JPEG_OLD; fixes for Imagenet JPGs
2.13  (2016-11-29) add 16-bit API, only supported for PNG right now
2.12  (2016-04-02) fix typo in 2.11 PSD fix that caused crashes
2.11  (2016-04-02) allocate large structures on the stack
remove white matting for transparent PSD
fix reported channel count for PNG & BMP
re-enable SSE2 in non-gcc 64-bit
support RGB-formatted JPEG
read 16-bit PNGs (only as 8-bit)
2.10  (2016-01-22) avoid warning introduced in 2.09 by STBI_REALLOC_SIZED
2.09  (2016-01-16) allow comments in PNM files
16-bit-per-pixel TGA (not bit-per-component)
info() for TGA could break due to .hdr handling
info() for BMP to shares code instead of sloppy parse
can use STBI_REALLOC_SIZED if allocator doesn't support realloc
code cleanup
2.08  (2015-09-13) fix to 2.07 cleanup, reading RGB PSD as RGBA
2.07  (2015-09-13) fix compiler warnings
partial animated GIF support
limited 16-bpc PSD support
#ifdef unused functions
bug with < 92 byte PIC,PNM,HDR,TGA
2.06  (2015-04-19) fix bug where PSD returns wrong '*comp' value
2.05  (2015-04-19) fix bug in progressive JPEG handling, fix warning
2.04  (2015-04-15) try to re-enable SIMD on MinGW 64-bit
2.03  (2015-04-12) extra corruption checking (mmozeiko)
stbi_set_flip_vertically_on_load (nguillemot)
fix NEON support; fix mingw support
2.02  (2015-01-19) fix incorrect assert, fix warning
2.01  (2015-01-17) fix various warnings; suppress SIMD on gcc 32-bit without -msse2
2.00b (2014-12-25) fix STBI_MALLOC in progressive JPEG
2.00  (2014-12-25) optimize JPG, including x86 SSE2 & NEON SIMD (ryg)
progressive JPEG (stb)
PGM/PPM support (Ken Miller)
STBI_MALLOC,STBI_REALLOC,STBI_FREE
GIF bugfix -- seemingly never worked
STBI_NO_*, STBI_ONLY_*
1.48  (2014-12-14) fix incorrectly-named assert()
1.47  (2014-12-14) 1/2/4-bit PNG support, both direct and paletted (Omar Cornut & stb)
optimize PNG (ryg)
fix bug in interlaced PNG with user-specified channel count (stb)
1.46  (2014-08-26)
fix broken tRNS chunk (colorkey-style transparency) in non-paletted PNG
1.45  (2014-08-16)
fix MSVC-ARM internal compiler error by wrapping malloc
1.44  (2014-08-07)
various warning fixes from Ronny Chevalier
1.43  (2014-07-15)
fix MSVC-only compiler problem in code changed in 1.42
1.42  (2014-07-09)
don't define _CRT_SECURE_NO_WARNINGS (affects user code)
fixes to stbi__cleanup_jpeg path
added STBI_ASSERT to avoid requiring assert.h
1.41  (2014-06-25)
fix search&replace from 1.36 that messed up comments/error messages
1.40  (2014-06-22)
fix gcc struct-initialization warning
1.39  (2014-06-15)
fix to TGA optimization when req_comp != number of components in TGA;
fix to GIF loading because BMP wasn't rewinding (whoops, no GIFs in my test suite)
add support for BMP version 5 (more ignored fields)
1.38  (2014-06-06)
suppress MSVC warnings on integer casts truncating values
fix accidental rename of 'skip' field of I/O
1.37  (2014-06-04)
remove duplicate typedef
1.36  (2014-06-03)
convert to header file single-file library
if de-iphone isn't set, load iphone images color-swapped instead of returning NULL
1.35  (2014-05-27)
various warnings
fix broken STBI_SIMD path
fix bug where stbi_load_from_file no longer left file pointer in correct place
fix broken non-easy path for 32-bit BMP (possibly never used)
TGA optimization by Arseny Kapoulkine
1.34  (unknown)
use STBI_NOTUSED in stbi__resample_row_generic(), fix one more leak in tga failure case
1.33  (2011-07-14)
make stbi_is_hdr work in STBI_NO_HDR (as specified), minor compiler-friendly improvements
1.32  (2011-07-13)
support for "info" function for all supported filetypes (SpartanJ)
1.31  (2011-06-20)
a few more leak fixes, bug in PNG handling (SpartanJ)
1.30  (2011-06-11)
added ability to load files via callbacks to accomidate custom input streams (Ben Wenger)
removed deprecated format-specific test/load functions
removed support for installable file formats (stbi_loader) -- would have been broken for IO callbacks anyway
error cases in bmp and tga give messages and don't leak (Raymond Barbiero, grisha)
fix inefficiency in decoding 32-bit BMP (David Woo)
1.29  (2010-08-16)
various warning fixes from Aurelien Pocheville
1.28  (2010-08-01)
fix bug in GIF palette transparency (SpartanJ)
1.27  (2010-08-01)
cast-to-stbi_uc to fix warnings
1.26  (2010-07-24)
fix bug in file buffering for PNG reported by SpartanJ
1.25  (2010-07-17)
refix trans_data warning (Won Chun)
1.24  (2010-07-12)
perf improvements reading from files on platforms with lock-heavy fgetc()
minor perf improvements for jpeg
deprecated type-specific functions so we'll get feedback if they're needed
attempt to fix trans_data warning (Won Chun)
1.23    fixed bug in iPhone support
1.22  (2010-07-10)
removed image *writing* support
stbi_info support from Jetro Lauha
GIF support from Jean-Marc Lienher
iPhone PNG-extensions from James Brown
warning-fixes from Nicolas Schulz and Janez Zemva (i.stbi__err. Janez (U+017D)emva)
1.21    fix use of 'stbi_uc' in header (reported by jon blow)
1.20    added support for Softimage PIC, by Tom Seddon
1.19    bug in interlaced PNG corruption check (found by ryg)
1.18  (2008-08-02)
fix a threading bug (local mutable static)
1.17    support interlaced PNG
1.16    major bugfix - stbi__convert_format converted one too many pixels
1.15    initialize some fields for thread safety
1.14    fix threadsafe conversion bug
header-file-only version (#define STBI_HEADER_FILE_ONLY before including)
1.13    threadsafe
1.12    const qualifiers in the API
1.11    Support installable IDCT, colorspace conversion routines
1.10    Fixes for 64-bit (don't use "unsigned long")
optimized upsampling by Fabian "ryg" Giesen
1.09    Fix format-conversion for PSD code (bad global variables!)
1.08    Thatcher Ulrich's PSD code integrated by Nicolas Schulz
1.07    attempt to fix C++ warning/errors again
1.06    attempt to fix C++ warning/errors again
1.05    fix TGA loading to return correct *comp and use good luminance calc
1.04    default float alpha is 1, not 255; use 'void *' for stbi_image_free
1.03    bugfixes to STBI_NO_STDIO, STBI_NO_HDR
1.02    support for (subset of) HDR files, float interface for preferred access to them
1.01    fix bug: possible bug in handling right-side up bmps... not sure
fix bug: the stbi__bmp_load() and stbi__tga_load() functions didn't work at all
1.00    interface to zlib that skips zlib header
0.99    correct handling of alpha in palette
0.98    TGA loader by lonesock; dynamically add loaders (untested)
0.97    jpeg errors on too large a file; also catch another malloc failure
0.96    fix detection of invalid v value - particleman@mollyrocket forum
0.95    during header scan, seek to markers in case of padding
0.94    STBI_NO_STDIO to disable stdio usage; rename all #defines the same
0.93    handle jpegtran output; verbose errors
0.92    read 4,8,16,24,32-bit BMP files of several formats
0.91    output 24-bit Windows 3.0 BMP files
0.90    fix a few more warnings; bump version number to approach 1.0
0.61    bugfixes due to Marc LeBlanc, Christopher Lloyd
0.60    fix compiling as c++
0.59    fix warnings: merge Dave Moore's -Wall fixes
0.58    fix bug: zlib uncompressed mode len/nlen was wrong endian
0.57    fix bug: jpg last huffman symbol before marker was >9 bits but less than 16 available
0.56    fix bug: zlib uncompressed mode len vs. nlen
0.55    fix bug: restart_interval not initialized to 0
0.54    allow NULL for 'int *comp'
0.53    fix bug in png 3->4; speedup png decoding
0.52    png handles req_comp=3,4 directly; minor cleanup; jpeg comments
0.51    obey req_comp requests, 1-component jpegs return as 1-component,
on 'test' only check type, not whether we support this variant
0.50  (2006-11-19)
first released version

## Comment 31

------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------

## Comment 32

define STB_IMAGE_IMPLEMENTATION

## Comment 33

i.e. it should look like this:
include ...
include ...
include ...
define STB_IMAGE_IMPLEMENTATION
include "stb_image.h"

## Comment 34

ifndef STBI_INCLUDE_STB_IMAGE_H
define STBI_INCLUDE_STB_IMAGE_H

## Comment 35

DOCUMENTATION

Limitations:
   - no 12-bit-per-channel JPEG
   - no JPEGs with arithmetic coding
   - GIF always returns *comp=4

Basic usage (see HDR discussion below for HDR usage):
   int x,y,n;
   unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
   // ... process data if not NULL ...
   // ... x = width, y = height, n = # 8-bit components per pixel ...
   // ... replace '0' with '1'..'4' to force that many components per pixel
   // ... but 'n' will always be the number that it would have been if you said 0
   stbi_image_free(data);

Standard parameters:
   int *x                 -- outputs image width in pixels
   int *y                 -- outputs image height in pixels
   int *channels_in_file  -- outputs # of image components in image file
   int desired_channels   -- if non-zero, # of image components requested in result

The return value from an image loader is an 'unsigned char *' which points
to the pixel data, or NULL on an allocation failure or if the image is
corrupt or invalid. The pixel data consists of *y scanlines of *x pixels,
with each pixel consisting of N interleaved 8-bit components; the first
pixel pointed to is top-left-most in the image. There is no padding between
image scanlines or between pixels, regardless of format. The number of
components N is 'desired_channels' if desired_channels is non-zero, or
*channels_in_file otherwise. If desired_channels is non-zero,
*channels_in_file has the number of components that _would_ have been
output otherwise. E.g. if you set desired_channels to 4, you will always
get RGBA output, but you can check *channels_in_file to see if it's trivially
opaque because e.g. there were only 3 channels in the source image.

An output image with N components has the following components interleaved
in this order in each pixel:

    N=#comp     components
      1           grey
      2           grey, alpha
      3           red, green, blue
      4           red, green, blue, alpha

If image loading fails for any reason, the return value will be NULL,
and *x, *y, *channels_in_file will be unchanged. The function
stbi_failure_reason() can be queried for an extremely brief, end-user
unfriendly explanation of why the load failed. Define STBI_NO_FAILURE_STRINGS
to avoid compiling these strings at all, and STBI_FAILURE_USERMSG to get slightly
more user-friendly ones.

Paletted PNG, BMP, GIF, and PIC images are automatically depalettized.

To query the width, height and component count of an image without having to
decode the full file, you can use the stbi_info family of functions:

  int x,y,n,ok;
  ok = stbi_info(filename, &x, &y, &n);
  // returns ok=1 and sets x, y, n if image is a supported format,
  // 0 otherwise.

Note that stb_image pervasively uses ints in its public API for sizes,
including sizes of memory buffers. This is now part of the API and thus
hard to change without causing breakage. As a result, the various image
loaders all have certain limits on image size; these differ somewhat
by format but generally boil down to either just under 2GB or just under
1GB. When the decoded image would be larger than this, stb_image decoding
will fail.

Additionally, stb_image will reject image files that have any of their
dimensions set to a larger value than the configurable STBI_MAX_DIMENSIONS,
which defaults to 2**24 = 16777216 pixels. Due to the above memory limit,
the only way to have an image with such dimensions load correctly
is for it to have a rather extreme aspect ratio. Either way, the
assumption here is that such larger images are likely to be malformed
or malicious. If you do need to load an image with individual dimensions
larger than that, and it still fits in the overall size limit, you can
#define STBI_MAX_DIMENSIONS on your own to be something larger.

===========================================================================

UNICODE:

  If compiling for Windows and you wish to use Unicode filenames, compile
  with
      #define STBI_WINDOWS_UTF8
  and pass utf8-encoded filenames. Call stbi_convert_wchar_to_utf8 to convert
  Windows wchar_t filenames to utf8.

===========================================================================

Philosophy

stb libraries are designed with the following priorities:

   1. easy to use
   2. easy to maintain
   3. good performance

Sometimes I let "good performance" creep up in priority over "easy to maintain",
and for best performance I may provide less-easy-to-use APIs that give higher
performance, in addition to the easy-to-use ones. Nevertheless, it's important
to keep in mind that from the standpoint of you, a client of this library,
all you care about is #1 and #3, and stb libraries DO NOT emphasize #3 above all.

Some secondary priorities arise directly from the first two, some of which
provide more explicit reasons why performance can't be emphasized.

   - Portable ("ease of use")
   - Small source code footprint ("easy to maintain")
   - No dependencies ("ease of use")

===========================================================================

I/O callbacks

I/O callbacks allow you to read from arbitrary sources, like packaged
files or some other source. Data read from callbacks are processed
through a small internal buffer (currently 128 bytes) to try to reduce
overhead.

The three functions you must define are "read" (reads some bytes of data),
"skip" (skips some bytes of data), "eof" (reports if the stream is at the end).

===========================================================================

SIMD support

The JPEG decoder will try to automatically use SIMD kernels on x86 when
supported by the compiler. For ARM Neon support, you must explicitly
request it.

(The old do-it-yourself SIMD API is no longer supported in the current
code.)

On x86, SSE2 will automatically be used when available based on a run-time
test; if not, the generic C versions are used as a fall-back. On ARM targets,
the typical path is to have separate builds for NEON and non-NEON devices
(at least this is true for iOS and Android). Therefore, the NEON support is
toggled by a build flag: define STBI_NEON to get NEON loops.

If for some reason you do not want to use any of SIMD code, or if
you have issues compiling it, you can disable it entirely by
defining STBI_NO_SIMD.

===========================================================================

HDR image support   (disable by defining STBI_NO_HDR)

stb_image supports loading HDR images in general, and currently the Radiance
.HDR file format specifically. You can still load any file through the existing
interface; if you attempt to load an HDR file, it will be automatically remapped
to LDR, assuming gamma 2.2 and an arbitrary scale factor defaulting to 1;
both of these constants can be reconfigured through this interface:

    stbi_hdr_to_ldr_gamma(2.2f);
    stbi_hdr_to_ldr_scale(1.0f);

(note, do not use _inverse_ constants; stbi_image will invert them
appropriately).

Additionally, there is a new, parallel interface for loading files as
(linear) floats to preserve the full dynamic range:

   float *data = stbi_loadf(filename, &x, &y, &n, 0);

If you load LDR images through this interface, those images will
be promoted to floating point values, run through the inverse of
constants corresponding to the above:

    stbi_ldr_to_hdr_scale(1.0f);
    stbi_ldr_to_hdr_gamma(2.2f);

Finally, given a filename (or an open file or memory block--see header
file for details) containing image data, you can query for the "most
appropriate" interface to use (that is, whether the image is HDR or
not), using:

    stbi_is_hdr(char *filename);

===========================================================================

iPhone PNG support:

We optionally support converting iPhone-formatted PNGs (which store
premultiplied BGRA) back to RGB, even though they're internally encoded
differently. To enable this conversion, call
stbi_convert_iphone_png_to_rgb(1).

Call stbi_set_unpremultiply_on_load(1) as well to force a divide per
pixel to remove any premultiplied alpha *only* if the image file explicitly
says there's premultiplied data (currently only happens in iPhone images,
and only if iPhone convert-to-rgb processing is on).

===========================================================================

ADDITIONAL CONFIGURATION

 - You can suppress implementation of any of the decoders to reduce
   your code footprint by #defining one or more of the following
   symbols before creating the implementation.

       STBI_NO_JPEG
       STBI_NO_PNG
       STBI_NO_BMP
       STBI_NO_PSD
       STBI_NO_TGA
       STBI_NO_GIF
       STBI_NO_HDR
       STBI_NO_PIC
       STBI_NO_PNM   (.ppm and .pgm)

 - You can request *only* certain decoders and suppress all other ones
   (this will be more forward-compatible, as addition of new decoders
   doesn't require you to disable them explicitly):

       STBI_ONLY_JPEG
       STBI_ONLY_PNG
       STBI_ONLY_BMP
       STBI_ONLY_PSD
       STBI_ONLY_TGA
       STBI_ONLY_GIF
       STBI_ONLY_HDR
       STBI_ONLY_PIC
       STBI_ONLY_PNM   (.ppm and .pgm)

  - If you use STBI_NO_PNG (or _ONLY_ without PNG), and you still
    want the zlib decoder to be available, #define STBI_SUPPORT_ZLIB

 - If you define STBI_MAX_DIMENSIONS, stb_image will reject images greater
   than that size (in either width or height) without further processing.
   This is to let programs in the wild set an upper bound to prevent
   denial-of-service attacks on untrusted data, as one could generate a
   valid image of gigantic dimensions and force stb_image to allocate a
   huge block of memory and spend disproportionate time decoding it. By
   default this is set to (1 << 24), which is 16777216, but that's still
   very big.

## Comment 36

ifndef STBI_NO_STDIO
include <stdio.h>
endif // STBI_NO_STDIO

## Comment 37

define STBI_VERSION 1

## Comment 38

include <stdlib.h>

## Comment 39

ifdef __cplusplus

## Comment 40

endif

## Comment 41

ifndef STBIDEF
ifdef STB_IMAGE_STATIC
define STBIDEF static
else
define STBIDEF extern
endif
endif

## Comment 42

////////////////////////////////////////////////////////////////////////////

PRIMARY API - works on images of any type

## Comment 43

load image by filename, open file, or memory buffer

## Comment 44

//////////////////////////////////

8-bits-per-channel interface

## Comment 45

ifndef STBI_NO_STDIO

## Comment 46

for stbi_load_from_file, file pointer is left pointing immediately after image
endif

## Comment 47

ifndef STBI_NO_GIF

## Comment 48

endif

## Comment 49

ifdef STBI_WINDOWS_UTF8

## Comment 50

endif

## Comment 51

//////////////////////////////////

16-bits-per-channel interface

## Comment 52

ifndef STBI_NO_STDIO

## Comment 53

endif

## Comment 54

//////////////////////////////////

float-per-channel interface

ifndef STBI_NO_LINEAR

## Comment 55

ifndef STBI_NO_STDIO

## Comment 56

endif
endif

## Comment 57

ifndef STBI_NO_HDR

## Comment 58

endif // STBI_NO_HDR

## Comment 59

ifndef STBI_NO_LINEAR

## Comment 60

endif // STBI_NO_LINEAR

## Comment 61

stbi_is_hdr is always defined, but always returns false if STBI_NO_HDR

## Comment 62

ifndef STBI_NO_STDIO

## Comment 63

endif // STBI_NO_STDIO

## Comment 64

get a VERY brief reason for failure
on most compilers (and ALL modern mainstream compilers) this is threadsafe

## Comment 65

free the loaded image -- this is just free()

## Comment 66

get image dimensions & components without fully decoding

## Comment 67

ifndef STBI_NO_STDIO

## Comment 68

endif

## Comment 69

for image formats that explicitly notate that they have premultiplied alpha,
we just return the colors as stored in the file. set this flag to force
unpremultiplication. results are undefined if the unpremultiply overflow.

## Comment 70

indicate whether we should process iphone images back to canonical format,
or just pass them through "as-is"

## Comment 71

flip the image vertically, so the first pixel in the output array is the bottom left

## Comment 72

as above, but only applies to images loaded on the thread that calls the function
this function is only available if your compiler supports thread-local variables;
calling it will fail to link if your compiler doesn't

## Comment 73

ZLIB client - used by PNG, available for other purposes

## Comment 74

ifdef __cplusplus

## Comment 75

endif

## Comment 76

//   end header file   /////////////////////////////////////////////////////
endif // STBI_INCLUDE_STB_IMAGE_H

## Comment 77

ifdef STB_IMAGE_IMPLEMENTATION

## Comment 78

if defined(STBI_ONLY_JPEG) || defined(STBI_ONLY_PNG) || defined(STBI_ONLY_BMP) \

## Comment 79

ifndef STBI_ONLY_JPEG
define STBI_NO_JPEG
endif
ifndef STBI_ONLY_PNG
define STBI_NO_PNG
endif
ifndef STBI_ONLY_BMP
define STBI_NO_BMP
endif
ifndef STBI_ONLY_PSD
define STBI_NO_PSD
endif
ifndef STBI_ONLY_TGA
define STBI_NO_TGA
endif
ifndef STBI_ONLY_GIF
define STBI_NO_GIF
endif
ifndef STBI_ONLY_HDR
define STBI_NO_HDR
endif
ifndef STBI_ONLY_PIC
define STBI_NO_PIC
endif
ifndef STBI_ONLY_PNM
define STBI_NO_PNM
endif
endif

## Comment 80

if defined(STBI_NO_PNG) && !defined(STBI_SUPPORT_ZLIB) && !defined(STBI_NO_ZLIB)
define STBI_NO_ZLIB
endif

## Comment 81

include <stdarg.h>
include <stddef.h> // ptrdiff_t on osx
include <stdlib.h>
include <string.h>
include <limits.h>

## Comment 82

if !defined(STBI_NO_LINEAR) || !defined(STBI_NO_HDR)
include <math.h>  // ldexp, pow
endif

## Comment 83

ifndef STBI_NO_STDIO
include <stdio.h>
endif

## Comment 84

ifndef STBI_ASSERT
include <assert.h>
define STBI_ASSERT(x) assert(x)
endif

## Comment 85

ifdef __cplusplus
define STBI_EXTERN extern "C"
else
define STBI_EXTERN extern
endif

## Comment 86

ifndef _MSC_VER
ifdef __cplusplus
define stbi_inline inline
else
define stbi_inline
endif
else
define stbi_inline __forceinline
endif

## Comment 87

ifndef STBI_NO_THREAD_LOCALS
if defined(__cplusplus) &&  __cplusplus >= 201103L
define STBI_THREAD_LOCAL       thread_local
elif defined(__GNUC__) && __GNUC__ < 5
define STBI_THREAD_LOCAL       __thread
elif defined(_MSC_VER)
define STBI_THREAD_LOCAL       __declspec(thread)
elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
define STBI_THREAD_LOCAL       _Thread_local
endif

## Comment 88

ifndef STBI_THREAD_LOCAL
if defined(__GNUC__)
define STBI_THREAD_LOCAL       __thread
endif
endif
endif

## Comment 89

if defined(_MSC_VER) || defined(__SYMBIAN32__)

## Comment 90

else
include <stdint.h>

## Comment 91

endif

## Comment 92

should produce compiler error if size is wrong

## Comment 93

ifdef _MSC_VER
define STBI_NOTUSED(v)  (void)(v)
else
define STBI_NOTUSED(v)  (void)sizeof(v)
endif

## Comment 94

ifdef _MSC_VER
define STBI_HAS_LROTL
endif

## Comment 95

ifdef STBI_HAS_LROTL
define stbi_lrot(x,y)  _lrotl(x,y)
else
define stbi_lrot(x,y)  (((x) << (y)) | ((x) >> (-(y) & 31)))
endif

## Comment 96

if defined(STBI_MALLOC) && defined(STBI_FREE) && (defined(STBI_REALLOC) || defined(STBI_REALLOC_SIZED))
ok
elif !defined(STBI_MALLOC) && !defined(STBI_FREE) && !defined(STBI_REALLOC) && !defined(STBI_REALLOC_SIZED)
ok
else
error "Must define all or none of STBI_MALLOC, STBI_FREE, and STBI_REALLOC (or STBI_REALLOC_SIZED)."
endif

## Comment 97

ifndef STBI_MALLOC
define STBI_MALLOC(sz)           malloc(sz)
define STBI_REALLOC(p,newsz)     realloc(p,newsz)
define STBI_FREE(p)              free(p)
endif

## Comment 98

ifndef STBI_REALLOC_SIZED
define STBI_REALLOC_SIZED(p,oldsz,newsz) STBI_REALLOC(p,newsz)
endif

## Comment 99

x86/x64 detection
if defined(__x86_64__) || defined(_M_X64)
define STBI__X64_TARGET
elif defined(__i386) || defined(_M_IX86)
define STBI__X86_TARGET
endif

## Comment 100

if defined(__GNUC__) && defined(STBI__X86_TARGET) && !defined(__SSE2__) && !defined(STBI_NO_SIMD)
gcc doesn't support sse2 intrinsics unless you compile with -msse2,
which in turn means it gets to use SSE2 everywhere. This is unfortunate,
but previous attempts to provide the SSE2 functions with runtime
detection caused numerous issues. The way architecture extensions are
exposed in GCC/Clang is, sadly, not really suited for one-file libs.
New behavior: if compiled with -msse2, we use SSE2 without any
detection; if not, we don't use it at all.
define STBI_NO_SIMD
endif

## Comment 101

if defined(__MINGW32__) && defined(STBI__X86_TARGET) && !defined(STBI_MINGW_ENABLE_SSE2) && !defined(STBI_NO_SIMD)
Note that __MINGW32__ doesn't actually mean 32-bit, so we have to avoid STBI__X64_TARGET

32-bit MinGW wants ESP to be 16-byte aligned, but this is not in the
Windows ABI and VC++ as well as Windows DLLs don't maintain that invariant.
As a result, enabling SSE2 on 32-bit MinGW is dangerous when not
simultaneously enabling "-mstackrealign".

See https://github.com/nothings/stb/issues/81 for more information.

So default to no SSE2 on 32-bit MinGW. If you've read this far and added
-mstackrealign to your build settings, feel free to #define STBI_MINGW_ENABLE_SSE2.
define STBI_NO_SIMD
endif

## Comment 102

if !defined(STBI_NO_SIMD) && (defined(STBI__X86_TARGET) || defined(STBI__X64_TARGET))
define STBI_SSE2
include <emmintrin.h>

## Comment 103

ifdef _MSC_VER

## Comment 104

if _MSC_VER >= 1400  // not VC6
include <intrin.h> // __cpuid

## Comment 105

else

## Comment 106

endif

## Comment 107

define STBI_SIMD_ALIGN(type, name) __declspec(align(16)) type name

## Comment 108

if !defined(STBI_NO_JPEG) && defined(STBI_SSE2)

## Comment 109

endif

## Comment 110

else // assume GCC-style if not VC++
define STBI_SIMD_ALIGN(type, name) type name __attribute__((aligned(16)))

## Comment 111

if !defined(STBI_NO_JPEG) && defined(STBI_SSE2)

## Comment 112

If we're even attempting to compile this on GCC/Clang, that means
-msse2 is on, which means the compiler is allowed to use SSE2
instructions at will, and so are we.

## Comment 113

endif

## Comment 114

endif
endif

## Comment 115

ARM NEON
if defined(STBI_NO_SIMD) && defined(STBI_NEON)
undef STBI_NEON
endif

## Comment 116

ifdef STBI_NEON
include <arm_neon.h>
ifdef _MSC_VER
define STBI_SIMD_ALIGN(type, name) __declspec(align(16)) type name
else
define STBI_SIMD_ALIGN(type, name) type name __attribute__((aligned(16)))
endif
endif

## Comment 117

ifndef STBI_SIMD_ALIGN
define STBI_SIMD_ALIGN(type, name) type name
endif

## Comment 118

ifndef STBI_MAX_DIMENSIONS
define STBI_MAX_DIMENSIONS (1 << 24)
endif

## Comment 119

/////////////////////////////////////////////

 stbi__context struct and start_xxx functions

## Comment 120

stbi__context structure is our basic context used by all images, so it
contains all the IO context, plus some basic image information

## Comment 121

initialize a memory-decode context

## Comment 122

initialize a callback-based context

## Comment 123

ifndef STBI_NO_STDIO

## Comment 124

static void stop_file(stbi__context *s) { }

## Comment 125

endif // !STBI_NO_STDIO

## Comment 126

conceptually rewind SHOULD rewind to the beginning of the stream,
but we just rewind to the beginning of the initial buffer, because
we only use it after doing 'test', which only ever looks at at most 92 bytes

## Comment 127

ifndef STBI_NO_JPEG

## Comment 128

endif

## Comment 129

ifndef STBI_NO_PNG

## Comment 130

endif

## Comment 131

ifndef STBI_NO_BMP

## Comment 132

endif

## Comment 133

ifndef STBI_NO_TGA

## Comment 134

endif

## Comment 135

ifndef STBI_NO_PSD

## Comment 136

endif

## Comment 137

ifndef STBI_NO_HDR

## Comment 138

endif

## Comment 139

ifndef STBI_NO_PIC

## Comment 140

endif

## Comment 141

ifndef STBI_NO_GIF

## Comment 142

endif

## Comment 143

ifndef STBI_NO_PNM

## Comment 144

endif

## Comment 145

ifdef STBI_THREAD_LOCAL

## Comment 146

endif

## Comment 147

ifndef STBI_NO_FAILURE_STRINGS

## Comment 148

endif

## Comment 149

stb_image uses ints pervasively, including for offset calculations.
therefore the largest decoded image size we can support with the
current code, even on 64-bit targets, is INT_MAX. this is not a
significant limitation for the intended use case.

we do, however, need to make sure our size calculations don't
overflow. hence a few helper functions for size calculations that
multiply integers together, making sure that they're non-negative
and no overflow occurs.

## Comment 150

return 1 if the sum is valid, 0 on overflow.
negative terms are considered invalid.

## Comment 151

now 0 <= b <= INT_MAX, hence also
0 <= INT_MAX - b <= INTMAX.
And "a + b <= INT_MAX" (which might overflow) is the
same as a <= INT_MAX - b (no overflow)

## Comment 152

returns 1 if the product is valid, 0 on overflow.
negative factors are considered invalid.

## Comment 153

portable way to check for no overflows in a*b

## Comment 154

if !defined(STBI_NO_JPEG) || !defined(STBI_NO_PNG) || !defined(STBI_NO_TGA) || !defined(STBI_NO_HDR)
returns 1 if "a*b + add" has no negative terms/factors and doesn't overflow

## Comment 155

endif

## Comment 156

returns 1 if "a*b*c + add" has no negative terms/factors and doesn't overflow

## Comment 157

returns 1 if "a*b*c*d + add" has no negative terms/factors and doesn't overflow
if !defined(STBI_NO_LINEAR) || !defined(STBI_NO_HDR) || !defined(STBI_NO_PNM)

## Comment 158

endif

## Comment 159

if !defined(STBI_NO_JPEG) || !defined(STBI_NO_PNG) || !defined(STBI_NO_TGA) || !defined(STBI_NO_HDR)
mallocs with size overflow checking

## Comment 160

endif

## Comment 161

if !defined(STBI_NO_LINEAR) || !defined(STBI_NO_HDR) || !defined(STBI_NO_PNM)

## Comment 162

endif

## Comment 163

returns 1 if the sum of two signed ints is valid (between -2^31 and 2^31-1 inclusive), 0 on overflow.

## Comment 164

returns 1 if the product of two ints fits in a signed short, 0 on overflow.

## Comment 165

stbi__err - error
stbi__errpf - error returning pointer to float
stbi__errpuc - error returning pointer to unsigned char

## Comment 166

ifdef STBI_NO_FAILURE_STRINGS
define stbi__err(x,y)  0
elif defined(STBI_FAILURE_USERMSG)
define stbi__err(x,y)  stbi__err(y)
else
define stbi__err(x,y)  stbi__err(x)
endif

## Comment 167

define stbi__errpf(x,y)   ((float *)(size_t) (stbi__err(x,y)?NULL:NULL))
define stbi__errpuc(x,y)  ((unsigned char *)(size_t) (stbi__err(x,y)?NULL:NULL))

## Comment 168

ifndef STBI_NO_LINEAR

## Comment 169

endif

## Comment 170

ifndef STBI_NO_HDR

## Comment 171

endif

## Comment 172

ifndef STBI_THREAD_LOCAL
define stbi__vertically_flip_on_load  stbi__vertically_flip_on_load_global
else

## Comment 173

define stbi__vertically_flip_on_load  (stbi__vertically_flip_on_load_set       \

## Comment 174

endif // STBI_THREAD_LOCAL

## Comment 175

test the formats with a very explicit header first (at least a FOURCC
or distinctive magic number first)
ifndef STBI_NO_PNG

## Comment 176

endif
ifndef STBI_NO_BMP

## Comment 177

endif
ifndef STBI_NO_GIF

## Comment 178

endif
ifndef STBI_NO_PSD

## Comment 179

else

## Comment 180

endif
ifndef STBI_NO_PIC

## Comment 181

endif

## Comment 182

then the formats that can end up attempting to load with just 1 or 2
bytes matching expectations; these are prone to false positives, so
try them later
ifndef STBI_NO_JPEG

## Comment 183

endif
ifndef STBI_NO_PNM

## Comment 184

endif

## Comment 185

ifndef STBI_NO_HDR

## Comment 186

endif

## Comment 187

ifndef STBI_NO_TGA
test tga last because it's a crappy test!

## Comment 188

endif

## Comment 189

swap row0 with row1

## Comment 190

ifndef STBI_NO_GIF

## Comment 191

endif

## Comment 192

it is the responsibility of the loaders to make sure we get either 8 or 16 bit.

## Comment 193

@TODO: move stbi__convert_format to here

## Comment 194

it is the responsibility of the loaders to make sure we get either 8 or 16 bit.

## Comment 195

@TODO: move stbi__convert_format16 to here
@TODO: special case RGB-to-Y (and RGBA-to-YA) for 8-bit-to-16-bit case to keep more precision

## Comment 196

if !defined(STBI_NO_HDR) && !defined(STBI_NO_LINEAR)

## Comment 197

endif

## Comment 198

ifndef STBI_NO_STDIO

## Comment 199

if defined(_WIN32) && defined(STBI_WINDOWS_UTF8)

## Comment 200

endif

## Comment 201

if defined(_WIN32) && defined(STBI_WINDOWS_UTF8)

## Comment 202

endif

## Comment 203

if defined(_WIN32) && defined(STBI_WINDOWS_UTF8)

## Comment 204

if defined(_MSC_VER) && _MSC_VER >= 1400

## Comment 205

else

## Comment 206

endif

## Comment 207

elif defined(_MSC_VER) && _MSC_VER >= 1400

## Comment 208

else

## Comment 209

endif

## Comment 210

need to 'unget' all the characters in the IO buffer

## Comment 211

need to 'unget' all the characters in the IO buffer

## Comment 212

endif //!STBI_NO_STDIO

## Comment 213

ifndef STBI_NO_GIF

## Comment 214

endif

## Comment 215

ifndef STBI_NO_LINEAR

## Comment 216

ifndef STBI_NO_HDR

## Comment 217

endif

## Comment 218

ifndef STBI_NO_STDIO

## Comment 219

endif // !STBI_NO_STDIO

## Comment 220

endif // !STBI_NO_LINEAR

## Comment 221

these is-hdr-or-not is defined independent of whether STBI_NO_LINEAR is
defined, for API simplicity; if STBI_NO_LINEAR is defined, it always
reports false!

## Comment 222

ifndef STBI_NO_HDR

## Comment 223

else

## Comment 224

endif

## Comment 225

ifndef STBI_NO_STDIO

## Comment 226

ifndef STBI_NO_HDR

## Comment 227

else

## Comment 228

endif

## Comment 229

endif // !STBI_NO_STDIO

## Comment 230

ifndef STBI_NO_HDR

## Comment 231

else

## Comment 232

endif

## Comment 233

ifndef STBI_NO_LINEAR

## Comment 234

endif

## Comment 235

////////////////////////////////////////////////////////////////////////////

Common code used by all image loaders

## Comment 236

at end of file, treat same as if from memory, but need to handle case
where s->img_buffer isn't pointing to safe memory, e.g. 0-byte file

## Comment 237

if defined(STBI_NO_JPEG) && defined(STBI_NO_HDR) && defined(STBI_NO_PIC) && defined(STBI_NO_PNM)
nothing
else

## Comment 238

if feof() is true, check if buffer = end
special case: we've only got the special 0 character at the end

## Comment 239

endif

## Comment 240

if defined(STBI_NO_JPEG) && defined(STBI_NO_PNG) && defined(STBI_NO_BMP) && defined(STBI_NO_PSD) && defined(STBI_NO_TGA) && defined(STBI_NO_GIF) && defined(STBI_NO_PIC)
nothing
else

## Comment 241

endif

## Comment 242

if defined(STBI_NO_PNG) && defined(STBI_NO_TGA) && defined(STBI_NO_HDR) && defined(STBI_NO_PNM)
nothing
else

## Comment 243

endif

## Comment 244

if defined(STBI_NO_JPEG) && defined(STBI_NO_PNG) && defined(STBI_NO_PSD) && defined(STBI_NO_PIC)
nothing
else

## Comment 245

endif

## Comment 246

if defined(STBI_NO_PNG) && defined(STBI_NO_PSD) && defined(STBI_NO_PIC)
nothing
else

## Comment 247

endif

## Comment 248

if defined(STBI_NO_BMP) && defined(STBI_NO_TGA) && defined(STBI_NO_GIF)
nothing
else

## Comment 249

endif

## Comment 250

ifndef STBI_NO_BMP

## Comment 251

endif

## Comment 252

define STBI__BYTECAST(x)  ((stbi_uc) ((x) & 255))  // truncate int to byte without warnings

## Comment 253

if defined(STBI_NO_JPEG) && defined(STBI_NO_PNG) && defined(STBI_NO_BMP) && defined(STBI_NO_PSD) && defined(STBI_NO_TGA) && defined(STBI_NO_GIF) && defined(STBI_NO_PIC) && defined(STBI_NO_PNM)
nothing
else
////////////////////////////////////////////////////////////////////////////

 generic converter from built-in img_n to req_comp
   individual types do this automatically as much as possible (e.g. jpeg
   does all cases internally since it needs to colorspace convert anyway,
   and it never has alpha, so very few cases ). png can automatically
   interleave an alpha=255 channel, but falls back to this for other cases

 assume data buffer is malloced, so malloc a new one and free that one
 only failure mode is malloc failing

## Comment 254

endif

## Comment 255

if defined(STBI_NO_PNG) && defined(STBI_NO_BMP) && defined(STBI_NO_PSD) && defined(STBI_NO_TGA) && defined(STBI_NO_GIF) && defined(STBI_NO_PIC) && defined(STBI_NO_PNM)
nothing
else

## Comment 256

define STBI__COMBO(a,b)  ((a)*8+(b))
define STBI__CASE(a,b)   case STBI__COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
convert source image with img_n components to one with req_comp components;
avoid switch per pixel, so use switch per scanline and massive macros

## Comment 257

undef STBI__CASE

## Comment 258

endif

## Comment 259

if defined(STBI_NO_PNG) && defined(STBI_NO_PSD)
nothing
else

## Comment 260

endif

## Comment 261

if defined(STBI_NO_PNG) && defined(STBI_NO_PSD)
nothing
else

## Comment 262

define STBI__COMBO(a,b)  ((a)*8+(b))
define STBI__CASE(a,b)   case STBI__COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
convert source image with img_n components to one with req_comp components;
avoid switch per pixel, so use switch per scanline and massive macros

## Comment 263

undef STBI__CASE

## Comment 264

endif

## Comment 265

ifndef STBI_NO_LINEAR

## Comment 266

compute number of non-alpha components

## Comment 267

endif

## Comment 268

ifndef STBI_NO_HDR
define stbi__float2int(x)   ((int) (x))

## Comment 269

compute number of non-alpha components

## Comment 270

endif

## Comment 271

////////////////////////////////////////////////////////////////////////////

 "baseline" JPEG/JFIF decoder

   simple implementation
     - doesn't support delayed output of y-dimension
     - simple interface (only one output format: 8-bit interleaved RGB)
     - doesn't try to recover corrupt jpegs
     - doesn't allow partial loading, loading multiple at once
     - still fast on x86 (copying globals into locals doesn't help x86)
     - allocates lots of intermediate memory (full size of all components)
       - non-interleaved case requires this anyway
       - allows good upsampling (see next)
   high-quality
     - upsampled channels are bilinearly interpolated, even across blocks
     - quality integer IDCT derived from IJG's 'slow'
   performance
     - fast huffman; reasonable integer IDCT
     - some SIMD kernels for common paths on targets with SSE2/NEON
     - uses a lot of intermediate memory, could cache poorly

## Comment 272

ifndef STBI_NO_JPEG

## Comment 273

huffman decoding acceleration
define FAST_BITS   9  // larger handles more cases; smaller stomps less cache

## Comment 274

weirdly, repacking this into AoS is a 10% speed loss, instead of a win

## Comment 275

sizes for components, interleaved MCUs

## Comment 276

definition of jpeg image component

## Comment 277

kernels

## Comment 278

build size list for each symbol (from JPEG spec)

## Comment 279

compute actual symbols (from jpeg spec)

## Comment 280

compute delta to add to code to compute symbol id

## Comment 281

compute largest code + 1 for this size, preshifted as needed later

## Comment 282

build non-spec acceleration table; 255 is flag for not-accelerated

## Comment 283

build a table that decodes both magnitude and value of small ACs in
one go.

## Comment 284

magnitude code followed by receive_extend code

## Comment 285

if the result is small enough, we can fit it in fast_ac table

## Comment 286

(1 << n) - 1

## Comment 287

decode a jpeg huffman value from the bitstream

## Comment 288

look at the top FAST_BITS and determine what symbol ID it is,
if the code is <= FAST_BITS

## Comment 289

naive test is to shift the code_buffer down so k bits are
valid, then test against maxcode. To speed this up, we've
preshifted maxcode left so that it has (16-k) 0s at the
end; in other words, regardless of the number of bits, it
wants to be compared against something shifted to have 16;
that way we don't need to shift inside the loop.

## Comment 290

error! code not found

## Comment 291

convert the huffman code to the symbol id

## Comment 292

convert the id to a symbol

## Comment 293

bias[n] = (-1<<n) + 1

## Comment 294

combined JPEG 'receive' and JPEG 'extend', since baseline
always extends everything it receives.

## Comment 295

get some unsigned bits

## Comment 296

j->code_bits;

## Comment 297

given a value that's at position X in the zigzag stream,
where does it appear in the 8x8 matrix coded as row-major?

## Comment 298

let corrupt input sample past end

## Comment 299

decode one 64-entry block--

## Comment 300

0 all the ac values now so we can do it 32-bits at a time

## Comment 301

decode AC components, see JPEG spec

## Comment 302

decode into unzigzag'd location

## Comment 303

decode into unzigzag'd location

## Comment 304

first scan for DC coefficient, must be first

## Comment 305

refinement scan for DC coefficient

## Comment 306

@OPTIMIZE: store non-zigzagged during the decode passes,
and only de-zigzag when dequantizing

## Comment 307

j->eob_run;

## Comment 308

j->eob_run;

## Comment 309

refinement scan for these AC coefficients

## Comment 310

j->eob_run;

## Comment 311

r=15 s=0 should write 16 0s, so we just do
a run of 15 0s and then write s (which is 0),
so we don't have to do anything special here

## Comment 312

sign bit

## Comment 313

advance by r

## Comment 314

r;

## Comment 315

take a -128..127 value and stbi__clamp it and convert to 0..255

## Comment 316

trick to use a single test to catch both cases

## Comment 317

define stbi__f2f(x)  ((int) (((x) * 4096 + 0.5)))
define stbi__fsh(x)  ((x) * 4096)

## Comment 318

derived from jidctint -- DCT_ISLOW
define STBI__IDCT_1D(s0,s1,s2,s3,s4,s5,s6,s7) \

## Comment 319

columns

## Comment 320

if all zeroes, shortcut -- this avoids dequantizing 0s and IDCTing

## Comment 321

no shortcut                 0     seconds
   (1|2|3|4|5|6|7)==0          0     seconds
   all separate               -0.047 seconds
   1 && 2|3 && 4|5 && 6|7:    -0.047 seconds

## Comment 322

constants scaled things up by 1<<12; let's bring them back
down, but keep 2 extra bits of precision

## Comment 323

no fast case since the first 1D IDCT spread components out

## Comment 324

constants scaled things up by 1<<12, plus we had 1<<2 from first
loop, plus horizontal and vertical each scale by sqrt(8) so together
we've got an extra 1<<3, so 1<<17 total we need to remove.
so we want to round that, which means adding 0.5 * 1<<17,
aka 65536. Also, we'll end up with -128 to 127 that we want
to encode as 0..255 by adding 128, so we'll add that before the shift

## Comment 325

tried computing the shifts into temps, or'ing the temps to see
if any were out of range, but that was slower

## Comment 326

ifdef STBI_SSE2
sse2 integer IDCT. not the fastest possible implementation but it
produces bit-identical results to the generic C version so it's
fully "transparent".

## Comment 327

This is constructed to match our regular (generic) integer IDCT exactly.

## Comment 328

dot product constant: even elems=x, odd elems=y
define dct_const(x,y)  _mm_setr_epi16((x),(y),(x),(y),(x),(y),(x),(y))

## Comment 329

out(0) = c0[even]*x + c0[odd]*y   (c0, x, y 16-bit, out 32-bit)
out(1) = c1[even]*x + c1[odd]*y
define dct_rot(out0,out1, x,y,c0,c1) \

## Comment 330

out = in << 12  (in 16-bit, out 32-bit)
define dct_widen(out, in) \

## Comment 331

wide add
define dct_wadd(out, a, b) \

## Comment 332

wide sub
define dct_wsub(out, a, b) \

## Comment 333

butterfly a/b, add bias, then shift by "s" and pack
define dct_bfly32o(out0, out1, a,b,bias,s) \

## Comment 334

8-bit interleave step (for transposes)
define dct_interleave8(a, b) \

## Comment 335

16-bit interleave step (for transposes)
define dct_interleave16(a, b) \

## Comment 336

define dct_pass(bias,shift) \

## Comment 337

rounding biases in column/row passes, see stbi__idct_block for explanation.

## Comment 338

load

## Comment 339

column pass

## Comment 340

16bit 8x8 transpose pass 1

## Comment 341

transpose pass 2

## Comment 342

transpose pass 3

## Comment 343

row pass

## Comment 344

pack

## Comment 345

8bit 8x8 transpose pass 1

## Comment 346

transpose pass 2

## Comment 347

transpose pass 3

## Comment 348

store

## Comment 349

undef dct_const
undef dct_rot
undef dct_widen
undef dct_wadd
undef dct_wsub
undef dct_bfly32o
undef dct_interleave8
undef dct_interleave16
undef dct_pass

## Comment 350

endif // STBI_SSE2

## Comment 351

ifdef STBI_NEON

## Comment 352

NEON integer IDCT. should produce bit-identical
results to the generic C version.

## Comment 353

define dct_long_mul(out, inq, coeff) \

## Comment 354

define dct_long_mac(out, acc, inq, coeff) \

## Comment 355

define dct_widen(out, inq) \

## Comment 356

wide add
define dct_wadd(out, a, b) \

## Comment 357

wide sub
define dct_wsub(out, a, b) \

## Comment 358

butterfly a/b, then shift using "shiftop" by "s" and pack
define dct_bfly32o(out0,out1, a,b,shiftop,s) \

## Comment 359

define dct_pass(shiftop, shift) \

## Comment 360

load

## Comment 361

add DC bias

## Comment 362

column pass

## Comment 363

16bit 8x8 transpose

## Comment 364

these three map to a single VTRN.16, VTRN.32, and VSWP, respectively.
whether compilers actually get this is another story, sadly.
define dct_trn16(x, y) { int16x8x2_t t = vtrnq_s16(x, y); x = t.val[0]; y = t.val[1]; }
define dct_trn32(x, y) { int32x4x2_t t = vtrnq_s32(vreinterpretq_s32_s16(x), vreinterpretq_s32_s16(y)); x = vreinterpretq_s16_s32(t.val[0]); y = vreinterpretq_s16_s32(t.val[1]); }
define dct_trn64(x, y) { int16x8_t x0 = x; int16x8_t y0 = y; x = vcombine_s16(vget_low_s16(x0), vget_low_s16(y0)); y = vcombine_s16(vget_high_s16(x0), vget_high_s16(y0)); }

## Comment 365

pass 1

## Comment 366

pass 2

## Comment 367

pass 3

## Comment 368

undef dct_trn16
undef dct_trn32
undef dct_trn64

## Comment 369

row pass
vrshrn_n_s32 only supports shifts up to 16, we need
17. so do a non-rounding shift of 16 first then follow
up with a rounding shift by 1.

## Comment 370

pack and round

## Comment 371

again, these can translate into one instruction, but often don't.
define dct_trn8_8(x, y) { uint8x8x2_t t = vtrn_u8(x, y); x = t.val[0]; y = t.val[1]; }
define dct_trn8_16(x, y) { uint16x4x2_t t = vtrn_u16(vreinterpret_u16_u8(x), vreinterpret_u16_u8(y)); x = vreinterpret_u8_u16(t.val[0]); y = vreinterpret_u8_u16(t.val[1]); }
define dct_trn8_32(x, y) { uint32x2x2_t t = vtrn_u32(vreinterpret_u32_u8(x), vreinterpret_u32_u8(y)); x = vreinterpret_u8_u32(t.val[0]); y = vreinterpret_u8_u32(t.val[1]); }

## Comment 372

sadly can't use interleaved stores here since we only write
8 bytes to each scan line!

## Comment 373

8x8 8-bit transpose pass 1

## Comment 374

pass 2

## Comment 375

pass 3

## Comment 376

store

## Comment 377

undef dct_trn8_8
undef dct_trn8_16
undef dct_trn8_32

## Comment 378

undef dct_long_mul
undef dct_long_mac
undef dct_widen
undef dct_wadd
undef dct_wsub
undef dct_bfly32o
undef dct_pass

## Comment 379

endif // STBI_NEON

## Comment 380

define STBI__MARKER_none  0xff
if there's a pending marker from the entropy stream, return that
otherwise, fetch from the stream and get a marker. if there's no
marker, return 0xff, which is never a valid marker value

## Comment 381

in each scan, we'll have scan_n components, and the order
of the components is specified by order[]
define STBI__RESTART(x)     ((x) >= 0xd0 && (x) <= 0xd7)

## Comment 382

after a restart interval, stbi__jpeg_reset the entropy decoder and
the dc prediction

## Comment 383

no more than 1<<31 MCUs if no restart_interal? that's plenty safe,
since we don't even allow 1<<30 pixels

## Comment 384

non-interleaved data, we just need to process one block at a time,
in trivial scanline order
number of blocks to do just depends on how many actual "pixels" this
component has, independent of interleaved MCU blocking and such

## Comment 385

every data block is an MCU, so countdown the restart interval

## Comment 386

if it's NOT a restart, then just bail, so we get corrupt data
rather than no data

## Comment 387

scan an interleaved mcu... process scan_n components in order

## Comment 388

scan out an mcu's worth of this component; that's just determined
by the basic H and V specified for the component

## Comment 389

after all interleaved components, that's an interleaved MCU,
so now count down the restart interval

## Comment 390

non-interleaved data, we just need to process one block at a time,
in trivial scanline order
number of blocks to do just depends on how many actual "pixels" this
component has, independent of interleaved MCU blocking and such

## Comment 391

every data block is an MCU, so countdown the restart interval

## Comment 392

scan an interleaved mcu... process scan_n components in order

## Comment 393

scan out an mcu's worth of this component; that's just determined
by the basic H and V specified for the component

## Comment 394

after all interleaved components, that's an interleaved MCU,
so now count down the restart interval

## Comment 395

dequantize and idct the data

## Comment 396

check for comment block or APP blocks

## Comment 397

after we see SOS

## Comment 398

check that plane subsampling factors are integer ratios; our resamplers can't deal with fractional ratios
and I've never seen a non-corrupted JPEG file actually use them

## Comment 399

compute interleaved mcu info

## Comment 400

these sizes can't be more than 17 bits

## Comment 401

number of effective pixels (e.g. for non-interleaved MCU)

## Comment 402

to simplify generation, we'll allocate enough memory to decode
the bogus oversized data from using interleaved MCUs and their
big blocks (e.g. a 16x16 iMCU on an image of width 33); we won't
discard the extra data until colorspace conversion

img_mcu_x, img_mcu_y: <=17 bits; comp[i].h and .v are <=4 (checked earlier)
so these muls can't overflow with 32-bit ints (which we require)

## Comment 403

align blocks for idct using mmx/sse

## Comment 404

w2, h2 are multiples of 8 (see above)

## Comment 405

use comparisons since in some cases we handle more than one case (e.g. SOF)
define stbi__DNL(x)         ((x) == 0xdc)
define stbi__SOI(x)         ((x) == 0xd8)
define stbi__EOI(x)         ((x) == 0xd9)
define stbi__SOF(x)         ((x) == 0xc0 || (x) == 0xc1 || (x) == 0xc2)
define stbi__SOS(x)         ((x) == 0xda)

## Comment 406

define stbi__SOF_progressive(x)   ((x) == 0xc2)

## Comment 407

some files have extra padding after their blocks, so ok, we'll scan

## Comment 408

some JPEGs have junk at end, skip over it but if we find what looks
like a valid marker, resume there

## Comment 409

not a stuffed zero or lead-in to another marker, looks
like an actual marker, return it

## Comment 410

stuffed zero has x=0 now which ends the loop, meaning we go
back to regular scan loop.
repeated 0xff keeps trying to read the next byte of the marker.

## Comment 411

decode image to YCbCr format

## Comment 412

if we reach eof without hitting a marker, stbi__get_marker() below will fail and we'll eventually return 0

## Comment 413

static jfif-centered resampling (across block boundaries)

## Comment 414

define stbi__div4(x) ((stbi_uc) ((x) >> 2))

## Comment 415

need to generate two samples vertically for every one in input

## Comment 416

need to generate two samples horizontally for every one in input

## Comment 417

if only one sample, can't do any interpolation

## Comment 418

define stbi__div16(x) ((stbi_uc) ((x) >> 4))

## Comment 419

need to generate 2x2 samples for every one in input

## Comment 420

if defined(STBI_SSE2) || defined(STBI_NEON)

## Comment 421

need to generate 2x2 samples for every one in input

## Comment 422

process groups of 8 pixels for as long as we can.
note we can't handle the last pixel in a row in this loop
because we need to handle the filter boundary conditions.

## Comment 423

if defined(STBI_SSE2)
load and perform the vertical filtering pass
this uses 3*x + y = 4*x + (y - x)

## Comment 424

horizontal filter works the same based on shifted vers of current
row. "prev" is current row shifted right by 1 pixel; we need to
insert the previous pixel value (from t1).
"next" is current row shifted left by 1 pixel, with first pixel
of next block of 8 pixels added in.

## Comment 425

horizontal filter, polyphase implementation since it's convenient:
even pixels = 3*cur + prev = cur*4 + (prev - cur)
odd  pixels = 3*cur + next = cur*4 + (next - cur)
note the shared term.

## Comment 426

interleave even and odd pixels, then undo scaling.

## Comment 427

pack and write output

## Comment 428

elif defined(STBI_NEON)
load and perform the vertical filtering pass
this uses 3*x + y = 4*x + (y - x)

## Comment 429

horizontal filter works the same based on shifted vers of current
row. "prev" is current row shifted right by 1 pixel; we need to
insert the previous pixel value (from t1).
"next" is current row shifted left by 1 pixel, with first pixel
of next block of 8 pixels added in.

## Comment 430

horizontal filter, polyphase implementation since it's convenient:
even pixels = 3*cur + prev = cur*4 + (prev - cur)
odd  pixels = 3*cur + next = cur*4 + (next - cur)
note the shared term.

## Comment 431

undo scaling and round, then store with even/odd phases interleaved

## Comment 432

endif

## Comment 433

"previous" value for next iter

## Comment 434

endif

## Comment 435

resample with nearest-neighbor

## Comment 436

this is a reduced-precision calculation of YCbCr-to-RGB introduced
to make sure the code produces the same results in both SIMD and scalar
define stbi__float2fixed(x)  (((int) ((x) * 4096.0f + 0.5f)) << 8)

## Comment 437

if defined(STBI_SSE2) || defined(STBI_NEON)

## Comment 438

ifdef STBI_SSE2
step == 3 is pretty ugly on the final interleave, and i'm not convinced
it's useful in practice (you wouldn't use it for textures, for example).
so just accelerate step == 4 case.

## Comment 439

this is a fairly straightforward implementation and not super-optimized.

## Comment 440

load

## Comment 441

unpack to short (and left-shift cr, cb by 8)

## Comment 442

color transform

## Comment 443

descale

## Comment 444

back to byte, set up for transpose

## Comment 445

transpose to interleave channels

## Comment 446

store

## Comment 447

endif

## Comment 448

ifdef STBI_NEON
in this version, step=3 support would be easy to add. but is there demand?

## Comment 449

this is a fairly straightforward implementation and not super-optimized.

## Comment 450

load

## Comment 451

expand to s16

## Comment 452

color transform

## Comment 453

undo scaling, round, convert to byte

## Comment 454

store, interleaving r/g/b/a

## Comment 455

endif

## Comment 456

endif

## Comment 457

set up the kernels

## Comment 458

ifdef STBI_SSE2

## Comment 459

endif

## Comment 460

ifdef STBI_NEON

## Comment 461

endif

## Comment 462

clean up the temporary component buffers

## Comment 463

fast 0..255 * 0..255 => 0..255 rounded multiplication

## Comment 464

validate req_comp

## Comment 465

load a jpeg image from whichever source, but leave in YCbCr format

## Comment 466

determine actual number of components to generate

## Comment 467

nothing to do if no components requested; check this now to avoid
accessing uninitialized coutput[0] later

## Comment 468

resample and color-convert

## Comment 469

allocate line buffer big enough for upsampling off the edges
with upsample factor of 4

## Comment 470

can't error after this so, this is safe

## Comment 471

now go ahead and resample

## Comment 472

endif

## Comment 473

public domain zlib decode    v0.2  Sean Barrett 2006-11-18
   simple implementation
     - all input must be provided in an upfront buffer
     - all output is written to a single output buffer (can malloc/realloc)
   performance
     - fast huffman

## Comment 474

ifndef STBI_NO_ZLIB

## Comment 475

fast-way is faster to check than jpeg huffman, but slow way is slower
define STBI__ZFAST_BITS  9 // accelerate all cases in default tables
define STBI__ZFAST_MASK  ((1 << STBI__ZFAST_BITS) - 1)
define STBI__ZNSYMS 288 // number of symbols in literal/length alphabet

## Comment 476

zlib-style huffman encoding
(jpegs packs from left, zlib from right, so can't share code)

## Comment 477

to bit reverse n bits, reverse 16 and shift
e.g. 11 bits, bit reverse and shift away 5

## Comment 478

DEFLATE spec for generating codes

## Comment 479

zlib-from-memory implementation for PNG reading
   because PNG allows splitting the zlib stream arbitrarily,
   and it's annoying structurally to have PNG call ZLIB call PNG,
   we require PNG read all the IDATs and combine them into a single
   memory buffer

## Comment 480

not resolved by fast table, so compute it the slow way
use jpeg approach, which requires MSbits at top

## Comment 481

code size is s, so:

## Comment 482

This is the first time we hit eof, insert 16 extra padding btis
to allow us to keep going; if we actually consume any of them
though, that is invalid data. This is caught later.

## Comment 483

We already inserted our extra 16 padding bits and are again
out, this stream is actually prematurely terminated.

## Comment 484

The first time we hit zeof, we inserted 16 extra zero bits into our bit
buffer so the decoder can just do its speculative decoding. But if we
actually consumed any of those bits (which is the case when num_bits < 16),
the stream actually read past the end so it is malformed.

## Comment 485

drain the bit-packed data into header

## Comment 486

now fill header the normal way

## Comment 487

window = 1 << (8 + cinfo)... but who cares, we fully buffer output

## Comment 488

use fixed code lengths

## Comment 489

endif

## Comment 490

public domain "baseline" PNG decoder   v0.10  Sean Barrett 2006-11-18
   simple implementation
     - only 8-bit samples
     - no CRC checking
     - allocates lots of intermediate memory
       - avoids problem of streaming data between subsystems
       - avoids explicit window management
   performance
     - uses stb_zlib, a PD zlib implementation with fast huffman decoding

## Comment 491

ifndef STBI_NO_PNG

## Comment 492

synthetic filter used for first scanline to avoid needing a dummy row of 0s

## Comment 493

This formulation looks very different from the reference in the PNG spec, but is
actually equivalent and has favorable data dependencies and admits straightforward
generation of branch-free code, which helps performance significantly.

## Comment 494

adds an extra all-255 alpha channel
dest == src is legal
img_n must be 1 or 3

## Comment 495

must process data backwards since we allow dest==src

## Comment 496

create the png data from post-deflated data

## Comment 497

note: error exits here don't need to clean up a->out individually,
stbi__do_png always does on error.

## Comment 498

we used to check for exact match between raw_len and img_len on non-interlaced PNGs,
but issue #276 reported a PNG in the wild that had extra data at the end (all zeros),
so just check for raw_len < img_len always.

## Comment 499

Allocate two scan lines worth of filter workspace buffer.

## Comment 500

Filtering for low-bit-depth images

## Comment 501

cur/prior filter buffers alternate

## Comment 502

check filter type

## Comment 503

if first row, use special filter that doesn't sample previous row

## Comment 504

perform actual filtering

## Comment 505

expand decoded bits in cur to dest, also adding an extra alpha channel if desired

## Comment 506

expand bits to bytes first

## Comment 507

insert alpha=255 values if desired

## Comment 508

convert the image data from big-endian to platform-native

## Comment 509

de-interlacing

## Comment 510

pass1_x[4] = 0, pass1_x[5] = 1, pass1_x[12] = 1

## Comment 511

compute color-based transparency, assuming we've
already got 255 as the alpha value in the output

## Comment 512

compute color-based transparency, assuming we've
already got 65535 as the alpha value in the output

## Comment 513

between here and free(out) below, exitting would leak

## Comment 514

ifndef STBI_THREAD_LOCAL
define stbi__unpremultiply_on_load  stbi__unpremultiply_on_load_global
define stbi__de_iphone_flag  stbi__de_iphone_flag_global
else

## Comment 515

define stbi__unpremultiply_on_load  (stbi__unpremultiply_on_load_set           \

## Comment 516

define stbi__de_iphone_flag  (stbi__de_iphone_flag_set                         \

## Comment 517

endif // STBI_THREAD_LOCAL

## Comment 518

convert bgr to rgb and unpremultiply

## Comment 519

convert bgr to rgb

## Comment 520

define STBI__PNG_TYPE(a,b,c,d)  (((unsigned) (a) << 24) + ((unsigned) (b) << 16) + ((unsigned) (c) << 8) + (unsigned) (d))

## Comment 521

if paletted, then pal_n is our final components, and
img_n is # components to decompress/filter.

## Comment 522

even with SCAN_header, have to scan to see if we have a tRNS

## Comment 523

non-paletted with tRNS = constant alpha. if header-scanning, we can stop now.

## Comment 524

header scan definitely stops at first IDAT

## Comment 525

initial guess for decoded data size to avoid unnecessary reallocs

## Comment 526

pal_img_n == 3 or 4

## Comment 527

non-paletted image with tRNS -> source image has (constant) alpha

## Comment 528

end of PNG chunk, read and skip CRC

## Comment 529

if critical, fail

## Comment 530

ifndef STBI_NO_FAILURE_STRINGS
not threadsafe

## Comment 531

endif

## Comment 532

end of PNG chunk, read and skip CRC

## Comment 533

endif

## Comment 534

Microsoft/Windows BMP image

## Comment 535

ifndef STBI_NO_BMP

## Comment 536

returns 0..31 for the highest set bit

## Comment 537

extract an arbitrarily-aligned N-bit value (N=bits)
from v, and then make it 8-bits long and fractionally
extend it to full full range.

## Comment 538

BI_BITFIELDS specifies masks explicitly, don't override

## Comment 539

otherwise, use defaults, which is all-0

## Comment 540

not documented, but generated by photoshop and handled by mspaint

## Comment 541

?!?!?

## Comment 542

V4/V5 header

## Comment 543

accept some number of extra bytes after the header, but if the offset points either to before
the header ends or implies a large amount of extra data, reject the file as malformed

## Comment 544

we established that bytes_read_so_far is positive and sensible.
the first half of this test rejects offsets that are either too small positives, or
negative, and guarantees that info.offset >= bytes_read_so_far > 0. this in turn
ensures the number computed in the second half of the test can't overflow.

## Comment 545

sanity-check size

## Comment 546

right shift amt to put high bit in position #7

## Comment 547

if alpha channel is all 0s, replace with all 255s

## Comment 548

endif

## Comment 549

Targa Truevision - TGA
by Jonathan Dummer
ifndef STBI_NO_TGA
returns STBI_rgb or whatever, 0 on error

## Comment 550

only RGB or RGBA (incl. 16bit) or grey allowed

## Comment 551

fallthrough

## Comment 552

when using a colormap, tga_bits_per_pixel is the size of the indexes
I don't think anything but 8 or 16bit indexes makes sense

## Comment 553

read 16bit value and convert to 24bit RGB

## Comment 554

we have 3 channels with 5bits each

## Comment 555

Note that this saves the data in RGB(A) order, so it doesn't need to be swapped later

## Comment 556

some people claim that the most significant bit might be used for alpha
(possibly if an alpha-bit is set in the "image descriptor byte")
but that only made 16bit test images completely translucent..
so let's treat all 15 and 16bit TGAs as RGB with no alpha.

## Comment 557

read in the TGA header stuff

## Comment 558

int tga_alpha_bits = tga_inverted & 15; // the 4 lowest bits - unused (useless?)
  image data

## Comment 559

do a tiny bit of precessing

## Comment 560

If I'm paletted, then I'll use the number of bits from the palette

## Comment 561

tga info

## Comment 562

skip to the data's starting position (offset usually = 0)

## Comment 563

do I need to load a palette?

## Comment 564

any data to skip? (offset usually = 0)

## Comment 565

load the palette

## Comment 566

load the data

## Comment 567

if I'm in RLE mode, do I need to get a RLE stbi__pngchunk?

## Comment 568

yep, get the next byte as a RLE command

## Comment 569

OK, if I need to read a pixel, do it now

## Comment 570

load however much data we did have

## Comment 571

read in index, then perform the lookup

## Comment 572

invalid index

## Comment 573

read in the data raw

## Comment 574

clear the reading flag for the next pixel

## Comment 575

copy data

## Comment 576

in case we're in RLE mode, keep counting down
RLE_count;

## Comment 577

do I need to invert the image?

## Comment 578

clear my palette, if I had one

## Comment 579

swap RGB - if the source data was RGB16, it already is in the right order

## Comment 580

convert to target component count

## Comment 581

the things I do to get rid of an error message, and yet keep
  Microsoft's C compilers happy... [8^(

## Comment 582

OK, done

## Comment 583

endif

## Comment 584

*************************************************************************************************
Photoshop PSD loader -- PD by Thatcher Ulrich, integration by Nicolas Schulz, tweaked by STB

## Comment 585

ifndef STBI_NO_PSD

## Comment 586

No-op.

## Comment 587

Copy next len+1 bytes literally.

## Comment 588

Next -len+1 bytes in the dest are replicated from next source byte.
(Interpret len as a negative 8-bit int.)

## Comment 589

Check identifier

## Comment 590

Check file type version.

## Comment 591

Skip 6 reserved bytes.

## Comment 592

Read the number of channels (R, G, B, A, etc).

## Comment 593

Read the rows and columns of the image.

## Comment 594

Make sure the depth is 8 bits.

## Comment 595

Make sure the color mode is RGB.
Valid options are:
  0: Bitmap
  1: Grayscale
  2: Indexed color
  3: RGB color
  4: CMYK color
  7: Multichannel
  8: Duotone
  9: Lab color

## Comment 596

Skip the Mode Data.  (It's the palette for indexed color; other info for other modes.)

## Comment 597

Skip the image resources.  (resolution, pen tool paths, etc)

## Comment 598

Skip the reserved data.

## Comment 599

Find out if the data is compressed.
Known values:
  0: no compression
  1: RLE compressed

## Comment 600

Check size

## Comment 601

Create the destination image.

## Comment 602

Initialize the data to zero.
memset( out, 0, pixelCount * 4 );

## Comment 603

Finally, the image data.

## Comment 604

RLE as used by .PSD and .TIFF
Loop until you get the number of unpacked bytes you are expecting:
    Read the next source byte into n.
    If n is between 0 and 127 inclusive, copy the next n+1 bytes literally.
    Else if n is between -127 and -1 inclusive, copy the next byte -n+1 times.
    Else if n is 128, noop.
Endloop

## Comment 605

The RLE-compressed data is preceded by a 2-byte data count for each row in the data,
which we're going to just skip.

## Comment 606

Read the RLE data by channel.

## Comment 607

Fill this channel with default data.

## Comment 608

Read the RLE data.

## Comment 609

We're at the raw image data.  It's each channel in order (Red, Green, Blue, Alpha, ...)
where each channel consists of an 8-bit (or 16-bit) value for each pixel in the image.

## Comment 610

Read the data by channel.

## Comment 611

Fill this channel with default data.

## Comment 612

remove weird white matte from PSD

## Comment 613

convert to desired output format

## Comment 614

endif

## Comment 615

*************************************************************************************************
Softimage PIC loader
by Tom Seddon

See http://softimage.wiki.softimage.com/index.php/INFO:_PIC_file_format
See http://ozviz.wasp.uwa.edu.au/~pbourke/dataformats/softimagepic/

## Comment 616

ifndef STBI_NO_PIC

## Comment 617

this will (should...) cater for even some bizarre stuff like having data
for the same channel in multiple packets.

## Comment 618

intermediate buffer is RGBA

## Comment 619

endif

## Comment 620

*************************************************************************************************
GIF loader -- public domain by Jean-Marc Lienher -- simplified/shrunk by stb

## Comment 621

ifndef STBI_NO_GIF

## Comment 622

recurse to decode the prefixes, since the linked-list is backwards,
and working backwards through an interleaved image would be nasty

## Comment 623

g->parse;

## Comment 624

support no starting clear code

## Comment 625

len;

## Comment 626

@OPTIMIZE: is there some way we can accelerate the non-clear path?

## Comment 627

this function is designed to support animated gifs, although stb_image doesn't support it
two back is the image from two frames ago, used for a very specific disposal format

## Comment 628

on first frame, any non-written pixels get the background colour (non-transparent)

## Comment 629

image is treated as "transparent" at the start - ie, nothing overwrites the current background;
background colour is only used for pixels that are not rendered first frame, after that "background"
color refers to the color that was there the previous frame.

## Comment 630

second frame - how do we dispose of the previous one?

## Comment 631

restore what was changed last frame to background before that frame;

## Comment 632

This is a non-disposal case eithe way, so just
leave the pixels as is, and they will become the new background
1: do not dispose
0:  not specified.

## Comment 633

background is what out is after the undoing of the previou frame;

## Comment 634

clear my history;

## Comment 635

if the width of the specified rectangle is 0, that means
we may not see *any* pixels or the image is malformed;
to make sure this is caught, move the current y down to
max_y (which is what out_gif_code checks).

## Comment 636

if this was the first frame,

## Comment 637

if first frame, any pixel not drawn to gets the background color

## Comment 638

unset old transparent

## Comment 639

don't need transparent

## Comment 640

free temp buffer;

## Comment 641

do the final conversion after loading everything;

## Comment 642

moved conversion to after successful load so that the same
can be done for multiple frames.

## Comment 643

if there was an error and we allocated an image buffer, free it!

## Comment 644

free buffers needed for multiple frame loading;

## Comment 645

endif

## Comment 646

*************************************************************************************************
Radiance RGBE HDR loader
originally by Nicolas Schulz
ifndef STBI_NO_HDR

## Comment 647

define STBI__HDR_BUFLEN  1024

## Comment 648

flush to end of line

## Comment 649

Exponent

## Comment 650

Check identifier

## Comment 651

Parse header

## Comment 652

Parse width and height
can't use sscanf() if we're not using stdio!

## Comment 653

Read data

## Comment 654

Load image data
image data is stored as some number of sca

## Comment 655

Read flat data

## Comment 656

Read RLE-encoded data

## Comment 657

not run-length encoded, so we have to actually use THIS data as a decoded
pixel (note this can't be a valid pixel--one of RGB must be >= 128)

## Comment 658

Run

## Comment 659

Dump

## Comment 660

endif // STBI_NO_HDR

## Comment 661

ifndef STBI_NO_BMP

## Comment 662

endif

## Comment 663

ifndef STBI_NO_PSD

## Comment 664

endif

## Comment 665

ifndef STBI_NO_PIC

## Comment 666

endif

## Comment 667

*************************************************************************************************
Portable Gray Map and Portable Pixel Map loader
by Ken Miller

PGM: http://netpbm.sourceforge.net/doc/pgm.html
PPM: http://netpbm.sourceforge.net/doc/ppm.html

Known limitations:
   Does not support comments in the header section
   Does not support ASCII image data (formats P2 and P3)

## Comment 668

ifndef STBI_NO_PNM

## Comment 669

Get identifier

## Comment 670

endif

## Comment 671

ifndef STBI_NO_JPEG

## Comment 672

endif

## Comment 673

ifndef STBI_NO_PNG

## Comment 674

endif

## Comment 675

ifndef STBI_NO_GIF

## Comment 676

endif

## Comment 677

ifndef STBI_NO_BMP

## Comment 678

endif

## Comment 679

ifndef STBI_NO_PSD

## Comment 680

endif

## Comment 681

ifndef STBI_NO_PIC

## Comment 682

endif

## Comment 683

ifndef STBI_NO_PNM

## Comment 684

endif

## Comment 685

ifndef STBI_NO_HDR

## Comment 686

endif

## Comment 687

test tga last because it's a crappy test!
ifndef STBI_NO_TGA

## Comment 688

endif

## Comment 689

ifndef STBI_NO_PNG

## Comment 690

endif

## Comment 691

ifndef STBI_NO_PSD

## Comment 692

endif

## Comment 693

ifndef STBI_NO_PNM

## Comment 694

endif

## Comment 695

ifndef STBI_NO_STDIO

## Comment 696

endif // !STBI_NO_STDIO

## Comment 697

endif // STB_IMAGE_IMPLEMENTATION

## Comment 698

ifdef unused functions

## Comment 699

----------------------------------------------------------------------------

## Comment 700

----------------------------------------------------------------------------

## Comment 701

----------------------------------------------------------------------------

## Comment 702

----------------------------------------------------------------------------
