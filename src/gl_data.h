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
	template<typename T> requires requires (T t) {t.length()} -> std::size_t;
	static std::size_t get_num_elements(T t) return t.length();

	template<typename T> requires requires (T t) {t.size()} -> std::size_t;
	static std::size_t get_num_elements(T t) return t.size();

	static const std::vector<std::vector<int>> partition(const GLData &data);
	static void triangulate(GLData &data, std::vector<int> indices, int &start_index, int &indices_index);
public:
	GLfloat *vertices;
	GLuint *indices;

	int num_verts;
	int num_elements;
	int num_attributes;
	int verts_size;
	int indices_size;

	template<typename T> GLData* add_attribute(T t);

	static GLData gen_gl_data(const Vertices vertices, std::vector<std::vector<AttributeContainer>> attributes) {
		GLData data;
		data.num_verts = vertices.size();
		data.num_elements = (vertices.size() - 2) * 3;

		data.num_attributes = 3;
		for(std::vector<AttributeContainer> attribute : attributes) {
			data.num_attributes += get_num_elements(attribute[0]);
		}
		data.vertices = new GLfloat[data.num_verts * data.num_attributes];

		for(int i = 0; i < data.num_verts; ++i) {
			// Format vertices for OpenGL
			data.vertices[i * data.num_attributes] = vertices[i][0];
			data.vertices[i * data.num_attributes + 1] = vertices[i][1];
			data.vertices[i * data.num_attributes + 2] = 0.0f;

			int attribute_offset = 2;
			for(std::vector<AttributeContainer> attribute : attributes) {
				int num_attr_elements = get_num_elements(attribute[0]);
				for(int j = 0; j < num_attr_elements; ++j) {
					data.vertices[i * data.num_attributes + (++attribute_offset)] = attribute[i][j];
				}
			}
		}

		data.indices = new GLuint[data.num_elements];
		int indices_index = 0; // Index of gl_indices to add to
		int index = 0;

		// Divide polygon into y-monotone partitions and triangulate each partition
		const std::vector<std::vector<int>> partitions = partition(data);
		for(std::vector<int> indices : partitions) {
			triangulate(data, indices, index, indices_index);
		}

		data.verts_size = sizeof(GLfloat) * data.num_verts * data.num_attributes;
		data.indices_size = sizeof(GLint) * data.num_elements;

#ifdef DEBUG_MODE
		std::string indices_str = "";
		for(int i = 0; i < data.num_elements; ++i) {
			indices_str += "(";
			indices_str += std::to_string(data.indices[i]) + ", ";
			indices_str += std::to_string(data.indices[++i]) + ", ";
			indices_str += std::to_string(data.indices[++i]);
			indices_str += ") ";
		}
		DEBUG("Indices: " << indices_str << std::endl);
#endif

		return data;
	}
};

} // namespace boa

#endif // GL_CONVERT_H
