
#include "Perlin.h"

/* Returns semi-random float between -1 and 1*/
double Util::perlinNoise(double x, double y, int seed) {
	x += seed;
	y += seed;
	double floorx = (double)((int)x);//This is kinda a cheap way to floor a double integer.
	double floory = (double)((int)y);
	double s, t, u, v;//Integer declaration
	s = findnoise2(floorx, floory);
	t = findnoise2(floorx + 1, floory);
	u = findnoise2(floorx, floory + 1);//Get the surrounding pixels to calculate the transition.
	v = findnoise2(floorx + 1, floory + 1);
	double int1 = cosInterpolate(s, t, x - floorx);//Interpolate between the values.
	double int2 = cosInterpolate(u, v, x - floorx);//Here we use x-floorx, to get 1st dimension. Don't mind the x-floorx thingie, it's part of the cosine formula.
	return cosInterpolate(int1, int2, y - floory);//Here we use y-floory, to get the 2nd dimension.
											   //return interpolate(s,u,y-floory);//Here we use y-floory, to get the 2nd dimension.

}

/* Returns semi-random float between -1 and 1*/
double Util::perlinNoise2(double x, double y, int seed) {
	x += seed;
	y += seed;
	double floorx = (double)((int)x);//This is kinda a cheap way to floor a double integer.
	double floory = (double)((int)y);
	double s, t, u, v;//Integer declaration
	s = findnoise2(floorx, floory);
	t = findnoise2(floorx + 1, floory);
	u = findnoise2(floorx, floory + 1);//Get the surrounding pixels to calculate the transition.
	v = findnoise2(floorx + 1, floory + 1);
	double int1 = cosInterpolate(s, t, x - floorx);//Interpolate between the values.
	double int2 = cosInterpolate(u, v, x - floorx);//Here we use x-floorx, to get 1st dimension. Don't mind the x-floorx thingie, it's part of the cosine formula.
												//std::cout << int1 << " " << int2 << " " << interpolate2(int1,int2,y-floory) << ":" << interpolate(int1,int2,y-floory) << std::endl;
	return interpolate2(int1, int2, y - floory);//Here we use y-floory, to get the 2nd dimension.
												//return interpolate(s,u,y-floory);//Here we use y-floory, to get the 2nd dimension.
}