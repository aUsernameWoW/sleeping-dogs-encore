#pragma once

// Reading game memory whose layout comes from the disassembly: a wrong guess must log garbage, not take the game
// down (it did once: a Skookum List's items are behind a pointer). VirtualQuery rather than __try, so the crash
// handler doesn't report a caught fault as a crash.
//
// VirtualQuery is a system call that takes the process's address-space lock. One per field read made walks of a
// car's materials or a script print's values take 100-1200 ms on the game thread, worst under D3D11On12, whose
// worker maps memory all the time (SDProfiler, 2026-10-29). So large regions seen readable (the game's pools, heap
// segments, module images: long-lived) are remembered for a moment; small ones are asked about every time.
// Identical copy in every mod that reads game memory.

#include <Windows.h>

#include <cstdint>
#include <cstring>

namespace mem
{
	struct Region
	{
		uintptr_t mBegin = 0, mEnd = 0;
		ULONGLONG mUntil = 0;
	};
	constexpr size_t kCachedRegions = 8;
	constexpr size_t kMinCachedSize = 1 << 20;
	constexpr ULONGLONG kCacheMs = 250;

	// Whether [p, p + n) is committed, readable memory.
	inline bool Readable(const void* p, size_t n)
	{
		const auto address = reinterpret_cast<uintptr_t>(p);
		if (address < 0x10000 || address + n < address) {
			return false;
		}
		thread_local Region cache[kCachedRegions];
		thread_local size_t next = 0;
		const ULONGLONG now = GetTickCount64();
		for (const Region& region : cache) {
			if (address >= region.mBegin && address + n <= region.mEnd && now < region.mUntil) {
				return true;
			}
		}
		MEMORY_BASIC_INFORMATION info;
		constexpr DWORD kReadable = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
			PAGE_EXECUTE_WRITECOPY;
		if (!VirtualQuery(p, &info, sizeof(info)) || info.State != MEM_COMMIT || !(info.Protect & kReadable) ||
			(info.Protect & PAGE_GUARD)) {
			return false;
		}
		const uintptr_t end = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
		if (info.RegionSize >= kMinCachedSize) {
			cache[next++ % kCachedRegions] = { reinterpret_cast<uintptr_t>(info.BaseAddress), end, now + kCacheMs };
		}
		return address + n <= end;
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
