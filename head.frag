#version 330

in vec3 Color;
out vec4 outColor;

void main() {
  outColor = vec4(Color / 255.0, 1.0);
}
