#include <stdio.h>

#include "vec3.c"

vec3
F(vec3 state) {
  vec3 result;

  /*
     x' = 10(y-x)
     y' = 28x - y - xz
     z' = (-8/3)z + xy
  */

  result.x = 10 * (state.y - state.x);
  result.y = 28*state.x - state.y - state.x*state.z;
  result.z = (-8.0f/3.0f)*state.z + state.x*state.y;

  return result;
}

vec3
rk4_weighted_avg(vec3 a, vec3 b, vec3 c, vec3 d) {
  vec3 result;

  result.x = (a.x + 2*b.x + 2*c.x + d.x) / 6.0;
  result.y = (a.y + 2*b.y + 2*c.y + d.y) / 6.0;
  result.z = (a.z + 2*b.z + 2*c.z + d.z) / 6.0;

  return result;
}

vec3
rk4(vec3 current, float dt) {
  vec3 k1 = F(current);
  vec3 k2 = F(vec3_add(current, vec3_scale(dt/2, k1)));
  vec3 k3 = F(vec3_add(current, vec3_scale(dt/2, k2)));
  vec3 k4 = F(vec3_add(current, vec3_scale(dt, k3)));

  vec3 k = rk4_weighted_avg(k1, k2, k3, k4);

  vec3 result = vec3_add(current, vec3_scale(dt, k));

  return result;
}

int
main() {
  float dt = 0.01;

  vec3 initial = {0.0, 1.0, 0.0};
  vec3 current = initial;

  while (1) {
    current = rk4(current, dt);
    printf("%f %f %f\n", current.x, current.y, current.z);
  }
}
