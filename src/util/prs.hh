// Decompress PRS format

#pragma once

#include "common.hh"
#include "util/buffer.hh"

Buffer prs_decode(void* compressed, u32 len);
