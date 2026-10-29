// mem::Readable's region cache: answers must stay right (unreadable, guard and freed memory), a large region is
// asked about once, a small freed one is never trusted from the cache, and a million reads stay fast.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../core/mem.hh"
#include <cstdio>

int main()
{
	int failures = 0;
	auto check = [&](bool ok, const char* what) { if (!ok) { std::printf("FAILED: %s\n", what); ++failures; } };

	check(!mem::Readable(nullptr, 4), "null");
	check(!mem::Readable(reinterpret_cast<void*>(0x1000), 4), "low address");
	const int local = 7;
	check(mem::Read<int>(&local, 0) == 7, "a stack variable");

	// A large region: cached, so a million reads cost a few VirtualQuery calls at most.
	const size_t size = 4 << 20;
	auto* big = static_cast<uint8_t*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	big[size - 8] = 42;
	LARGE_INTEGER f, t0, t1;
	QueryPerformanceFrequency(&f);
	QueryPerformanceCounter(&t0);
	uint64_t sum = 0;
	for (int i = 0; i < 1000000; ++i) sum += mem::Read<uint8_t>(big, (i * 4099ull) % size);
	QueryPerformanceCounter(&t1);
	const double ms = (t1.QuadPart - t0.QuadPart) * 1000.0 / f.QuadPart;
	std::printf("1M reads in a 4 MiB region: %.1f ms (sum %llu)\n", ms, static_cast<unsigned long long>(sum));
	check(ms < 200, "cached reads are fast");
	check(mem::Read<uint8_t>(big, size - 8) == 42, "last bytes of the region");
	check(!mem::Readable(big + size - 4, 8), "a read past the region's end");

	// Guard pages and no-access pages inside a large reservation are never readable, cache or not.
	auto* guarded = static_cast<uint8_t*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	DWORD old;
	VirtualProtect(guarded + size / 2, 4096, PAGE_READWRITE | PAGE_GUARD, &old);
	check(mem::Readable(guarded, 4), "start of a guarded allocation");
	check(!mem::Readable(guarded + size / 2, 4), "a guard page");

	// A small region is asked about every time: freed, it reads as unreadable at once.
	auto* small = static_cast<uint8_t*>(VirtualAlloc(nullptr, 65536, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	check(mem::Readable(small, 4), "small region before free");
	VirtualFree(small, 0, MEM_RELEASE);
	check(!mem::Readable(small, 4), "small region after free");

	// A large one is trusted for kCacheMs at most.
	VirtualFree(big, 0, MEM_RELEASE);
	Sleep(static_cast<DWORD>(mem::kCacheMs) + 100);
	check(!mem::Readable(big, 4), "large region once the cache expired");

	if (failures) return 1;
	std::printf("ok\n");
	return 0;
}
