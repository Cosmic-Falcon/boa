#include "gl_data.h"

namespace boa {

GLData::GLData(const Vertices vertices, const int stride) {
	num_attributes = stride;
	gen_gl_data(vertices);
}

const int constrain(int value, const int bound) {
	while(value >= bound) value -= bound;
	while(value < 0) value += bound;

	return value;
}

const double constrain(double value, const double bound) {
	while(value >= bound) value -= bound;
	while(value < 0) value += bound;

	return value;
}

// Get vertices of the region defined by two sequences of points on the top and
// bottom of a polygon.
// NOTE: Modifies the parameters &top_start and &bottom_end (leftmost vertices) to point to
// top_end and bottom_start (rightmost vertices), respectively.
// Returns the indices of the subpolygon in clockwise order.
// TODO: Redocument
void split_polygon(const std::vector<int> &indices, const int num_verts,
		const int top_start, const int top_end, const int bottom_start, const int bottom_end,
		std::vector<int> &indices_1, std::vector<int> &indices_2) {
	DEBUG("SPLITTING POLYGON: " << indices[top_start] << ", " << indices[top_end] << ", " << indices[bottom_start] << ", " << indices[bottom_end]);

	for(int i = top_start; i != top_end; i = constrain(i + 1, num_verts)) {
		indices_1.push_back(indices[i]);
	}
	indices_1.push_back(indices[top_end]);

	for(int i = bottom_start; i != bottom_end; i = constrain(i + 1, num_verts)) {
		indices_1.push_back(indices[i]);
	}
	indices_1.push_back(indices[bottom_end]);

	for(int i = top_end; i != bottom_start; i = constrain(i + 1, num_verts)) {
		indices_2.push_back(indices[i]);
	}
	indices_2.push_back(indices[bottom_start]);

#ifdef DEBUG_MODE
	std::string indices_1_str = "";
	for(int i : indices_1) indices_1_str += std::to_string(i) + " ";
	DEBUG("\tLeft indices: " << indices_1_str);

	std::string indices_2_str = "";
	for(int i : indices_2) indices_2_str += std::to_string(i) + " ";
	DEBUG("\tRight indices: " << indices_2_str);
#endif
}

// Divide polygon into y-monotone partitions.
// Returns a vector of the partitions ordered from left to right.
std::vector<std::vector<int>> GLData::partition() {
	DEBUG_TITLE("PARTITIONING " << std::to_string(num_verts) << " VERTICES");
	struct Node {
		int indices_index;
		Node *prev;
		Node *next;
		Node *left;
		Node *right;

		Node(int indices_index, Node *prev, Node *next, Node *left, Node *right) {
			this->indices_index = indices_index;
			this->prev = prev;
			this->next = next;
			this->left = left;
			this->right = right;
		}
	};

	// Partition into monotone partitions
	std::vector<std::vector<int>> partitions;

	std::queue<std::vector<int>> subpolygons;
	std::vector<int> all_indices(num_verts);
	for(int i = 0; i < num_verts; ++i) all_indices[i] = i;
	subpolygons.push(all_indices);

	while(!subpolygons.empty()) {
		// Get indices to partition
		const std::vector<int> indices = subpolygons.front();
		const int num_verts = indices.size();
		subpolygons.pop();

		const auto ltr_compare = [&] (int &lhs, int &rhs) -> bool { return vertices[indices[lhs] * num_attributes] > vertices[indices[rhs] * num_attributes]; }; // Multiply by num_attributes with no offset to get X coordinates
		std::priority_queue<int, std::vector<int>, decltype(ltr_compare)> ltr_order(ltr_compare);

		// Get leftmost and rightmost vertices and order vertices from left-to-right in ltr_order
		// TODO: find left and rightmost indices by getting the top and bottom of ltr_order. Will have to change to a set to do so
		int left_indices_index = 0;
		int right_indices_index = 0;

#ifdef DEBUG_MODE
		std::string sub_indices_str = "";
		for(int i = 0; i < num_verts; ++i) sub_indices_str += std::to_string(indices[i]) + " ";
		DEBUG("SUB-INDICES: " << sub_indices_str);
#endif

		for(int i = 0; i < num_verts; ++i) {
			ltr_order.push(i);
			if(vertices[indices[i] * num_attributes] < vertices[indices[left_indices_index] * num_attributes])
				left_indices_index = i;
			else if(vertices[indices[i] * num_attributes] > vertices[indices[right_indices_index] * num_attributes])
				right_indices_index = i;
		}

		// Construct linked list of vertices in order of placement in the polygon, starting with the leftmost vertex
		Node *leftmost = new Node(left_indices_index, nullptr, nullptr, nullptr, nullptr);
		Node *rightmost = nullptr;
		Node *current = leftmost;
		for(int i = left_indices_index + 1; i < num_verts + left_indices_index; ++i) {
			Node *child = new Node(i % num_verts, current, nullptr, nullptr, nullptr);
			current->next = child;
			current = child;

			if(child->indices_index == right_indices_index)
				rightmost = child;
		}
		current->next = leftmost;
		leftmost->prev = current;

		// Add links to left and right vertices in the linked list
		// TODO: Try converting to std::find
		Node *next = leftmost;
		for(int i = 0; i < num_verts; ++i) {
			current = next;
			for(int j = next->indices_index; indices[j] < indices[ltr_order.top()]; ++j)
				next = next->next;
			for(int j = next->indices_index; indices[j] > indices[ltr_order.top()]; --j)
				next = next->prev;

			current->right = next;
			next->left = current;
			ltr_order.pop();
		}

#ifdef DEBUG_MODE
		Node *node = leftmost;
		std::string ltr_str = "";
		for(int i = 0; i < num_verts; ++i) {
			ltr_str += std::to_string(indices[node->indices_index]) + " ";
			node = node->right;
		}
		DEBUG("LEFT TO RIGHT: " << ltr_str);
#endif

		// The indices to start and end the partition. Move rightward as new partitions are created.
		current = leftmost;
		GLfloat current_x, current_y,
			prev_x, prev_y,
			next_x, next_y,
			left_x, right_x;

		bool monotone_partition = true;
		for(int i = 0; i < num_verts; ++i) {
			const int indices_index = current->indices_index;

			// TODO: See if the check that the vertex is to the right of the leftmost index is necessary
			current_x = vertices[indices[indices_index] * num_attributes];
			current_y = vertices[indices[indices_index] * num_attributes + 1];
			prev_x = vertices[indices[constrain(indices_index - 1, num_verts)] * num_attributes];
			prev_y = vertices[indices[constrain(indices_index - 1, num_verts)] * num_attributes + 1];
			next_x = vertices[indices[constrain(indices_index + 1, num_verts)] * num_attributes];
			next_y = vertices[indices[constrain(indices_index + 1, num_verts)] * num_attributes + 1];
			left_x = vertices[indices[left_indices_index] * num_attributes];
			right_x = vertices[indices[right_indices_index] * num_attributes];

			if(current_x > left_x && current_x < prev_x && current_x < next_x) {
				const GLfloat prev_theta = std::atan2(prev_y - current_y, prev_x - current_x);
				const GLfloat next_theta = std::atan2(next_y - current_y, next_x - current_x);
				const GLfloat theta = std::atan2(vertices[indices[current->left->indices_index] * num_attributes + 1] - current_y, vertices[indices[current->left->indices_index] * num_attributes] - current_x);

				if(prev_y > next_y ? (theta > prev_theta || theta < next_theta) : (prev_theta < theta && theta < next_theta)) {
					// TODO: run new iterations
					std::vector<int> indices_1;
					std::vector<int> indices_2;
					split_polygon(indices, num_verts, left_indices_index, std::min(indices_index, current->left->indices_index), std::max(indices_index, current->left->indices_index), constrain(left_indices_index - 1, num_verts), indices_1, indices_2);
					subpolygons.push(indices_1);
					subpolygons.push(indices_2);
					DEBUG("\tCreated partitioning diagonal. (INDEX | LEFT CONNECTOR): " << indices[indices_index] << " | " << indices[current->left->indices_index]);
					monotone_partition = false;
					break;
				} else {
					DEBUG("\tFAILED DIAGONAL (INDEX | LEFT CONNECTOR): " << indices[indices_index] << " | " << indices[current->left->indices_index]);
				}
			} else if(current_x < right_x && current_x > prev_x && current_x > next_x) {
				const GLfloat prev_theta = std::atan2(prev_y - current_y, prev_x - current_x);
				const GLfloat next_theta = std::atan2(next_y - current_y, next_x - current_x);
				const GLfloat theta = std::atan2(vertices[indices[current->right->indices_index] * num_attributes + 1] - current_y, vertices[indices[current->right->indices_index] * num_attributes] - current_x);

				DEBUG("THETA (previous, current, next): " << prev_theta << ", " << theta << ", " << next_theta);

				if(prev_y > next_y ? (theta > prev_theta || theta < next_theta) : (prev_theta < theta && theta < next_theta)) {
					std::vector<int> indices_1;
					std::vector<int> indices_2;
					split_polygon(indices, num_verts, left_indices_index, std::min(indices_index, current->right->indices_index), std::max(indices_index, current->right->indices_index), constrain(left_indices_index - 1, num_verts), indices_1, indices_2);
					subpolygons.push(indices_1);
					subpolygons.push(indices_2);
					DEBUG("\tCreated partitioning diagonal. (INDEX | RIGHT CONNECTOR): " << indices[indices_index] << " | " << indices[current->right->indices_index]);
					monotone_partition = false;
					break;
				} else {
					DEBUG("\tFAILED DIAGONAL (INDEX | RIGHT CONNECTOR): " << indices[indices_index] << " | " << indices[current->right->indices_index]);
				}
			}

			if(current == rightmost)
				break;
			else
				current = current->right;
		}

		if(monotone_partition)
			partitions.push_back(indices);
	}

#ifdef DEBUG_MODE
	DEBUG("Created " << partitions.size() << " partitions:");
	for(auto partition_vertices : partitions) {
		std::string partition_str = "";
		for(int i = 0; i < partition_vertices.size(); ++i) {
			partition_str += std::to_string(partition_vertices[i]) + " ";
		}
		DEBUG("\t" << partition_str);
	}
#endif

	return partitions;
}

// Divide a y-monotone polygon partition into triangles.
void GLData::triangulate(std::vector<int> partition_indices, int &start_index, int &indices_index) {
	const int num_partition_verts = partition_indices.size();

#ifdef DEBUG_MODE
	std::string vertex_string = "";
	for(int i = 0; i < num_partition_verts; ++i) {
		vertex_string += std::to_string(partition_indices[i]) + " ";
	}
	DEBUG_TITLE("TRIANGULATING: " + vertex_string);
#endif

	int left_index = 0; // Index of the leftmost point (start point)
	int right_index = 0; // Index of the rightmost point (end point)

	// TODO: check if these are always the first and last vertices
	for(int i = 1; i < num_partition_verts; ++i) {
		// Find leftmost and rightmost vertices
		if(vertices[partition_indices[i] * num_attributes] < vertices[partition_indices[left_index] * num_attributes])
			left_index = i;
		else if(vertices[partition_indices[i] * num_attributes] > vertices[partition_indices[right_index] * num_attributes])
			right_index = i;
	}

	int top_index = left_index; // Index of top vertex
	int bottom_index = left_index; // Index of bottom vertex
	int current = left_index; // Vertex being analyzed
	int last = left_index; // Last vertex to be analyzed

	std::vector<int> remaining_vertices; // Vertices that have been analyzed but not triangulated. Used for creating fans.

	// Sweep through vertices from left to right and triangulate each monotone polygon
	for(int i = 0; i < num_partition_verts; ++i) {
		// Move to next vertex to the right to analyze
		if((vertices[partition_indices[constrain(top_index + 1, num_partition_verts)] * num_attributes] < vertices[partition_indices[constrain(bottom_index - 1, num_partition_verts)] * num_attributes] || bottom_index == right_index) && top_index != right_index) {
			// Checks if the next top vertex is to the left of the next bottom vertex or bottom vertex is the rightmost vertex. Evaluates to false if the top vertex is the rightmost vertex.
			// Moves the top vertex rightwards (clockwise)
			top_index = constrain(top_index + 1, num_partition_verts);
			last = current;
			current = top_index;
		} else {
			// Moves the bottom vertex rightwards (counterclockwise)
			bottom_index = constrain(bottom_index - 1, num_partition_verts);
			last = current;
			current = bottom_index;
		}

		remaining_vertices.push_back(last);

		/* If the next vertex is on the bottom half of the polygon and the previous
		 * vertices that don't form triangles are on the top half (or vice-versa),
		 * then the current point and all of the previous points will form a series
		 * of triangles shaped similarly to a chinese fan. If a fan of triangles is
		 * formed, add each of the triangles in the fan.
		 */
		DEBUG("Top index, bottom index: " << partition_indices[top_index] << ", " << partition_indices[bottom_index] << " | index " << indices_index << " of " << ((num_verts - 2) * 3));
		if(remaining_vertices.size() > 1 && indices_index < (num_verts - 2) * 3 /*&& (top_index != left_index && bottom_index != left_index)*/) { // On top half and neither top or bottom vertices are the leftmost vertex
			if((current == top_index && current - 1 != last) || (current == bottom_index && current + 1 != last)) { // Last vertex was not on same top/bottom half of the partition as the current vertex
#ifdef DEBUG_MODE
				DEBUG("FAN");
				std::string str = "";
				for(auto i : remaining_vertices) str += std::to_string(partition_indices[i]) + " ";
				DEBUG("\tRemaining vertices: " << str);
#endif

				const int num_vertices_to_add = remaining_vertices.size() - 1;
				for(int j = 0; j < num_vertices_to_add; ++j) {
					// Add vertices in clockwise order
					const int vert_a = remaining_vertices.front();
					remaining_vertices.erase(remaining_vertices.begin());
					const int vert_b = remaining_vertices.front();
					if(current == vert_a || current == vert_b) {
						// TODO: Figure out why this is needed. Shouldn't the current vertex not be in the remaining vertices?
						DEBUG("\tABORTING FAN: " << partition_indices[current] << ", " << partition_indices[vert_a] << ", " << partition_indices[vert_b]);
						break;
					}

					// Add triangle of fan to partition_indices
					indices[indices_index++] = partition_indices[current];
					if(vertices[partition_indices[vert_a] * num_attributes + 1] < vertices[partition_indices[vert_b] * num_attributes + 1]) { // If first vertex is to the left of the second
						indices[indices_index++] = partition_indices[vert_a];
						indices[indices_index++] = partition_indices[vert_b];
					} else {
						indices[indices_index++] = partition_indices[vert_b];
						indices[indices_index++] = partition_indices[vert_a];
					}

					DEBUG("\tResultant indices: " << indices[indices_index - num_attributes] << ", " << indices[indices_index - 2] << ", " << indices[indices_index - 1]);
				}
#ifdef DEBUG_MODE
				str = "";
				for(auto i : remaining_vertices) str += std::to_string(partition_indices[i]) + " ";
				DEBUG("\tRemaining vertices: " << str);
#endif
			} else {
			// If the last vertex was on the same half as the current one and a fan is not formed, check if a triangular ear is formed
				DEBUG("Checking for ear");
				const int prev_prev_index = partition_indices[constrain(remaining_vertices[remaining_vertices.size() - 2], num_partition_verts)];
				const int prev_index = partition_indices[constrain(remaining_vertices.back(), num_partition_verts)];
				const int current_index = partition_indices[constrain(current, num_partition_verts)];
				DEBUG("\tAngle segment: (" << prev_prev_index << ", " << prev_index << ", " << current_index << ")");

				const GLfloat prev_theta = std::atan2(vertices[prev_prev_index * num_attributes + 1] - vertices[prev_index * num_attributes + 1], vertices[prev_prev_index * num_attributes] - vertices[prev_index * num_attributes]);
				const GLfloat current_theta = std::atan2(vertices[current_index * num_attributes + 1] - vertices[prev_index * num_attributes + 1], vertices[current_index * num_attributes] - vertices[prev_index * num_attributes]);

				if(current == top_index) { // Ear on top
					const GLfloat net_theta = constrain(current_theta - prev_theta, 2 * boa::PI);
					DEBUG("\tPrevious theta, current theta, net theta: " << prev_theta << ", " << current_theta << ", " << net_theta);
					if (net_theta < boa::PI) {
						DEBUG("\tUPPER EAR around " << partition_indices[current]);
						indices[indices_index++] = partition_indices[current];
						indices[indices_index + 1] = partition_indices[remaining_vertices.back()];
						remaining_vertices.pop_back();
						indices[indices_index] = partition_indices[remaining_vertices.back()];
						indices_index += 2;
						DEBUG("\tResultant indices: " << indices[indices_index - num_attributes] << ", " << indices[indices_index - 2] << ", " << indices[indices_index - 1]);
					}
				} else if(current == bottom_index) { // Ear on bottom
					const GLfloat net_theta = constrain(prev_theta - current_theta, 2 * boa::PI);
					DEBUG("\tPrevious theta, current theta, net theta: " << prev_theta << ", " << current_theta << ", " << net_theta);
					if (net_theta < boa::PI) {
						DEBUG("\tLOWER EAR around " << partition_indices[current]);
						indices[indices_index++] = partition_indices[current];
						indices[indices_index++] = partition_indices[remaining_vertices.back()];
						remaining_vertices.pop_back();
						indices[indices_index++] = partition_indices[remaining_vertices.back()];
						DEBUG("\tResultant indices: " << indices[indices_index - num_attributes] << ", " << indices[indices_index - 2] << ", " << indices[indices_index - 1]);
					}
				}
			}
		}
	}

#ifdef DEBUG_MODE
	std::string indices_str = "";
	for(int i = 0; i < (num_partition_verts - 2) * 3; ++i) {
		indices_str += "(";
		indices_str += std::to_string(indices[start_index + i]) + ", ";
		indices_str += std::to_string(indices[start_index + (++i)]) + ", ";
		indices_str += std::to_string(indices[start_index + (++i)]);
		indices_str += ") ";
	}
	DEBUG("Partition indices: " << indices_str << std::endl);
#endif

	start_index += num_partition_verts;
}

void GLData::gen_gl_data(const Vertices &raw_vertices) {
	num_verts = raw_vertices.size();
	num_elements = (raw_vertices.size() - 2) * 3;

	vertices = new GLfloat[num_verts * num_attributes];

	for(int i = 0; i < num_verts; ++i) {
		// Format vertices for OpenGL
		vertices[i * num_attributes] = raw_vertices[i][0];
		vertices[i * num_attributes + 1] = raw_vertices[i][1];
		vertices[i * num_attributes + 2] = 0.0f;
	}

	indices = new GLuint[num_elements];
	int indices_index = 0; // Index of gl_indices to add to
	int index = 0;

	// Divide polygon into y-monotone partitions and triangulate each partition
	const std::vector<std::vector<int>> partitions = partition();
	for(const std::vector<int> indices : partitions) {
		triangulate(indices, index, indices_index);
	}

	verts_size = sizeof(GLfloat) * num_verts * num_attributes;
	indices_size = sizeof(GLint) * num_elements;

#ifdef DEBUG_MODE
	std::string indices_str = "";
	for(int i = 0; i < num_elements; ++i) {
		indices_str += "(";
		indices_str += std::to_string(indices[i]) + ", ";
		indices_str += std::to_string(indices[++i]) + ", ";
		indices_str += std::to_string(indices[++i]);
		indices_str += ") ";
	}
	DEBUG("Indices: " << indices_str << std::endl);
#endif
}

GLfloat *GLData::get_vertices() { return vertices; }
GLuint *GLData::get_indices() { return indices; }
int GLData::get_num_verts() { return num_verts; }
int GLData::get_num_elements() { return num_elements; }
int GLData::get_verts_size() { return verts_size; }
int GLData::get_indices_size() { return indices_size; }

} // namespace boa
