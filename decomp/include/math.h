#ifndef _MSL_MATH_H
#define _MSL_MATH_H

float tanf(float x);
float sinf(float x);
float cosf(float x);

#ifdef __MWERKS__
// MSL's inline sqrtf (three Newton steps on frsqrte); the SDK's GX code inlines it
static inline float sqrtf(float x) {
    static const double _half = .5;
    static const double _three = 3.0;
    volatile float y;
    if (x > 0.0f) {
        double guess = __frsqrte((double)x);
        guess = _half * guess * (_three - guess * guess * x);
        guess = _half * guess * (_three - guess * guess * x);
        guess = _half * guess * (_three - guess * guess * x);
        y = (float)(x * guess);
        return y;
    }
    return x;
}
#else
float sqrtf(float x);
#endif

#endif

