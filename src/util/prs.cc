// This implementation based heavily on the following C# implementation:
// https://github.com/FraGag/prs.net/blob/master/FraGag.Compression.Prs/Prs.Impl.cs
// Original implementation by Francis Gagné under the following license:

// Copyright (c) 2012, Francis Gagné
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the <organization> nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.

#include "common.hh"

static u32 getControlBit(u32* bitPos, u8* currentByte, Buffer& in) {
	*bitPos -= 1;
	if (*bitPos == 0) {
		in.read(currentByte);
		*bitPos = 8;
	}

	u32 flag = *currentByte & (u8) 0x01;
	*currentByte >>= 1;
	return flag;
}

Buffer prs_decode(void* compressed, u32 len) {
	Buffer in(compressed, len, false);
	Buffer out(len);
	out.setStretchy(true);

	u32 bitPos = 9;
	u8 currentByte;
	u32 lookBehindOffset, lookBehindLength;
	u8 byte;

	in.read(&currentByte);
	while (true) {
		if (getControlBit(&bitPos, &currentByte, in)) {
			in.read(&byte);
			out.write(byte);
			continue;
		}
		if (getControlBit(&bitPos, &currentByte, in)) {
			in.read(&byte);
			lookBehindOffset = byte;
			in.read(&byte);
			lookBehindOffset |= byte << 8;
			if (!lookBehindOffset) break;

			lookBehindLength = lookBehindOffset & 7;
			lookBehindOffset = (lookBehindOffset >> 3) | -0x2000;

			if (lookBehindLength == 0) {
				in.read(&byte);
				lookBehindLength = byte + 1;
			} else {
				lookBehindLength += 2;
			}
		} else {
			lookBehindLength = 0;
			lookBehindLength = (lookBehindLength << 1) | getControlBit(&bitPos, &currentByte, in);
			lookBehindLength = (lookBehindLength << 1) | getControlBit(&bitPos, &currentByte, in);
			in.read(&byte);
			lookBehindOffset = byte | -0x100;
			lookBehindLength += 2;
		}

		for (int i = 0; i < lookBehindLength; i++) {
			u32 writePosition = out.tell();
			out.seek(writePosition + lookBehindOffset);
			out.read(&byte);
			out.seek(writePosition);
			out.write(byte);
		}
	}

	out.setStretchy(false);
	return std::move(out);
}
