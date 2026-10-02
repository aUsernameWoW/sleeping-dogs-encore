#pragma once

// Reading game memory whose layout comes from the disassembly: a wrong guess must log garbage, not take the game
// down (it did once: a Skookum List's items are behind a pointer). VirtualQuery rather than __try, so the crash
// handler doesn't report a caught fault as a crash.

#include <Windows.h>

#include <cstdint>
#include <cstring>

namespace mem
{
	// Whether [p, p + n) is committed, readable memory.
	inline bool Readable(const void* p, size_t n)
	{
		const auto address = reinterpret_cast<uintptr_t>(p);
		if (address < 0x10000 || address + n < address) {
			return false;
		}
		MEMORY_BASIC_INFORMATION info;
		constexpr DWORD kReadable = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
			PAGE_EXECUTE_WRITECOPY;
		if (!VirtualQuery(p, &info, sizeof(info)) || info.State != MEM_COMMIT || !(info.Protect & kReadable) ||
			(info.Protect & PAGE_GUARD)) {
			return false;
		}
		return address + n <= reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
	}

	// The T at p + offset, or T{} if it isn't readable.
	template <typename T>
	T Read(const void* p, size_t offset)
	{
		T value{};
		const uint8_t* at = static_cast<const uint8_t*>(p) + offset;
		if (p && Readable(at, sizeof(value))) {
			std::memcpy(&value, at, sizeof(value));
		}
		return value;
	}
}
