#version 330

in float fade;
uniform vec3 color;

out vec4 outColor;

void main() {
  outColor = vec4(color, fade);
}
