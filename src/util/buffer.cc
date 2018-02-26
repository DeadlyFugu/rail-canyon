#include "common.hh"
#include "stdlib.h"
#include "string.h"
#include "util/buffer.hh"

namespace rc {
namespace util {

Buffer::Buffer(unsigned len, bool zeroed) : stretchy(false), owned(true) {
	if (zeroed) {
		base = (u8*) calloc(1, len);
	} else {
		base = (u8*) malloc(len);
	}

	head = base;
	end = base + len;
}

Buffer::Buffer(void* src, unsigned len, bool owned) : stretchy(false), owned(owned) {
	base = (u8*) src;
	head = base;
	end = base + len;
}

Buffer::~Buffer() {
	if (owned) {
		free(base);
	}
}

Buffer Buffer::view() {
	return Buffer(base, size(), false);
}

Buffer Buffer::view(unsigned start, unsigned len) {
	assert(start + len <= size(), "view out of bounds");
	return Buffer(base + start, len, false);
}

Buffer Buffer::copy() {
	auto len = size();
	auto new_base = malloc(len);
	memcpy(new_base, base, len);
	return Buffer(new_base, len, true);
}

Buffer Buffer::copy(unsigned start, unsigned len) {
	assert(start + len < size(), "copy out of bounds");
	auto new_base = malloc(len);
	memcpy(new_base, base + start, len);
	return Buffer(new_base, len, true);
}

void Buffer::seek(unsigned pos) {
	head = base + pos;
}

unsigned Buffer::tell() {
	return (unsigned) (head - base);
}

void* Buffer::base_ptr() {
	return base;
}

void* Buffer::head_ptr() {
	return head;
}

unsigned Buffer::size() {
	return (unsigned) (end - base);
}

bool Buffer::atEnd() {
	return head >= end;
}

void Buffer::read(void* dst, unsigned len) {
	assert(head + len <= end, "read out of bounds");
	memcpy(dst, head, len);
	head += len;
}

void Buffer::write(const void* src, unsigned len) {
	if (head + len > end) {
		if (stretchy) {
			resize(head - base + len);
		} else {
			assert(false, "write out of bounds");
		}
	}
	memcpy(head, src, len);
	head += len;
}

std::string Buffer::read_u16string(bool align) {
	u16 len;
	read(&len);

	char* buffer = (char*) malloc(len + 1);
	read(buffer, len);
	buffer[len] = 0;

	std::string s(buffer);
	free(buffer);

	if (align) align16();

	return s;
}

void Buffer::write_u16string(const std::string& value, bool align) {
	u16 len = value.size();
	write(len);

	write(&value[0], len);

	if (align) align16();
}

void Buffer::fill(u8 c) {
	for (u8* p = base; p < end; p++) {
		*p = c;
	}
}

void Buffer::resize(unsigned len) {
	assert(stretchy, "cannot resize non-stretchy buffer");
	auto head_offs = tell();
	auto end_offs = size();
	base = (u8*) realloc(base, len);
	head = base + head_offs;
	end = base + len;
}

void Buffer::setStretchy(bool enable) {
	stretchy = enable;
}

bool Buffer::isStretchy() {
	return stretchy;
}

void Buffer::dump() {
	log_info("buffer %s (%i bytes):", "<todo: labels>", size());
	auto start = base;
	auto p = base;
	char hex_row[] = "........ ........ ........ ........";
	static const char hex_char[] = "0123456789abcdef";
	char ascii_row[] = "................";
	while (p < end) {
		auto local_offs = p - start;
		auto char_offs = 0;
		for (int i = 0; i < 16; i++) {
			u8 value = u8(p < end ? *p : 0);
			hex_row[char_offs] = hex_char[value >> 4];
			hex_row[char_offs+1] = hex_char[value & 0x0f];
			char_offs += (i % 4 == 3 ? 3 : 2);
			ascii_row[i] = isprint(value) ? value : '.';
			p++;
		}
		log_info("[0x%08x] %s  %s", local_offs, hex_row, ascii_row);
	}
}

}
}