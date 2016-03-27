#version 330

in vec3 color;
in vec2 texcoord;
in vec2 position;

out vec3 Color;
out vec2 Texcoord;

void main() {
  Color = color;
  Texcoord = texcoord;
  gl_Position = vec4(position, 0.0, 1.0);
}
