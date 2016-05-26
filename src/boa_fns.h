#ifndef BOA_FNS_H
#define BOA_FNS_H

#include <fstream>
#include <initializer_list>
#include <iostream>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SOIL.h>

#include "global.h"

namespace boa {

// General
bool init(GLint version_major, GLint version_minor, GLboolean resizable);

// Shaders
GLuint compile_shader(const char* source, GLenum type);
GLuint create_program(std::initializer_list<GLuint> shaders);

// Textures
GLuint load_texture(const char* source);
void set_texture(GLenum target, GLenum unit, GLuint texture);

// Windows
GLFWwindow* create_window(int width, int height, std::string name, GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr);

} // boa

#endif // BOA_FNS_H
