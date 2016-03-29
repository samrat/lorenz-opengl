#version 330

in vec3 position;

uniform vec3 rotation;
uniform vec3 translation;


mat4 view_frustum(float angle_of_view,
                  float aspect_ratio,
                  float z_near,
                  float z_far) {
  return mat4(1.0/tan(angle_of_view), 0.0, 0.0, 0.0,
              0.0, aspect_ratio/tan(angle_of_view), 0.0, 0.0,
              0.0, 0.0, (z_far+z_near)/(z_far-z_near), 1.0,
              0.0, 0.0, -2.0*z_far*z_near/(z_far-z_near), 0.0);
}

mat4 scale(float x, float y, float z) {
  return mat4(x, 0.0, 0.0, 0.0,
              0.0, y, 0.0, 0.0,
              0.0, 0.0, z, 0.0,
              0.0, 0.0, 0.0, 1.0);
}

mat4 translate(float x, float y, float z) {
  return mat4(1.0, 0.0, 0.0, 0.0,
              0.0, 1.0, 0.0, 0.0,
              0.0, 0.0, 1.0, 0.0,
              x, y, z, 1.0);
}

mat4 rotate_x(float t) {
  float st = sin(t);
  float ct = cos(t);
  return mat4(1.0, 0.0, 0.0, 0.0,
              0.0, ct, st, 0.0,
              0.0, -st, ct, 0.0,
              0.0, 0.0, 0.0, 1.0);
}

mat4 rotate_y(float t) {
    float st = sin(t);
    float ct = cos(t);
    return mat4(
        vec4( ct, 0.0,  st, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(-st, 0.0,  ct, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );
}

mat4 rotate_z(float t) {
    float st = sin(t);
    float ct = cos(t);
    return mat4(
        vec4( ct,  st, 0.0, 0.0),
        vec4(-st,  ct, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );
}

void main() {
  gl_Position = view_frustum(radians(45.0), 4.0/3.0, 0.0, 10.0)
    * translate(translation.x, translation.y, translation.z)
    * rotate_x(rotation.x)
    * rotate_y(rotation.y)
    * rotate_z(rotation.z)
    * scale(1/25.0, 1.0/25.0, 1.0/25.0)
    * vec4(position, 1.0);
}
