#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "vec3.c"
#include "util.c"

#define WIDTH 800
#define HEIGHT 600

#define STEPS_PER_FRAME 5
#define TAIL_LENGTH 1024

static struct {
  GLuint vertex_buffer, element_buffer;
  GLuint tail_vertex_buffer;

  GLuint projection_vertex_shader;
  GLuint head_vertex_shader, head_fragment_shader, head_program;
  GLuint tail_vertex_shader, tail_fragment_shader, tail_program;


  struct {
    GLuint tex;
  } uniforms;

  struct {
    GLuint position, color, texcoord;
  } attributes;

  unsigned char backbuffer[HEIGHT][WIDTH][4];

} g_gl_state;

static float position[] = {     /* head position */
  0.0, 0.0, 0.0,
};

vec3 tail[TAIL_LENGTH];

static GLuint
make_buffer(GLenum target,
            const void *buffer_data,
            GLsizei buffer_size) {
  GLuint buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(target, buffer);
  glBufferData(target, buffer_size, buffer_data, GL_DYNAMIC_DRAW);

  return buffer;
}

static void
show_info_log(GLuint object,
              PFNGLGETSHADERIVPROC glGet__iv,
              PFNGLGETSHADERINFOLOGPROC glGet__InfoLog) {
  GLint log_length;
  char *log;

  glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
  log = malloc(log_length);
  glGet__InfoLog(object, log_length, NULL, log);
  fprintf(stderr, "%s", log);
  free(log);
}


static GLuint
make_shader(GLenum type, const char *filename) {
  GLint length;
  GLchar *source = file_contents(filename, &length);
  GLuint shader;
  GLint shader_ok;

  if (!source)
    return 0;

  shader = glCreateShader(type);
  glShaderSource(shader, 1, (const GLchar**)&source, &length);
  free(source);
  glCompileShader(shader);

  /* Check that shader compiled properly */
  glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
  if (!shader_ok) {
    fprintf(stderr, "Failed to compile %s:\n", filename);
    show_info_log(shader, glGetShaderiv, glGetShaderInfoLog);
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

static GLuint
make_program(GLuint vertex_shader, GLuint fragment_shader) {
  GLint program_ok;

  GLuint program = glCreateProgram();
  // glAttachShader(program, g_gl_state.projection_vertex_shader);
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glBindFragDataLocation(program, 0, "outColor");
  glLinkProgram(program);

  glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
  if (!program_ok) {
    fprintf(stderr, "Failed to link shader program:\n");
    show_info_log(program, glGetProgramiv, glGetProgramInfoLog);
    glDeleteProgram(program);
    return 0;
  }

  return program;
}

static int
make_resources(void) {
  /* Create buffers */
  g_gl_state.vertex_buffer = make_buffer(GL_ARRAY_BUFFER,
                                         position,
                                         sizeof(position));
  g_gl_state.tail_vertex_buffer = make_buffer(GL_ARRAY_BUFFER,
                                              tail,
                                              sizeof(tail));

  /* g_gl_state.element_buffer = make_buffer(GL_ELEMENT_ARRAY_BUFFER, */
  /*                                         elements, */
  /*                                         sizeof(elements)); */

  /* Compile GLSL program  */
  g_gl_state.projection_vertex_shader = make_shader(GL_VERTEX_SHADER,
                                                    "project.vert");

  g_gl_state.head_vertex_shader = make_shader(GL_VERTEX_SHADER,
                                         "head.vert");
  g_gl_state.head_fragment_shader = make_shader(GL_FRAGMENT_SHADER,
                                           "head.frag");
  g_gl_state.head_program = make_program(g_gl_state.head_vertex_shader,
                                         g_gl_state.head_fragment_shader);

  g_gl_state.tail_vertex_shader = make_shader(GL_VERTEX_SHADER,
                                              "tail.vert");
  g_gl_state.tail_fragment_shader = make_shader(GL_FRAGMENT_SHADER,
                                                "tail.frag");
  g_gl_state.tail_program = make_program(g_gl_state.tail_vertex_shader,
                                         g_gl_state.tail_fragment_shader);


  /* Look up shader variable locations */
  g_gl_state.attributes.position =
    glGetAttribLocation(g_gl_state.head_program, "position");

  /* g_gl_state.uniforms.tex = */
  /*   glGetUniformLocation(g_gl_state.program, "tex"); */

  return 1;
}

static void
render(GLFWwindow *window) {
  glUseProgram(g_gl_state.head_program);

  /*  */

  glEnableVertexAttribArray(g_gl_state.attributes.position);
  glVertexAttribPointer(g_gl_state.attributes.position,
                        3, GL_FLOAT, GL_FALSE,
                        3*sizeof(float), 0);

  /* glEnableVertexAttribArray(g_gl_state.attributes.color); */
  /* glVertexAttribPointer(g_gl_state.attributes.color, */
  /*                       3, GL_FLOAT, GL_FALSE, */
  /*                       7*sizeof(float), (void*)(2*sizeof(float))); */

  /* glEnableVertexAttribArray(g_gl_state.attributes.texcoord); */
  /* glVertexAttribPointer(g_gl_state.attributes.texcoord, */
  /*                       2, GL_FLOAT, GL_FALSE, */
  /*                       7*sizeof(float), (void*)(5*sizeof(float))); */
  /* glUniform1i(g_gl_state.uniforms.tex, 0); */

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  glDrawArrays(GL_POINTS, 0, 1);


  glUseProgram(g_gl_state.tail_program);
  glEnableVertexAttribArray(g_gl_state.attributes.position);
  glVertexAttribPointer(g_gl_state.attributes.position,
                        TAIL_LENGTH, GL_FLOAT, GL_FALSE,
                        TAIL_LENGTH*sizeof(float), 0);
  glDrawArrays(GL_POINTS, 0, TAIL_LENGTH);

  glfwSwapBuffers(window);
}

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
  window = glfwCreateWindow(WIDTH, HEIGHT, "Lorenz System", NULL, NULL);
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

  make_resources();

  float dt = 0.005;

  vec3 initial = {0.0, 1.0, 0.0};
  vec3 current = initial;

  for (int i = 0; i < TAIL_LENGTH; i++) {
    tail[i].x = 0.0f;
    tail[i].y = 0.0f;
    tail[i].z = 0.0f;
  }
  int tail_index = 0;

  while (!glfwWindowShouldClose(window)) {
    //memset(g_gl_state.backbuffer, 0, sizeof(g_gl_state.backbuffer));
    for (int i = 0; i < STEPS_PER_FRAME; i++) {
      tail[tail_index] = current;
      tail_index = (tail_index+1) % TAIL_LENGTH;

      current = rk4(current, dt);
      position[0] = current.x + 10;
      position[1] = current.y + 10;
      position[2] = current.z;
      // printf("%f %f %f\n", current.x, current.y, current.z);
      glBindBuffer(GL_ARRAY_BUFFER, g_gl_state.vertex_buffer);
      glBufferData(GL_ARRAY_BUFFER,
                   sizeof(position), position, GL_DYNAMIC_DRAW);

    }

    glBindBuffer(GL_ARRAY_BUFFER, g_gl_state.tail_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(tail), tail, GL_DYNAMIC_DRAW);

    render(window);

    glfwPollEvents();
  }
}
