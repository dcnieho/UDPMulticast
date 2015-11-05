#pragma once
#include <cstdint>

constexpr uint64_t str2int(const char* str, size_t h = 0)
{
	return !str[h] ? 5381ULL : (str2int(str, h + 1) * 33ULL) ^ str[h];
}