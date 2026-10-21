#include "jaudio/ja_calc.h"

#include "jaudio/sdk.h"

// MSL C++'s std::sqrtf; BS2 keeps it out of line here (sqrtf__3stdFf)
namespace std {
inline f32 sqrtf(f32 x)
{
	static const f64 _half  = 0.5;
	static const f64 _three = 3.0;

	vf32 y;
	if (x > 0.0f) {
#ifdef __MWERKS__
		f64 guess = __frsqrte((f64)x);
#else
		f64 guess = 1.0;
#endif
		guess = _half * guess * (_three - guess * guess * x);
		guess = _half * guess * (_three - guess * guess * x);
		guess = _half * guess * (_three - guess * guess * x);
		y     = (f32)(x * guess);
		return y;
	}
	return x;
}
} // namespace std

#define SINTABLE_LENGTH (257)
static f32 SINTABLE[SINTABLE_LENGTH];

/**
 * @TODO: Documentation
 * BS2: 81352DA0, size 0x20
 */
f32 sqrtf2(f32 x)
{
	return std::sqrtf(x);
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000020
 */
void cosf2(f32)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 */
f32 atanf2(f32 x, f32 y)
{
	return atan2(x, y);
}

/**
 * @TODO: Documentation
 * @note UNUSED Size: 000020
 */
void sinf2(f32)
{
	TRAP_UNIMPLEMENTED;
}

/**
 * @TODO: Documentation
 * BS2: 81352E40, size 0x88
 */
void Jac_InitSinTable()
{
	for (u32 i = 0; i < SINTABLE_LENGTH; i++) {
		SINTABLE[i] = sinf(i * HALF_PI / 256.0f);
	}
}

/**
 * @TODO: Documentation
 * BS2: 81352EE0, size 0x34
 */
f32 sinf3(f32 x)
{
	return SINTABLE[(int)(256.0f * x)];
}
