// Managed memory blocks
// todo: use rwstreamlib's buffer objects instead

#pragma once
#include "common.hh"
#include <string>

namespace rc {
namespace util {

class Buffer {
private:
	u8* base;
	u8* head;
	u8* end;
public:
	bool stretchy, owned;

	// create new buffer of len len
	// if zeroed is true, data will be set to zero first
	Buffer(unsigned len, bool zeroed = false);

	// create new buffer from existing data
	// if owned is true, this buffer will manage deleting the data afterwards
	Buffer(void* src, unsigned len, bool owned);

	// deletes owned data
	~Buffer();

	// delete copy constructor (must explicitly copy via copy, view, or move)
	Buffer(const Buffer&) = delete;

	// move constructor
	Buffer(Buffer&& other) {
		memcpy(this, &other, sizeof(Buffer));
		other.base = nullptr;
	}

	// returns a view (doesn't own data) copy of the buffer
	Buffer view();

	// returns a view into part of this buffer
	Buffer view(unsigned start, unsigned len);

	// returns a copy (duplicate data, won't modify original) of the buffer
	Buffer copy();

	// returns a copy of part of this buffer
	Buffer copy(unsigned start, unsigned len);

	// set buffer head to pos
	void seek(unsigned pos);

	// return buffer head pos
	unsigned tell();

	// align buffer head to 2 byte boundary
	inline void align16() {
		if ((head - base) % 2 == 1) head++;
	}

	// align buffer head to 4 byte boundary
	inline void align32() {
		seek(::align32(tell()));
	}

	// align buffer head to 8 byte boundary
	inline void align64() {
		seek(::align64(tell()));
	}

	// align buffer head to 16 byte boundary
	inline void align128() {
		seek(::align128(tell()));
	}

	// return pointer to base
	void* base_ptr();

	// return pointer to head
	void* head_ptr();

	// return size
	unsigned size();

	// test if at end of buffer
	bool atEnd();

	// read len bytes from buffer to dst
	void read(void* dst, unsigned len);

	// write len bytes from src to buffer
	void write(const void* src, unsigned len);

	// read a struct to value
	template<typename T>
	void read(T* value) {
		read(value, sizeof(T));
	}

	// write a struct from value
	template<typename T>
	void write(const T& value) {
		write(&value, sizeof(T));
	}

	// write the contents of another buffer into this one
	inline void write(Buffer& other) {
		write(other.base_ptr(), other.size());
	}

	// read string from buffer in u16string format
	std::string read_u16string(bool align16 = true);

	// write string to buffer in u16string format
	void write_u16string(const std::string& value, bool align16 = true);

	// set all bytes in buffer to a specific value
	void fill(u8 c);

	// resize buffer, only works if stretchy
	void resize(unsigned len);

	// set auto-resize
	void setStretchy(bool enable);

	// test if auto-resize enabled
	bool isStretchy();

	// print contents to log
	void dump();
};

}
}
