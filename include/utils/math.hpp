
#ifndef GAME_UTILS_MATH
#define GAME_UTILS_MATH

#define _USE_MATH_DEFINES
#include <cmath>

#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include "../utils/globals.hpp"

#ifdef __GNUC__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#endif // __GNUC__

template <typename U>
static inline U math_abs(const U x)
{
	return (x >= (U)0 ? (U)x : (U)-x);
}

template <typename U>
static inline U math_signalt(const U x)
{
	return (x >= (U)0 ? (U)1 : (U)-1);
}

template <typename U>
static inline U math_sign(const U x)
{
	if (x == (U)0)
		return (U)0;
	return math_signalt(x);
}

template <typename U>
static U math_sqrt(U x)
{
	return (U)sqrt((double)x);
}

template <>
float math_sqrt(float x)
{
	return sqrtf(x);
}

// Clearly not my code
template <>
int math_sqrt(int n)
{
	int g = 0x8000;
	int c = 0x8000;
	for (;;)
	{
		if (g * g > n)
		{
			g ^= c;
		}
		c >>= 1;
		if (c == 0)
		{
			return g;
		}
		g |= c;
	}
}

// https://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
static inline uint32_t log_2(uint64_t n)
{
	static const int table[64] = {
		0, 58, 1, 59, 47, 53, 2, 60, 39, 48, 27, 54, 33, 42, 3, 61,
		51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4, 62,
		57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56,
		45, 25, 31, 35, 16, 9, 12, 44, 24, 15, 8, 23, 7, 6, 5, 63};

	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n |= n >> 32;

	return table[(n * 0x03f6eaf2cd271461) >> 58];
}

// https://stackoverflow.com/a/4636854
template <typename U>
inline U
math_floordiv(U num, U den)
{
	static_assert(std::is_integral<U>::value, "math_floordiv only takes integers.");
	if (0 < (num ^ den))
		return num / den;
	else
	{
		auto res = div(num, den);
		return (res.rem) ? res.quot - 1
						 : res.quot;
	}
}

// Function overloading for flooring variables
template <typename U, typename std::enable_if_t<std::is_integral<U>::value, bool> = true>
constexpr U math_floor(const U &x)
{
	return x;
}

inline float math_floor(float x)
{
	return floorf(x);
}

inline double math_floor(double x)
{
	return floor(x);
}

template <typename U>
inline U
math_mod(U x, U m)
{
	static_assert(std::is_integral<U>::value, "math_mod only takes integers.");
	return ((x % m) + m) % m;
}

template <typename U = float>
inline float
math_mod(float x, float m)
{
	return fmodf(fmodf(x, m) + m, m);
}

template <typename U>
static inline U math_max(U a, U b)
{
	return (a > b) ? a : b;
}

template <typename U>
static inline U math_min(U a, U b)
{
	return (a < b) ? a : b;
}

template <typename U>
static inline bool math_between(const U &x, const U &a, const U &b)
{
	return a <= x && x < b;
}

template <typename U>
U math_distsq(U a, U b, U A, U B)
{
	return (a - A) * (a - A) + (b - B) * (b - B);
}

template <typename U>
U math_clamp(U x, U min, U max)
{
	return x < min ? min : (x > max ? max : x);
}

template <typename U = float>
static float math_map(U x, float a, float b, float A, float B)
{
	return (x - a) * (B - A) / (b - a) + A;
}

template <typename Vec>
inline bool vec_compare(const Vec &vecA, const Vec &vecB)
{
	if (vecA.y == vecB.y)
		return vecA.x < vecB.x;
	return vecA.y < vecB.y;
}

template <typename Vec>
static void vec_abs(Vec &vec)
{
	vec.x = math_abs(vec.x);
	vec.y = math_abs(vec.y);
}

inline int vec_dist(const IVec &a, const IVec &b)
{
	return math_sqrt<int>((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

inline float vec_dist(const FVec &a, const FVec &b)
{
	return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

template <typename Vec>
inline int vec_dist(const Vec &a, const Vec &b)
{
	return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

template <typename Vec>
inline float vec_dist_circles(
	const Vec &a,
	const float ra,
	const Vec &b,
	const float rb)
{
	float d = vec_dist(a, b) - ra - rb;
	return (d < 0.0f) ? 0.0f : d;
}

template <typename U>
static bool vec_inside(
	const U& pos,
	const U& rectCorner,
	const U& rectSize)
{
	return
		math_between(pos.x,
			rectCorner.x,
			rectCorner.x + rectSize.x)
		&&
		math_between(pos.y,
			rectCorner.y,
			rectCorner.y + rectSize.y);
}

template <typename U>
inline bool vec_inside(
	const sf::Vector2<U> &v,
	U x,
	U y,
	U w,
	U h)
{
	return vec_inside(v, {x, y}, {w, h});
}

template <typename U>
inline sf::Vector2<U> vec_reverse(const sf::Vector2<U> &v)
{
	return sf::Vector2<U>{v.y, v.x};
}

template <typename U>
inline sf::Vector2<U> vec_mult(const sf::Vector2<U> &v1, const U &x)
{
	return sf::Vector2<U>{v1.x * x, v1.y * x};
}

template <typename U>
inline sf::Vector2<U> vec_mult(const sf::Vector2<U> &v1, const sf::Vector2<U> &v2)
{
	return sf::Vector2<U>{v1.x * v2.x, v1.y * v2.y};
}

#define TYPE_T_U decltype(std::declval<T>() * std::declval<U>())

template <typename T, typename U>
inline sf::Vector2<TYPE_T_U> operator*(const sf::Vector2<T> &v1, const sf::Vector2<U> &v2)
{
	return sf::Vector2<TYPE_T_U>{v1.x * v2.x, v1.y * v2.y};
}

template <typename T, typename U>
inline sf::Vector2<TYPE_T_U> operator/(const sf::Vector2<T> &v1, const sf::Vector2<U> &v2)
{
	return sf::Vector2<TYPE_T_U>{v1.x / v2.x, v1.y / v2.y};
}

template <typename T, typename U>
inline sf::Vector2<TYPE_T_U> vec_mult(const sf::Vector2<T> &v1, const sf::Vector2<U> &v2)
{
	return sf::Vector2<TYPE_T_U>{v1.x * v2.x, v1.y * v2.y};
}

template <typename U = float>
static constexpr inline U vec_distsq(const Vec<U> &v1, const Vec<U> &v2)
{
	U dx = v2.x - v1.x, dy = v2.y - v1.y;
	return (dx * dx + dy * dy);
}

template <typename U = float>
inline bool vec_cmp_dist(const Vec<U> &posA, const Vec<U> &posB, const Vec<U> &center)
{
	return vec_distsq(posA, center) < vec_distsq(posB, center);
}

template <typename U = float>
static U vec_lensq(const Vec<U> &v)
{
	return (v.x * v.x + v.y * v.y);
}

template <typename U = float>
static U vec_lensq(const Vec<U> &&v)
{
	return (v.x * v.x + v.y * v.y);
}

template <typename U>
static U vec_len(const Vec<U> &vec)
{
	return (U)math_sqrt<U>(vec_lensq<U>(vec));
}

template <typename U>
static Vec<U> vec_from_angle(
	const float angle)
{
	float x = cos(angle);
	float y = sin(angle);
	return Vec<U>(x, y);
}

// https://stackoverflow.com/a/16544330
template <typename U>
static U vec_angle_between(
	const sf::Vector2<U> &vecA,
	const sf::Vector2<U> &vecB)
{
	U dot = vecA.x * vecB.x + vecA.y * vecB.y;
	U det = vecA.x * vecB.y - vecA.y * vecB.x;
	return (U)atan2(det, dot);
}

template <typename U>
static float vec_normalize(sf::Vector2<U> &vec)
{
	float len = vec_len<U>(vec);
	if (len == (U)0)
	{
		vec = {(U)0, (U)0};
		return (U)0;
	}
	vec /= len;
	return len;
}

template <typename U>
static void vec_limit(sf::Vector2<U> &vec, U limit)
{
	U len = math_sqrt<U>(vec_len<U>(vec));
	if (len > limit)
	{
		vec = vec / len * limit;
	}
}

template <typename T, typename U>
static sf::Vector2<U> vec_convert(const sf::Vector2<T> v)
{
	sf::Vector2<U> out = {(U)v.x, (U)v.y};
	return out;
}

template <typename U>
static inline void vec_frames_apply(sf::Vector2<U> &dest, const sf::Vector2<U> &source)
{
	dest.x = (source.x >= 0) ? dest.x : source.x;
	dest.y = (source.y >= 0) ? dest.y : source.y;
}

template <typename U, typename std::enable_if_t<std::is_integral<U>::value, bool> = true>
static constexpr sf::Vector2<U> vec_floor(const sf::Vector2<U> &v)
{
	return v;
}

static sf::Vector2f vec_floor(const sf::Vector2f &v)
{
	return {floorf(v.x), floorf(v.y)};
}

template <typename U>
static inline U vec_prod(const sf::Vector2<U> &x)
{
	return x.x * x.y;
}

static inline sf::Vector2<double> vec_floor(const sf::Vector2<double> &v)
{
	return {floor(v.x), floor(v.y)};
}

static inline IVec vec_pos_to_tile(const FVec &v)
{
	return IVec{(int)floorf(v.x), (int)floorf(v.y)};
}

static inline FVec vec_tile_to_pos(const IVec &v)
{
	return (FVec)v + FVec{0.5f, 0.5f};
}

static inline bool aabb_intersect_circle(
	const FVec boxCenter,
	const FVec boxSize,
	const FVec circlePos,
	const float circleRadius)
{
	auto boxPos = boxCenter - boxSize / 2.f;
	float x = math_clamp(
		circlePos.x,
		boxPos.x,
		boxPos.x + boxSize.x);
	float y = math_clamp(
		circlePos.y,
		boxPos.y,
		boxPos.y + boxSize.y);
	float d = vec_distsq(circlePos, {x, y});
	return d <= circleRadius * circleRadius;
}

static inline FVec aabb_closest_point(
	const FVec &boxCenter,
	const FVec &boxSize,
	const FVec &point)
{
	FVec start = boxCenter - boxSize / 2.f;
	FVec end = boxCenter + boxSize / 2.f;
	return {math_clamp(point.x, start.x, end.x),
			math_clamp(point.y, start.y, end.y)};
}

/*
static float aabb_distance(
	const FVec &vec,
	const FVec &p1, const FVec &sizeB)
{
	return vec_dist(
		aabb_closest_point(p1, sizeB, vec),
		vec);
}*/

template <typename T>
Vec<T> vec_rotate_2d(const Vec<T>& v, float a )
{
	T x = v.x * cosf(a) - v.y * sinf(a);
	T y = v.x * sinf(a) + v.y * cosf(a);
	return { x, y };
}

template <typename T>
Vec<int> vec_90_rotate_axis(T x)
{
	const static Vec<int> table[] = {{1, 1}, {-1, 1}, {-1, -1}, {1, -1}};
	return table[(int)math_mod(x, 4)];
}

template <typename T>
bool vec_90_rotate_reverse(T x)
{
	const static bool table[] = {false, true, false, true};
	return table[(int)math_mod(x, 4)];
}

template <typename T, typename U>
Vec<U> vec_90_rotate(const Vec<U> &v, T x)
{
	//return vec_mult(v, vec_90_rotate_axis(x));

	switch ((int)math_mod(x, 4))
	{
	case 0:
		return Vec<U>{v.x, v.y};
	case 1:
		return Vec<U>{v.y, -v.x};
	case 2:
		return Vec<U>{-v.x, -v.y};
	case 3:
		return Vec<U>{-v.y, v.x};
	}
	return {0, 0};
}

inline FVec delta_from_crect(
	const FVec &pos,
	float radius,
	const FVec &rectPos,
	FVec rectSize)
{
	FVec dist = pos - rectPos;
	FVec delta;
	rectSize.x += radius;
	rectSize.y += radius;
	if (fabs(dist.x) > fabs(dist.y))
		delta.x = ((dist.x > 0.0) ? 1 : -1) * rectSize.x / 2.f - dist.x;
	else
		delta.y = ((dist.y > 0.0) ? 1 : -1) * rectSize.y / 2.f - dist.y;
	return delta * 1.f;
}

static bool aabb_collision_center(
	const FVec &centerA,
	const FVec &sizeA,
	const FVec &centerB,
	const FVec &sizeB)
{
	float dx, dy;
	dx = fabs(centerA.x - centerB.x);
	dy = fabs(centerA.y - centerB.y);
	return dx < (sizeA.x + sizeB.x) / 2.f &&
		   dy < (sizeA.y + sizeB.y) / 2.f;
}

inline FVec aabb_center_to_corner(
	const FVec &centerA,
	const FVec &sizeA)
{
	return FVec{
		centerA.x - sizeA.x / 2,
		centerA.y - sizeA.y / 2};
}

// https://stackoverflow.com/a/35066128
static float aabb_distance_rectangle(
	const FVec &centerA,
	const FVec &sizeA,
	const FVec &centerB,
	const FVec &sizeB)
{
	FVec dif = centerA - centerB;
	vec_abs(dif);
	dif -= sizeA / 2.f + sizeB / 2.f;
	return vec_len(FVec{
		math_max(0.f, dif.x),
		math_max(0.f, dif.y)});
}

// https://stackoverflow.com/a/35066128
static float aabb_distance_point(
	const FVec &centerA,
	const FVec &centerB,
	const FVec &sizeB)
{
	FVec dif = centerA - centerB;
	vec_abs(dif);
	dif -= sizeB / 2.f;
	return vec_len(FVec{
		math_max(0.f, dif.x),
		math_max(0.f, dif.y)});
}

inline float aabb_distance_circle(
	const FVec &a,
	const float r,
	const FVec &b,
	const FVec &s)
{
	float d = aabb_distance_point(a, b, s) - r;
	return (d < 0.0f) ? 0.0f : d;
}

#ifdef __GNUC__

#pragma GCC diagnostic pop

#endif // __GNUC__

#endif // GAME_UTILS_MATH