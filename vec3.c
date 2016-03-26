typedef struct {
  float x, y, z;

} vec3;

vec3
vec3_add(vec3 a, vec3 b) {
  vec3 result;

  result.x = a.x + b.x;
  result.y = a.y + b.y;
  result.z = a.z + b.z;

  return result;
}

vec3
vec3_scale(float t, vec3 a) {
  vec3 result;

  result.x = t * a.x;
  result.y = t * a.y;
  result.z = t * a.z;

  return result;
}
