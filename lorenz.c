#include <stdio.h>
#include <string.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "vec3.c"

#define WIDTH 600
#define HEIGHT 400
#define STEPS_PER_FRAME 4

float vertices[] = {
  /* position   color             texcoords */
  -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // Top-left
  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // Top-right
  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,  // Bottom-right
  -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // Bottom-left
};

GLuint elements[] = {
  0, 1, 2,
  2, 3, 0
};

const GLchar *vertex_source =
  "#version 330\n"

  "in vec3 color;\n"
  "in vec2 texcoord;\n"
  "in vec2 position;\n"

  "out vec3 Color;"
  "out vec2 Texcoord;"

  "void main() {\n"
  "  Color = color;"
  "  Texcoord = texcoord;"
  "  gl_Position = vec4(position, 0.0, 1.0);\n"
  "}\n";

const GLchar *fragment_source =
  "#version 330\n"
  "in vec3 Color;"
  "in vec2 Texcoord;"

  "out vec4 outColor;\n"

  "uniform sampler2D tex;"

  "void main() {"
  "  outColor = texture(tex, Texcoord);"
  "}";

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
  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window;

    /* Create a windowed mode window and its OpenGL context */
  window = glfwCreateWindow(800, 600, "Open.GL", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  /* Make the window's context current */
  glfwMakeContextCurrent(window);

  if (gl3wInit() != 0) {
    fprintf(stderr, "GL3W: failed to initialize\n");
    return 1;
  }

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);


  // Send vertex data to GPU
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Element buffer
  GLuint ebo;
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               sizeof(elements), elements, GL_STATIC_DRAW);


  /* Compile shaders and link program */
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_source, NULL);
  glCompileShader(vertex_shader);

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_source, NULL);
  glCompileShader(fragment_shader);

  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glBindFragDataLocation(shader_program, 0, "outColor");
  glLinkProgram(shader_program);

  glUseProgram(shader_program);

  GLint pos_attrib = glGetAttribLocation(shader_program, "position");
  glEnableVertexAttribArray(pos_attrib);
  glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE,
                        7*sizeof(float), 0);


  GLint color_attrib = glGetAttribLocation(shader_program, "color");
  glEnableVertexAttribArray(color_attrib);
  glVertexAttribPointer(color_attrib, 3, GL_FLOAT, GL_FALSE,
                        7*sizeof(float), (void*)(2*sizeof(float)));

  GLint tex_attrib = glGetAttribLocation(shader_program, "texcoord");
  glEnableVertexAttribArray(tex_attrib);
  glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE,
                        7*sizeof(float), (void*)(5*sizeof(float)));


  /* Texture */
  unsigned char backbuffer[HEIGHT][WIDTH][4];

  memset(backbuffer, 0, sizeof(backbuffer));

  GLuint texture;
  glGenTextures(1, &texture);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, backbuffer);
  glUniform1i(glGetUniformLocation(shader_program, "tex"), 0);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  float dt = 0.01;

  vec3 initial = {0.0, 1.0, 0.0};
  vec3 current = initial;

  #define TAIL_LENGTH 512

  vec3 tail[TAIL_LENGTH];
  int tail_index = 0;

  while (!glfwWindowShouldClose(window)) {
    memset(backbuffer, 0, sizeof(backbuffer));
    for (int i = 0; i < STEPS_PER_FRAME; i++) {
      tail[tail_index] = current;
      tail_index = (tail_index+1) % TAIL_LENGTH;

      current = rk4(current, dt);
      backbuffer[(int)(current.z*3.5)+100][(int)(current.y*3.5)+100][0] = 255;
      backbuffer[(int)(current.y*3.5)+100][(int)(current.x*3.5)+300][0] = 255;
    }

    for (int i = 0; i < TAIL_LENGTH; i++) {
      vec3 c = tail[i];
      backbuffer[(int)(c.z*3.5)+100][(int)(c.y*3.5)+100][0] = 255;
      backbuffer[(int)(c.y*3.5)+100][(int)(c.x*3.5)+300][0] = 255;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, backbuffer);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glfwSwapBuffers(window);

    glfwPollEvents();
  }
}
