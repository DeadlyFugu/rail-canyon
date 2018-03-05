// Aliases for common types

#pragma once

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef uintptr_t uptr;
typedef intptr_t iptr;

// Align to 4 byte offset
inline u32 align32(u32 offs) {
	u32 rem = offs % 4;
	return rem ? offs + 4 - rem : offs;
}

// Align to 8 byte offset
inline u32 align64(u32 offs) {
	u32 rem = offs % 8;
	return rem ? offs + 8 - rem : offs;
}

// Align to 16 byte offset
inline u32 align128(u32 offs) {
	u32 rem = offs % 16;
	return rem ? offs + 16 - rem : offs;
}

// Align to 16 byte offset
inline u64 align128(u64 offs) {
	u64 rem = offs % 16;
	return rem ? offs + 16 - rem : offs;
}

// 16-bit generic type
union general16 {
	struct {
		u8 lo8;
		u8 hi8;
	};
	u16 u16;
	i16 i16;
};

// 32-bit generic type
union general32 {
	struct {
		general16 lo16;
		general16 hi16;
	};
	u32 u32;
	i32 i32;
	float f32;
};

// 64-bit generic type
union general64 {
	struct {
		general32 lo32;
		general32 hi32;
	};
	u64 u64;
	i64 i64;
	double f64;
};

static_assert(sizeof(general16) == 2, "general16 must be 2 bytes");
static_assert(sizeof(general32) == 4, "general32 must be 4 bytes");
static_assert(sizeof(general64) == 8, "general64 must be 8 bytes");
