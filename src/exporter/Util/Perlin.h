#pragma once

#include <random>

namespace Util {
	inline void randSeed(int seed) {
		srand(seed);
	}

	inline float randFloat(float min = 0.f, float max = 1.f) {
		float zeroToOne = (((double)rand() / (RAND_MAX)));
		return (zeroToOne * (max - min)) - min;
	}

	inline double cosInterpolate(double a, double b, double x) {
		double ft = x * 3.1415927;
		double f = (1.0 - cos(ft))* 0.5;
		return a*(1.0 - f) + b*f;
	}

	inline double linearInterpolate(double a, double b, double x) {
		return (a * (1.0 - x)) + (b * x);
	}

	inline double interpolate2(double a, double b, double x) {
		double t = 0.5;
		if (x < 0.5) {
			t = x * x;
		}
		if (x > 0.5) {
			t = sqrt(x);
		}
		t = (b * t) + (a * (1.0 - t));
		return (0.5 * t) + (0.5 * cosInterpolate(a, b, cosInterpolate(0.0, 1.0, cosInterpolate(0.0, 1.0, x))));
		//return (b * x) + (a * (1.0 - x));
	}

	inline double findnoise2(double x, double y) {
		int n = (int)x + (int)y * 57;
		n = (n << 13) ^ n;
		int nn = (n*(n*n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
		return 1.0 - ((double)nn / 1073741824.0);
	}

	/* Returns perlin noise between -1 and 1*/
	double perlinNoise(double x, double y, int seed);
	/* Returns perlin noise between 0 and 1*/
	inline double perlinNoise01(double x, double y, int seed) {
		return (1.f + perlinNoise(x, y, seed)) / 2.f;
	}
	/* Returns perlin noise between -1 and 1*/
	double perlinNoise2(double x, double y, int seed);

	inline double perlinNoiseRange(double minimum, double maximum, double x, double y, int seed) {
		double size = maximum - minimum;
		return minimum + ((size / 2.f) * (perlinNoise(x, y, seed) + 1.f));
	}
}