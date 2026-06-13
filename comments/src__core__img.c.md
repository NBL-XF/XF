# Comments extracted from `src/core/img.c`

Source: `src/core/img.c`

## Comment 1

steals ownership

## Comment 2

Canonical v1 contract:
- always expose RGB tuples
- data is arr<tuple<num,num,num>>
- channels reports 3

gray gets expanded to (v,v,v)
rgba drops alpha

## Comment 3

v1: write PNG regardless of extension.
Easy to extend later to jpg/bmp/tga by sniffing path suffix.

## Comment 4

include "internal.h"

## Comment 5

define STB_IMAGE_IMPLEMENTATION
include "./stb/stb_image.h"

## Comment 6

define STB_IMAGE_WRITE_IMPLEMENTATION
include "./stb/stb_image_write.h"
