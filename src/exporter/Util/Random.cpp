#include "Random.h"
#include <random>
#include <cstdint>

using namespace Util;

std::string Util::randomString(std::string::size_type length) {
	static auto& chrs = "abcdefghijklmnopqrstuvwxyz";

	thread_local static std::mt19937 rg{ std::random_device{}() };
	thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

	std::string s;
	s.reserve(length);

	while (length--)
		s += chrs[pick(rg)];

	return s;
}

std::string Util::randomNumberString(std::string::size_type length) {
	static auto& chrs = "01234567890";

	thread_local static std::mt19937 rg{ std::random_device{}() };
	thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

	std::string s;
	s.reserve(length);

	while (length--)
		s += chrs[pick(rg)];

	return s;
}

float Util::glslRandom(tf_v0_vec2 seed) {

	int x = seed.x;
	int y = seed.y;

	for (int i = 0; i < 4; i++) {
		x += (x << 29);
		x ^= (x << 11);
		x ^= (y << 17);
		x ^= (x >> 19);
		x ^= (y >> 6);
		x ^= (x << 4);
	}
	
	return (x & 8191) / 8192.0;
}

// float Util::randFloat(){
// 	 return randFloat(std::random_device{}());
// }

float Util::randFloat(int seed){
	thread_local static auto rand = DeterministicRandom{};
	rand.seed(seed);
	return rand.randFloat();
}

namespace {
	uint32_t mixSeed(uint32_t seed) {
		seed += 0x9E3779B9u;
		seed = (seed ^ (seed >> 16)) * 0x85EBCA6Bu;
		seed = (seed ^ (seed >> 13)) * 0xC2B2AE35u;
		seed ^= (seed >> 16);

		return seed;
	}
}

void DeterministicRandom::seed(int seedValue) {
	auto hashed = mixSeed(static_cast<uint32_t>(seedValue));
	hashed = (hashed % 0x7FFFFFFEu) + 1u;
	distribution.seed(hashed);
}

int DeterministicRandom::randInt(int min, int max) {
	int range = max - min;
	return (distribution() % range) + min;
}

float DeterministicRandom::randFloat() {
	return distribution() / 2147483647.f;
}