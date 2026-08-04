#pragma once


namespace Util {
	template<class T, class U>
	bool compare(const T& t1, const T& t2, const U& u1, const U& u2) {
		if (t1 == t2) {
			return u1 < u2;
		} else {
			return t1 < t2;
		}
	}

	template<class T, class U, class V>
	bool compare(const T& t1, const T& t2, const U& u1, const U& u2, const V& v1, const V& v2) {
		if (t1 == t2) {
			if (u1 == u2) {
				return v1 < v2;
			} else {
				return u1 < u2;
			}
		} else {
			return t1 < t2;
		}
	}

	template<class T, class U, class V, class W>
	bool compare(const T& t1, const T& t2, const U& u1, const U& u2, const V& v1, const V& v2, const W& w1, const W& w2) {
		if (t1 == t2) {
			if (u1 == u2) {
				if (v1 == v2) {
					return w1 < w2;
				} else {
					return v1 < v2;
				}
			} else {
				return u1 < u2;
			}
		} else {
			return t1 < t2;
		}
	}

	template<class T, class U, class V, class W, class X>
	bool compare(const T& t1, const T& t2, const U& u1, const U& u2, const V& v1, const V& v2, const W& w1, const W& w2, const X& x1, const X& x2) {
		if (t1 == t2) {
			if (u1 == u2) {
				if (v1 == v2) {
					if (w1 == w2) {
						return x1 < x2;
					} else {
						return w1 < w2;
					}
				} else {
					return v1 < v2;
				}
			} else {
				return u1 < u2;
			}
		} else {
			return t1 < t2;
		}
	}
}