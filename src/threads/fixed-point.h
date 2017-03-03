#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H 

#define FRAC (1 << 14)

#define convert_to_fixed(n) (n * FRAC)
#define round_down(x) (x / FRAC)
#define round_nearest(x) (x >= 0 ? ((x + FRAC / 2) / FRAC) : ((x - FRAC / 2) / FRAC))
#define add_fixed(x,y) (x + y)
#define add_int(x,n) (x + n * FRAC)
#define sub_fixed(x,y) (x - y)
#define sub_int(x,n) (x - n * FRAC)
#define multiply_fixed(x,y) ((int)(((int64_t)x)*y/FRAC))
#define multiply_int(x,n) (x * n)
#define divide_fixed(x,y) ((int)(((int64_t)x)*FRAC/y))
#define divide_int(x,n) (x / n)

#endif
