#ifndef GL_CONVERT_H
#define GL_CONVERT_H

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <queue>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "boa_global.h"

namespace boa {

using Vertices = std::vector<glm::vec4>;

struct GLData {
	GLfloat *vertices;
	GLuint *indices;

	int num_verts;
	int num_elements;
	int verts_size;
	int indices_size;
};

GLData gen_gl_data(Vertices vertices, std::vector<glm::vec3> colors);

} // boa

#endif // GL_CONVERT_H
