# Comments extracted from `src/core/img.c`

Version: `v1.0.3`

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

include "internal.h"

## Comment 4

define STB_IMAGE_IMPLEMENTATION
include "./stb/stb_image.h"

## Comment 5

define STB_IMAGE_WRITE_IMPLEMENTATION
include "./stb/stb_image_write.h"
