#ifndef _STUB_MATH_H
#define _STUB_MATH_H

double      floor(double x);
double      ceil(double x);
double      round(double x);
double      fabs(double x);
double      sqrt(double x);
double      pow(double x, double y);
double      log(double x);
double      log2(double x);
double      log10(double x);
double      exp(double x);
double      sin(double x);
double      cos(double x);
double      tan(double x);
double      asin(double x);
double      acos(double x);
double      atan(double x);
double      atan2(double y, double x);
double      fmod(double x, double y);
double      frexp(double x, int *exp);
double      ldexp(double x, int exp);

float       floorf(float x);
float       ceilf(float x);
float       roundf(float x);
float       fabsf(float x);
float       sqrtf(float x);
float       powf(float x, float y);
float       logf(float x);
float       log2f(float x);
float       log10f(float x);
float       expf(float x);
float       sinf(float x);
float       cosf(float x);
float       tanf(float x);
float       asinf(float x);
float       acosf(float x);
float       atanf(float x);
float       atan2f(float y, float x);
float       fmodf(float x, float y);

#ifndef INFINITY
#define INFINITY (__builtin_inff())
#endif

#ifndef NAN
#define NAN (__builtin_nanf(""))
#endif

#ifndef HUGE_VAL
#define HUGE_VAL (__builtin_huge_val())
#endif

#endif
