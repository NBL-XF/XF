# Comments extracted from `src/core/stb/stb_image_write.h`

Source: `src/core/stb/stb_image_write.h`

## Comment 1

stb_image_write - v1.16 - public domain - http://nothings.org/stb
writes out PNG/BMP/TGA/JPEG/HDR images to C stdio - Sean Barrett 2010-2015
no warranty implied; use at your own risk

Before #including,

#define STB_IMAGE_WRITE_IMPLEMENTATION

in the file that you want to have the implementation.

Will probably not work correctly with strict-aliasing optimizations.

ABOUT:

This header file is a library for writing images to C stdio or a callback.

The PNG output is not optimal; it is 20-50% larger than the file
written by a decent optimizing implementation; though providing a custom
zlib compress function (see STBIW_ZLIB_COMPRESS) can mitigate that.
This library is designed for source code compactness and simplicity,
not optimal image file size or run-time performance.

BUILDING:

You can #define STBIW_ASSERT(x) before the #include to avoid using assert.h.
You can #define STBIW_MALLOC(), STBIW_REALLOC(), and STBIW_FREE() to replace
malloc,realloc,free.
You can #define STBIW_MEMMOVE() to replace memmove()
You can #define STBIW_ZLIB_COMPRESS to use a custom zlib-style compress function
for PNG compression (instead of the builtin one), it must have the following signature:
unsigned char * my_compress(unsigned char *data, int data_len, int *out_len, int quality);
The returned data will be freed with STBIW_FREE() (free() by default),
so it must be heap allocated with STBIW_MALLOC() (malloc() by default),

UNICODE:

If compiling for Windows and you wish to use Unicode filenames, compile
with
#define STBIW_WINDOWS_UTF8
and pass utf8-encoded filenames. Call stbiw_convert_wchar_to_utf8 to convert
Windows wchar_t filenames to utf8.

USAGE:

There are five functions, one for each image file format:

int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);
int stbi_write_bmp(char const *filename, int w, int h, int comp, const void *data);
int stbi_write_tga(char const *filename, int w, int h, int comp, const void *data);
int stbi_write_jpg(char const *filename, int w, int h, int comp, const void *data, int quality);
int stbi_write_hdr(char const *filename, int w, int h, int comp, const float *data);

void stbi_flip_vertically_on_write(int flag); // flag is non-zero to flip data vertically

There are also five equivalent functions that use an arbitrary write function. You are
expected to open/close your file-equivalent before and after calling these:

int stbi_write_png_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data, int stride_in_bytes);
int stbi_write_bmp_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data);
int stbi_write_tga_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data);
int stbi_write_hdr_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const float *data);
int stbi_write_jpg_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data, int quality);

where the callback is:
void stbi_write_func(void *context, void *data, int size);

You can configure it with these global variables:
int stbi_write_tga_with_rle;             // defaults to true; set to 0 to disable RLE
int stbi_write_png_compression_level;    // defaults to 8; set to higher for more compression
int stbi_write_force_png_filter;         // defaults to -1; set to 0..5 to force a filter mode


You can define STBI_WRITE_NO_STDIO to disable the file variant of these
functions, so the library will not use stdio.h at all. However, this will
also disable HDR writing, because it requires stdio for formatted output.

Each function returns 0 on failure and non-0 on success.

The functions create an image file defined by the parameters. The image
is a rectangle of pixels stored from left-to-right, top-to-bottom.
Each pixel contains 'comp' channels of data stored interleaved with 8-bits
per channel, in the following order: 1=Y, 2=YA, 3=RGB, 4=RGBA. (Y is
monochrome color.) The rectangle is 'w' pixels wide and 'h' pixels tall.
The *data pointer points to the first byte of the top-left-most pixel.
For PNG, "stride_in_bytes" is the distance in bytes from the first byte of
a row of pixels to the first byte of the next row of pixels.

PNG creates output files with the same number of components as the input.
The BMP format expands Y to RGB in the file format and does not
output alpha.

PNG supports writing rectangles of data even when the bytes storing rows of
data are not consecutive in memory (e.g. sub-rectangles of a larger image),
by supplying the stride between the beginning of adjacent rows. The other
formats do not. (Thus you cannot write a native-format BMP through the BMP
writer, both because it is in BGR order and because it may have padding
at the end of the line.)

PNG allows you to set the deflate compression level by setting the global
variable 'stbi_write_png_compression_level' (it defaults to 8).

HDR expects linear float data. Since the format is always 32-bit rgb(e)
data, alpha (if provided) is discarded, and for monochrome data it is
replicated across all three channels.

TGA supports RLE or non-RLE compressed data. To use non-RLE-compressed
data, set the global variable 'stbi_write_tga_with_rle' to 0.

JPEG does ignore alpha channels in input data; quality is between 1 and 100.
Higher quality looks better but results in a bigger image.
JPEG baseline (no JPEG progressive).

CREDITS:


Sean Barrett           -    PNG/BMP/TGA
Baldur Karlsson        -    HDR
Jean-Sebastien Guay    -    TGA monochrome
Tim Kelsey             -    misc enhancements
Alan Hickman           -    TGA RLE
Emmanuel Julien        -    initial file IO callback implementation
Jon Olick              -    original jo_jpeg.cpp code
Daniel Gibson          -    integrate JPEG, allow external zlib
Aarni Koskela          -    allow choosing PNG filter

bugfixes:
github:Chribba
Guillaume Chereau
github:jry2
github:romigrou
Sergio Gonzalez
Jonas Karlsson
Filip Wasil
Thatcher Ulrich
github:poppolopoppo
Patrick Boettcher
github:xeekworx
Cap Petschulat
Simon Rodriguez
Ivan Tikhonov
github:ignotion
Adam Schackart
Andrew Kensler

LICENSE

See end of file for license information.

## Comment 2

UTF8

## Comment 3

UTF8

## Comment 4

UTF8

## Comment 5

FALLTHROUGH

## Comment 6

skip RLE for images too small or large

## Comment 7

fallthrough

## Comment 8

encode into scratch buffer

## Comment 9

fallthrough

## Comment 10

RLE each component separately

## Comment 11

**************************************************************************

JPEG writer

This is based on Jon Olick's jo_jpeg.cpp:
public domain Simple, Minimalistic JPEG writer - http://www.jonolick.com/code.html

## Comment 12

Revision history
1.16  (2021-07-11)
make Deflate code emit uncompressed blocks when it would otherwise expand
support writing BMPs with alpha channel
1.15  (2020-07-13) unknown
1.14  (2020-02-02) updated JPEG writer to downsample chroma channels
1.13
1.12
1.11  (2019-08-11)

1.10  (2019-02-07)
support utf8 filenames in Windows; fix warnings and platform ifdefs
1.09  (2018-02-11)
fix typo in zlib quality API, improve STB_I_W_STATIC in C++
1.08  (2018-01-29)
add stbi__flip_vertically_on_write, external zlib, zlib quality, choose PNG filter
1.07  (2017-07-24)
doc fix
1.06 (2017-07-23)
writing JPEG (using Jon Olick's code)
1.05   ???
1.04 (2017-03-03)
monochrome BMP expansion
1.03   ???
1.02 (2016-04-02)
avoid allocating large structures on the stack
1.01 (2016-01-16)
STBIW_REALLOC_SIZED: support allocators with no realloc support
avoid race-condition in crc initialization
minor compile issues
1.00 (2015-09-14)
installable file IO function
0.99 (2015-09-13)
warning fixes; TGA rle support
0.98 (2015-04-08)
added STBIW_MALLOC, STBIW_ASSERT etc
0.97 (2015-01-18)
fixed HDR asserts, rewrote HDR rle logic
0.96 (2015-01-17)
add HDR output
fix monochrome BMP
0.95 (2014-08-17)
add monochrome TGA output
0.94 (2014-05-31)
rename private functions to avoid conflicts with stb_image.h
0.93 (2014-05-27)
warning fixes
0.92 (2010-08-01)
casts to unsigned char to fix warnings
0.91 (2010-07-17)
first public release
0.90   first internal release

## Comment 13

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

## Comment 14

define STB_IMAGE_WRITE_IMPLEMENTATION

## Comment 15

define STBIW_WINDOWS_UTF8

## Comment 16

ifndef INCLUDE_STB_IMAGE_WRITE_H
define INCLUDE_STB_IMAGE_WRITE_H

## Comment 17

include <stdlib.h>

## Comment 18

if STB_IMAGE_WRITE_STATIC causes problems, try defining STBIWDEF to 'inline' or 'static inline'
ifndef STBIWDEF
ifdef STB_IMAGE_WRITE_STATIC
define STBIWDEF  static
else
ifdef __cplusplus
define STBIWDEF  extern "C"
else
define STBIWDEF  extern
endif
endif
endif

## Comment 19

ifndef STB_IMAGE_WRITE_STATIC  // C++ forbids static forward declarations

## Comment 20

endif

## Comment 21

ifndef STBI_WRITE_NO_STDIO

## Comment 22

ifdef STBIW_WINDOWS_UTF8

## Comment 23

endif
endif

## Comment 24

endif//INCLUDE_STB_IMAGE_WRITE_H

## Comment 25

ifdef STB_IMAGE_WRITE_IMPLEMENTATION

## Comment 26

ifdef _WIN32
ifndef _CRT_SECURE_NO_WARNINGS
define _CRT_SECURE_NO_WARNINGS
endif
ifndef _CRT_NONSTDC_NO_DEPRECATE
define _CRT_NONSTDC_NO_DEPRECATE
endif
endif

## Comment 27

ifndef STBI_WRITE_NO_STDIO
include <stdio.h>
endif // STBI_WRITE_NO_STDIO

## Comment 28

include <stdarg.h>
include <stdlib.h>
include <string.h>
include <math.h>

## Comment 29

if defined(STBIW_MALLOC) && defined(STBIW_FREE) && (defined(STBIW_REALLOC) || defined(STBIW_REALLOC_SIZED))
ok
elif !defined(STBIW_MALLOC) && !defined(STBIW_FREE) && !defined(STBIW_REALLOC) && !defined(STBIW_REALLOC_SIZED)
ok
else
error "Must define all or none of STBIW_MALLOC, STBIW_FREE, and STBIW_REALLOC (or STBIW_REALLOC_SIZED)."
endif

## Comment 30

ifndef STBIW_MALLOC
define STBIW_MALLOC(sz)        malloc(sz)
define STBIW_REALLOC(p,newsz)  realloc(p,newsz)
define STBIW_FREE(p)           free(p)
endif

## Comment 31

ifndef STBIW_REALLOC_SIZED
define STBIW_REALLOC_SIZED(p,oldsz,newsz) STBIW_REALLOC(p,newsz)
endif

## Comment 32

ifndef STBIW_MEMMOVE
define STBIW_MEMMOVE(a,b,sz) memmove(a,b,sz)
endif

## Comment 33

ifndef STBIW_ASSERT
include <assert.h>
define STBIW_ASSERT(x) assert(x)
endif

## Comment 34

define STBIW_UCHAR(x) (unsigned char) ((x) & 0xff)

## Comment 35

ifdef STB_IMAGE_WRITE_STATIC

## Comment 36

else

## Comment 37

endif

## Comment 38

initialize a callback-based context

## Comment 39

ifndef STBI_WRITE_NO_STDIO

## Comment 40

if defined(_WIN32) && defined(STBIW_WINDOWS_UTF8)
ifdef __cplusplus
define STBIW_EXTERN extern "C"
else
define STBIW_EXTERN extern
endif

## Comment 41

endif

## Comment 42

if defined(_WIN32) && defined(STBIW_WINDOWS_UTF8)

## Comment 43

if defined(_MSC_VER) && _MSC_VER >= 1400

## Comment 44

else

## Comment 45

endif

## Comment 46

elif defined(_MSC_VER) && _MSC_VER >= 1400

## Comment 47

else

## Comment 48

endif

## Comment 49

endif // !STBI_WRITE_NO_STDIO

## Comment 50

composite against pink background

## Comment 51

write RGB bitmap

## Comment 52

RGBA bitmaps need a v4 header
use BI_BITFIELDS mode with 32bpp and alpha mask
(straight BI_RGB with alpha mask doesn't work in most readers)

## Comment 53

ifndef STBI_WRITE_NO_STDIO

## Comment 54

endif //!STBI_WRITE_NO_STDIO

## Comment 55

len;

## Comment 56

ifndef STBI_WRITE_NO_STDIO

## Comment 57

endif

## Comment 58

*************************************************************************************************
Radiance RGBE HDR writer
by Baldur Karlsson

## Comment 59

define stbiw__max(a, b)  ((a) > (b) ? (a) : (b))

## Comment 60

ifndef STBI_WRITE_NO_STDIO

## Comment 61

find first run

## Comment 62

dump up to first run

## Comment 63

if there's a run, output it

## Comment 64

find next byte after run

## Comment 65

output run up to r

## Comment 66

Each component is stored separately. Allocate scratch space for full output scanline.

## Comment 67

ifdef __STDC_LIB_EXT1__

## Comment 68

else

## Comment 69

endif

## Comment 70

endif // STBI_WRITE_NO_STDIO

## Comment 71

////////////////////////////////////////////////////////////////////////////

PNG writer

## Comment 72

ifndef STBIW_ZLIB_COMPRESS
stretchy buffer; stbiw__sbpush() == vector<>::push_back() -- stbiw__sbcount() == vector<>::size()
define stbiw__sbraw(a) ((int *) (void *) (a) - 2)
define stbiw__sbm(a)   stbiw__sbraw(a)[0]
define stbiw__sbn(a)   stbiw__sbraw(a)[1]

## Comment 73

define stbiw__sbneedgrow(a,n)  ((a)==0 || stbiw__sbn(a)+n >= stbiw__sbm(a))
define stbiw__sbmaybegrow(a,n) (stbiw__sbneedgrow(a,(n)) ? stbiw__sbgrow(a,n) : 0)
define stbiw__sbgrow(a,n)  stbiw__sbgrowf((void **) &(a), (n), sizeof(*(a)))

## Comment 74

define stbiw__sbpush(a, v)      (stbiw__sbmaybegrow(a,1), (a)[stbiw__sbn(a)++] = (v))
define stbiw__sbcount(a)        ((a) ? stbiw__sbn(a) : 0)
define stbiw__sbfree(a)         ((a) ? STBIW_FREE(stbiw__sbraw(a)),0 : 0)

## Comment 75

define stbiw__zlib_flush() (out = stbiw__zlib_flushf(out, &bitbuf, &bitcount))
define stbiw__zlib_add(code,codebits) \

## Comment 76

define stbiw__zlib_huffa(b,c)  stbiw__zlib_add(stbiw__zlib_bitrev(b,c),c)
default huffman tables
define stbiw__zlib_huff1(n)  stbiw__zlib_huffa(0x30 + (n), 8)
define stbiw__zlib_huff2(n)  stbiw__zlib_huffa(0x190 + (n)-144, 9)
define stbiw__zlib_huff3(n)  stbiw__zlib_huffa(0 + (n)-256,7)
define stbiw__zlib_huff4(n)  stbiw__zlib_huffa(0xc0 + (n)-280,8)
define stbiw__zlib_huff(n)  ((n) <= 143 ? stbiw__zlib_huff1(n) : (n) <= 255 ? stbiw__zlib_huff2(n) : (n) <= 279 ? stbiw__zlib_huff3(n) : stbiw__zlib_huff4(n))
define stbiw__zlib_huffb(n) ((n) <= 143 ? stbiw__zlib_huff1(n) : stbiw__zlib_huff2(n))

## Comment 77

define stbiw__ZHASH   16384

## Comment 78

endif // STBIW_ZLIB_COMPRESS

## Comment 79

ifdef STBIW_ZLIB_COMPRESS
user provided a zlib compress implementation, use that

## Comment 80

else // use builtin

## Comment 81

hash next 3 bytes of data to be compressed

## Comment 82

when hash table entry is too long, delete half the entries

## Comment 83

"lazy matching" - check match at *next* byte, and if it's better, do cur byte as literal

## Comment 84

write out final bytes

## Comment 85

pad with 0 bits to byte boundary

## Comment 86

store uncompressed instead if compression was worse

## Comment 87

compute adler32 on input

## Comment 88

make returned pointer freeable

## Comment 89

endif // STBIW_ZLIB_COMPRESS

## Comment 90

ifdef STBIW_CRC32

## Comment 91

else

## Comment 92

endif

## Comment 93

define stbiw__wpng4(o,a,b,c,d) ((o)[0]=STBIW_UCHAR(a),(o)[1]=STBIW_UCHAR(b),(o)[2]=STBIW_UCHAR(c),(o)[3]=STBIW_UCHAR(d),(o)+=4)
define stbiw__wp32(data,v) stbiw__wpng4(data, (v)>>24,(v)>>16,(v)>>8,(v));
define stbiw__wptag(data,s) stbiw__wpng4(data, s[0],s[1],s[2],s[3])

## Comment 94

@OPTIMIZE: provide an option that always forces left-predict or paeth predict

## Comment 95

first loop isn't optimized since it's just one pixel

## Comment 96

Estimate the entropy of the line using this filter; the less, the better.

## Comment 97

when we get here, filter_type contains the filter type, and line_buffer contains the data

## Comment 98

each tag requires 12 bytes of overhead

## Comment 99

ifndef STBI_WRITE_NO_STDIO

## Comment 100

endif

## Comment 101

Even part

## Comment 102

Odd part

## Comment 103

The rotator is modified from fig 4-8 to avoid extra negations.

## Comment 104

DCT rows

## Comment 105

DCT columns

## Comment 106

Quantize/descale/zigzag the coefficients

## Comment 107

DU[stbiw__jpg_ZigZag[j]] = (int)(v < 0 ? ceilf(v - 0.5f) : floorf(v + 0.5f));
ceilf() and floorf() are C99, not C89, but I /think/ they're not needed here anyway?

## Comment 108

Encode DC

## Comment 109

Encode ACs

## Comment 110

end0pos = first element in reverse order !=0

## Comment 111

Constants that don't pollute global namespace

## Comment 112

Huffman tables

## Comment 113

Write Headers

## Comment 114

Encode 8x8 macroblocks

## Comment 115

comp == 2 is grey+alpha (alpha is ignored)

## Comment 116

row >= height => use last input row

## Comment 117

if col >= width => use pixel from last input column

## Comment 118

subsample U,V

## Comment 119

row >= height => use last input row

## Comment 120

if col >= width => use pixel from last input column

## Comment 121

Do the bit alignment of the EOI marker

## Comment 122

EOI

## Comment 123

ifndef STBI_WRITE_NO_STDIO

## Comment 124

endif

## Comment 125

endif // STB_IMAGE_WRITE_IMPLEMENTATION

## Comment 126

----------------------------------------------------------------------------

## Comment 127

----------------------------------------------------------------------------

## Comment 128

----------------------------------------------------------------------------

## Comment 129

----------------------------------------------------------------------------
