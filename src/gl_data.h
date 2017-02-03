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

template<typename T> concept bool AttributeContainer() {
	return requires(T t, int i) { {t[i]}; } &&
		(requires(T t) { {t.length()} -> std::size_t; } ||
		requires(T t) { {t.size()} -> std::size_t; });
}

class GLData {
private:
	GLfloat *vertices;
	GLuint *indices;

	int num_verts;
	int num_elements;
	int num_attributes;
	int verts_size;
	int indices_size;

	template<typename T> requires requires (T t) {
		{t.length()} -> std::size_t;
	} static std::size_t get_num_elements(const T t) {
		return t.length();
	}

	template<typename T> requires requires (T t) {
		{t.size()} -> std::size_t;
	} static std::size_t get_num_elements(const T t) {
		return t.size();
	}

	std::vector<std::vector<int>> partition();
	void triangulate(std::vector<int> indices, int &start_index, int &indices_index);
public:
	GLData(const Vertices vertices, const int stride);

	GLfloat *get_vertices();
	GLuint *get_indices();
	int get_num_verts();
	int get_num_elements();
	int get_verts_size();
	int get_indices_size();

	void gen_gl_data(const Vertices &vertices);

	GLData &set_attribute(const int offset, const AttributeContainer attribute) {
		int num_attr_elements = get_num_elements(attribute[0]);
		assert(offset + num_attr_elements <= num_attributes); // Make sure that there is enough space to add the attributes to the OpenGL vertex data

		for(int i = 0; i < num_verts; ++i) {
			for(int j = 0; j < num_attr_elements; ++j) {
				vertices[i * num_attributes + offset + j] = attribute[i][j];
			}
		}

		return *this;
	}
};

} // namespace boa

#endif // GL_CONVERT_H
