# Lorenz System

Interactive demo of a chaotic [Lorenz system][wikipedia]. 

- Uses RK4 integration to solve the system numerically.
- OpenGL with GLFW and gl3w for rendering

**Youtube video:** https://www.youtube.com/watch?v=3YdTHaBjJGo

## Usage

Run `make` to build, then run `./lorenz`.

## Controls

Click and drag to look around the system. Right-click and drag to
re-position the system.

## TODO
- Get rid of code duplication of 3D math-related helper functions in
  the head/tail vertex shaders.
- Improve controls. Right now, it does not feel very intuitive.

[wikipedia]: https://en.wikipedia.org/wiki/Lorenz_system
