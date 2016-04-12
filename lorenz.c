#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "vec3.c"
#include "util.c"

#define WIDTH 800
#define HEIGHT 600

#define COUNT 5
#define STEPS_PER_FRAME 3
#define TAIL_LENGTH 1024
#define SIGMA 10.0f
#define BETA (-8.0f/3.0f)
#define RHO 28.0f

typedef enum {BUTTON_NONE, BUTTON_LEFT, BUTTON_MIDDLE, BUTTON_RIGHT} mouse_button;

static struct {
  GLuint vertex_buffer, element_buffer;
  GLuint tail_vertex_buffer;
  GLuint tail_index_buffer;
  GLuint colors_buffer;

  GLuint projection_vertex_shader;
  GLuint head_vertex_shader, head_fragment_shader, head_program;
  GLuint tail_vertex_shader, tail_fragment_shader, tail_program;

  struct {
    struct {
      GLuint rotation, translation;
    } uniforms;
    struct {
      GLuint position, color;
    } attributes;

  } head;

  struct {
    struct {
      GLuint rotation, translation;
      GLuint tail_length, color;
    } uniforms;
    struct {
      GLuint position, index;
    } attributes;
  } tail;

  double xpos, ypos;

  vec3 rotation;
  vec3 translation;
  mouse_button button;

  bool pause;

} g_gl_state;

int colors[] = {
  0x8d, 0xd3, 0xc7,
  0xff, 0xff, 0xb3,
  0xbe, 0xba, 0xda,
  0xfb, 0x80, 0x72,
  0x80, 0xb1, 0xd3,
  0xfd, 0xb4, 0x62,
  0xb3, 0xde, 0x69,
  0xfc, 0xcd, 0xe5,
  0xd9, 0xd9, 0xd9,
  0xbc, 0x80, 0xbd,
  0xcc, 0xeb, 0xc5,
  0xff, 0xed, 0x6f,
  0xff, 0xff, 0xff
};


static float position[3*COUNT];

vec3 tail[TAIL_LENGTH*COUNT];
GLuint tail_index[TAIL_LENGTH*COUNT];
int tail_indices[COUNT];

/* static void update_timer(void) { */
/*   int milliseconds = glfwGetTime() * 1000; */
/*   g_gl_state.timer = (float)milliseconds * 0.001f; */
/* } */


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

  g_gl_state.tail_index_buffer = make_buffer(GL_ELEMENT_ARRAY_BUFFER,
                                             tail_index,
                                             sizeof(tail_index));

  g_gl_state.colors_buffer = make_buffer(GL_ARRAY_BUFFER,
                                         colors,
                                         sizeof(colors));
  /* Compile GLSL program  */
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
  g_gl_state.head.attributes.position =
    glGetAttribLocation(g_gl_state.head_program, "position");
  g_gl_state.head.attributes.color =
    glGetAttribLocation(g_gl_state.head_program, "color");

  g_gl_state.tail.attributes.position =
    glGetAttribLocation(g_gl_state.tail_program, "position");

  g_gl_state.head.uniforms.rotation =
    glGetUniformLocation(g_gl_state.head_program, "rotation");
  g_gl_state.head.uniforms.translation =
    glGetUniformLocation(g_gl_state.head_program, "translation");

  g_gl_state.tail.uniforms.rotation =
    glGetUniformLocation(g_gl_state.tail_program, "rotation");
    g_gl_state.tail.uniforms.translation =
    glGetUniformLocation(g_gl_state.tail_program, "translation");
  g_gl_state.tail.uniforms.tail_length =
    glGetUniformLocation(g_gl_state.tail_program, "tail_length");
  g_gl_state.tail.uniforms.color =
    glGetUniformLocation(g_gl_state.tail_program, "color");

  return 1;
}

static void pick_color(int i, float *color) {

  for (int c = 0; c < 3; c++) {
    color[c] = colors[3*i + c] / 255.0;
  }
}

static void
render(GLFWwindow *window) {
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(g_gl_state.tail_program);
  glUniform3f(g_gl_state.tail.uniforms.rotation,
              g_gl_state.rotation.x,
              g_gl_state.rotation.y,
              g_gl_state.rotation.z);
  glUniform3f(g_gl_state.tail.uniforms.translation,
              g_gl_state.translation.x,
              g_gl_state.translation.y,
              g_gl_state.translation.z);

  glUniform1f(g_gl_state.tail.uniforms.tail_length, TAIL_LENGTH);

  glEnableVertexAttribArray(g_gl_state.tail.attributes.position);
  glVertexAttribPointer(g_gl_state.tail.attributes.position,
                        3, GL_FLOAT, GL_FALSE,
                        3*sizeof(float), 0);
  glBindBuffer(GL_ARRAY_BUFFER, g_gl_state.tail_index_buffer);
  glVertexAttribPointer(g_gl_state.tail.attributes.index,
                        1, GL_FLOAT, GL_FALSE,
                        sizeof(float), 0);

  glBindBuffer(GL_ARRAY_BUFFER, g_gl_state.tail_vertex_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_gl_state.tail_index_buffer);
  glVertexAttribPointer(g_gl_state.tail.attributes.position,
                        3, GL_FLOAT, GL_FALSE,
                        3*sizeof(float), 0);

  for (int c = 0; c < COUNT; c++) {
    int offset = c * TAIL_LENGTH;
    float color[3];
    pick_color(c, (float *)&color);
    glUniform3fv(g_gl_state.tail.uniforms.color, 1, color);

    /* Shift index to avoid creating a closed loop. */
    tail_index[offset] = tail_indices[c];
    for (int i = 1; i < TAIL_LENGTH; i++) {
      int curr = tail_index[offset+i-1] + 1;
      if (curr == (c+1)*TAIL_LENGTH) {
        curr = c*TAIL_LENGTH;
      }
      tail_index[offset+i] = curr;
    }

    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(tail_index),
                 tail_index,
                 GL_DYNAMIC_DRAW);

    glDrawElements(GL_LINE_STRIP,
                   TAIL_LENGTH,
                   GL_UNSIGNED_INT,
                   (GLvoid *)(offset*sizeof(GLuint)));

  }

  glUseProgram(g_gl_state.head_program);
  glUniform3f(g_gl_state.head.uniforms.rotation,
              g_gl_state.rotation.x,
              g_gl_state.rotation.y,
              g_gl_state.rotation.z);
  glUniform3f(g_gl_state.head.uniforms.translation,
              g_gl_state.translation.x,
              g_gl_state.translation.y,
              g_gl_state.translation.z);

  glBindBuffer(GL_ARRAY_BUFFER, g_gl_state.colors_buffer);
  glEnableVertexAttribArray(g_gl_state.head.attributes.color);
  glVertexAttribPointer(g_gl_state.head.attributes.color,
                        3, GL_UNSIGNED_INT, GL_FALSE,
                        3*sizeof(float), 0);

  glBindBuffer(GL_ARRAY_BUFFER, g_gl_state.vertex_buffer);
  glEnableVertexAttribArray(g_gl_state.head.attributes.position);
  glVertexAttribPointer(g_gl_state.head.attributes.position,
                        3, GL_FLOAT, GL_FALSE,
                        3*sizeof(float), 0);
  glPointSize(8.0f);
  glDrawArrays(GL_POINTS, 0, COUNT);

  glDisableVertexAttribArray(g_gl_state.tail.attributes.position);
  glDisableVertexAttribArray(g_gl_state.head.attributes.position);
  glDisableVertexAttribArray(g_gl_state.head.attributes.color);

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

  result.x = SIGMA * (state.y - state.x);
  result.y = RHO*state.x - state.y - state.x*state.z;
  result.z = BETA*state.z + state.x*state.y;

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

/* Compute next step of autonomous differential equation. */
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

void
key_callback(GLFWwindow *window, int key,
             int scancode, int action, int mods) {
  if (action == GLFW_RELEASE) {
    switch(key) {
    case GLFW_KEY_W: {
      g_gl_state.rotation.x += 0.01;
    } break;
    case GLFW_KEY_A: {
      g_gl_state.rotation.y -= 0.01;
    } break;
    case GLFW_KEY_S: {
      g_gl_state.rotation.x -= 0.01;
    } break;
    case GLFW_KEY_D: {
      g_gl_state.rotation.y += 0.01;
    } break;
    case GLFW_KEY_Q: {
      g_gl_state.rotation.z += 0.01;
    } break;
    case GLFW_KEY_E: {
      g_gl_state.rotation.z -= 0.01;
    } break;

    case GLFW_KEY_P: {
      g_gl_state.pause = !g_gl_state.pause;
    } break;
    }
  }
}


static void
mouse_button_callback(GLFWwindow *window,
                      int button,
                      int action,
                      int mods) {
  if (action == GLFW_PRESS) {
    switch(button) {
    case GLFW_MOUSE_BUTTON_LEFT: {
      g_gl_state.button = BUTTON_LEFT;
      glfwGetCursorPos(window, &g_gl_state.xpos, &g_gl_state.ypos);
    } break;
    case GLFW_MOUSE_BUTTON_RIGHT: {
      g_gl_state.button = BUTTON_RIGHT;
      glfwGetCursorPos(window, &g_gl_state.xpos, &g_gl_state.ypos);
    } break;
    }
  }

  if (action == GLFW_RELEASE) {
    g_gl_state.button = BUTTON_NONE;
  }

}

static void
cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
  switch (g_gl_state.button) {
  case BUTTON_LEFT: {
    double deltax = xpos - g_gl_state.xpos;
    double deltay = ypos - g_gl_state.ypos;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      g_gl_state.rotation.z += deltax / 8192;
    }
    else {
      g_gl_state.rotation.y += deltax / 8192;
    }
    g_gl_state.rotation.x += deltay / 8192;
  } break;
  case BUTTON_RIGHT: {
    double deltax = xpos - g_gl_state.xpos;
    double deltay = ypos - g_gl_state.ypos;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      g_gl_state.translation.z += deltay / (1<<14);
    }
    else {
      g_gl_state.translation.x += -deltax / (1<<14);
    }
    g_gl_state.translation.y += deltay / (1<<14);
  } break;
  default: return;
  }
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

  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);

  if (gl3wInit() != 0) {
    fprintf(stderr, "GL3W: failed to initialize\n");
    return 1;
  }

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  float dt = 0.005;

  g_gl_state.rotation.x = 1.65f;
  g_gl_state.rotation.y = 3.08f;
  g_gl_state.rotation.z = -0.93f;
  g_gl_state.translation.x = 0.0f;
  g_gl_state.translation.y = 0.075f;
  g_gl_state.translation.z = 1.81f;

  vec3 initial[COUNT] = {{0.0, 1.2, 0.2},
                         {1.0, 0.04, 1.0},
                         {-0.5, -1.0, 0.0},
                         {0.01, 0.6, 0.2},
                         {0.01, -0.5, 0.2}};
  vec3 current[COUNT];
  for (int i = 0; i < COUNT; i++) {
    current[i] = initial[i];
  }

  for (int i = 0; i < TAIL_LENGTH*COUNT; i++) {
    tail_index[i] = i;
    tail[i].x = 0.0f;
    tail[i].y = 0.0f;
    tail[i].z = 0.0f;
  }

  for (int i = 0; i < COUNT; i++) {
    tail_indices[i] = i*TAIL_LENGTH;
  }

  make_resources();

  g_gl_state.pause = false;
  while (!glfwWindowShouldClose(window)) {
    if (!g_gl_state.pause) {
      for (int c = 0; c < COUNT; c++) {
        for (int i = 0; i < STEPS_PER_FRAME; i++) {
          tail[tail_indices[c]] = current[c];
          tail_indices[c] = tail_indices[c]+1;

          if (tail_indices[c] == (c+1)*TAIL_LENGTH) {
            tail_indices[c] = c*TAIL_LENGTH;
          }

          current[c] = rk4(current[c], dt);
        }
        position[3*c + 0] = current[c].x;
        position[3*c + 1] = current[c].y;
        position[3*c + 2] = current[c].z;
      }
    }

    glBindBuffer(GL_ARRAY_BUFFER, g_gl_state.tail_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(tail), tail, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, g_gl_state.vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(position), position, GL_DYNAMIC_DRAW);

    render(window);

    glfwPollEvents();
  }
}
