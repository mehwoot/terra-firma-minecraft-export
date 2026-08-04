#pragma once

#include <Api/v0/Vector.h>

#include <string>
#include <random>

namespace Util {
	std::string randomString(std::string::size_type length);
	std::string randomNumberString(std::string::size_type length);
	float glslRandom(tf_v0_vec2 seed);
	int randInt(int seed);
	// float randFloat();
	float randFloat(int seed);

	class DeterministicRandom {
	protected:
		std::minstd_rand distribution;

	public:
		void seed(int seedValue);
		// returns a random integer in the range [min, max)
		int randInt(int min = 0, int max = 2147483647);
		float randFloat();
	};
}

