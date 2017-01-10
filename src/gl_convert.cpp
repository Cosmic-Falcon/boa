#include "gl_convert.h"

namespace boa {

int constrain(int index, int bound) {
	while(index >= bound) index -= bound;
	while(index < 0) index += bound;

	return index;
}

// Get vertices of the region defined by two sequences of points on the top and
// bottom of a polygon.
// NOTE: Modifies the parameters &top_start and &bottom_end (leftmost vertices) to point to
// top_end and bottom_start (rightmost vertices), respectively.
// Returns the indices of the subpolygon in clockwise order.
std::vector<int> get_subpolygon(GLData &data, int &top_start, int top_end, int bottom_start, int &bottom_end) {
	DEBUG("SUBPOLYGON: " << top_start << ", " << top_end << ", " << bottom_start << ", " << bottom_end);
	std::vector<int> subpolygon;

	for(int i = top_start; i != top_end; i = constrain(i + 1, data.num_verts)) {
		subpolygon.push_back(i);
	}
	subpolygon.push_back(top_end);

	for(int i = bottom_start; i != bottom_end; i = constrain(i + 1, data.num_verts)) {
		subpolygon.push_back(i);
	}
	subpolygon.push_back(bottom_end);

	// Move old start and points to the right side
	// TODO: Maybe not the best idea to modify these variables as a side effect
	top_start = top_end;
	bottom_end = bottom_start;

	return subpolygon;
}

// Divide polygon into y-monotone partitions.
// Returns a vector of the partitions ordered from left to right.
std::vector<std::vector<int>> partition(GLData &data) {
	struct Node {
		int index;
		Node *prev;
		Node *next;
		Node *left;
		Node *right;

		Node(int index, Node *prev, Node *next, Node *left, Node *right) {
			this->index = index;
			this->prev = prev;
			this->next = next;
			this->left = left;
			this->right = right;
		}
	};

	auto ltr_compare = [&] (int &lhs, int &rhs) -> bool { return data.vertices[lhs * 3] > data.vertices[rhs * 3]; }; // Multiply by 3 with no offset to get X coordinates
	std::priority_queue<int, std::vector<int>, decltype(ltr_compare)> ltr_order(ltr_compare);

	// Get leftmost and rightmost vertices and order vertices from left-to-right in ltr_order
	// TODO: find left and rightmost indices by getting the top and bottom of the ltr_order priority queue
	int left_index = 0;
	int right_index = 0;
	ltr_order.push(0);
	for(int i = 1; i < data.num_verts; ++i) {
		ltr_order.push(i);
		if(data.vertices[i * 3] < data.vertices[left_index * 3])
			left_index = i;
		else if(data.vertices[i * 3] > data.vertices[right_index * 3])
			right_index = i;
	}

	// Construct linked list of vertices in order of placement in the polygon
	Node *root = new Node(left_index, nullptr, nullptr, nullptr, nullptr);
	Node *rightmost = nullptr;
	Node *current = root;
	for(int i = left_index + 1; i < data.num_verts + left_index; ++i) {
		Node *child = new Node(i % data.num_verts, current, nullptr, nullptr, nullptr);
		current->next = child;
		current = child;

		if(child->index == right_index)
			rightmost = child;
	}
	current->next = root;
	root->prev = current;

	// Add links to left and right vertices in the linked list
	Node *next = root;
	for(int i = 0; i < data.num_verts; ++i) {
		current = next;
		for(int j = next->index; j < ltr_order.top(); ++j)
			next = next->next;
		for(int j = next->index; j > ltr_order.top(); --j)
			next = next->prev;

		current->right = next;
		next->left = current;
		ltr_order.pop();
	}

#ifdef DEBUG_MODE
	Node *node = root;
	std::string ltr_str = "";
	for(int i = 0; i < data.num_verts; ++i) {
		ltr_str += std::to_string(node->index) + " ";
		node = node->right;
	}
	DEBUG("LEFT TO RIGHT: " << ltr_str);
#endif

	// Partition into monotone partitions
	std::vector<std::vector<int>> partitions;
	current = root;

	// The indices to start and end the partition. Move rightward as new partitions are created.
	int start_index = left_index;
	int close_index = constrain(left_index - 1, data.num_verts);

	GLfloat current_x, current_y,
		prev_x, prev_y,
		next_x, next_y,
		left_x, right_x;

	for(int i = 0; i < data.num_verts; ++i) {
		int index = current->index;

		// TODO: See if the check that the vertex is to the right of the leftmost index is necessary
		current_x = data.vertices[index * 3];
		current_y = data.vertices[index * 3 + 1];
		prev_x = data.vertices[constrain(index - 1, data.num_verts) * 3];
		prev_y = data.vertices[constrain(index - 1, data.num_verts) * 3 + 1];
		next_x = data.vertices[constrain(index + 1, data.num_verts) * 3];
		next_y = data.vertices[constrain(index + 1, data.num_verts) * 3 + 1];
		left_x = data.vertices[left_index * 3];
		right_x = data.vertices[right_index * 3];

		if(current_x > left_x && current_x < prev_x && current_x < next_x) {
			GLfloat prev_theta = std::atan2(prev_y - current_y, prev_x - current_x);
			GLfloat next_theta = std::atan2(next_y - current_y, next_x - current_x);
			GLfloat theta = std::atan2(data.vertices[current->left->index * 3 + 1] - current_y, data.vertices[current->left->index * 3] - current_x);

			DEBUG("THETA (previous, current, next): " << prev_theta << ", " << theta << ", " << next_theta);

			if(prev_y > next_y ? (theta > prev_theta || theta < next_theta) : (prev_theta < theta && theta < next_theta)) {
				partitions.push_back(get_subpolygon(data, start_index, std::min(index, current->left->index), std::max(index, current->left->index), close_index));
				DEBUG("Created partitioning diagonal. INDEX | LEFT CONNECTOR: " << index << " | " << current->left->index);
			} else {
				DEBUG("FAILED DIAGONAL INDEX | LEFT CONNECTOR: " << index << " | " << current->left->index);
			}
		} else if(current_x < right_x && current_x > prev_x && current_x > next_x) {
			GLfloat prev_theta = std::atan2(prev_y - current_y, prev_x - current_x);
			GLfloat next_theta = std::atan2(next_y - current_y, next_x - current_x);
			GLfloat theta = std::atan2(data.vertices[current->right->index * 3 + 1] - current_y, data.vertices[current->right->index * 3] - current_x);

			DEBUG("THETA (previous, current, next): " << prev_theta << ", " << theta << ", " << next_theta);

			if(prev_y > next_y ? (theta > prev_theta || theta < next_theta) : (prev_theta < theta && theta < next_theta)) {
				partitions.push_back(get_subpolygon(data, start_index, std::min(index, current->right->index), std::max(index, current->right->index), close_index));
				DEBUG("Created partitioning diagonal. INDEX | RIGHT CONNECTOR: " << index << " | " << current->right->index);
			} else {
				DEBUG("FAILED DIAGONAL INDEX | RIGHT CONNECTOR: " << index << " | " << current->right->index);
			}
		}

		if(current == rightmost)
			break;
		else
			current = current->right;
	}

	partitions.push_back(get_subpolygon(data, start_index, right_index, constrain(right_index + 1, data.num_verts), close_index)); // Create and add partition with remaining vertices

#ifdef DEBUG_MODE
	DEBUG("Created " << partitions.size() << " partitions:");
	for(auto partition : partitions) {
		for(int i = 0; i < partition.size(); ++i) {
			std::cout << partition[i] << " ";
		}
		std::cout << std::endl;
	}
#endif

	return partitions;
}

// Divide a y-monotone polygon partition into triangles.
void triangulate(GLData &data, std::vector<int> indices, int &start_index, int &indices_index) {
#ifdef DEBUG_MODE
	std::string vertex_string = "";
	for(int i = 0; i < data.num_verts; ++i) {
		vertex_string += std::to_string(indices[i]) + " ";
	}
	DEBUG("Triangulating " + vertex_string);
#endif

	int left_index = 0; // Index of the leftmost point (start point)
	int right_index = 0; // Index of the rightmost point (end point)

	// TODO: check if these are always the first and last vertices
	for(int i = 0; i < data.num_verts; ++i) {
		// Find leftmost and rightmost vertices
		if(data.vertices[indices[i] * 3] < data.vertices[indices[left_index] * 3])
			left_index = i;
		else if(data.vertices[indices[i] * 3] > data.vertices[indices[right_index] * 3])
			right_index = i;
	}

	int a = left_index; // Index of top vertex
	int b = left_index; // Index of bottom vertex
	int current = left_index; // Vertex being analyzed
	int last = left_index; // Last vertex to be analyzed

	std::vector<int> remaining_vertices; // Vertices that have been analyzed but not triangulated. Used for creating fans.

	// Sweep through vertices from left to right and triangulate each monotone polygon
	for(int i = 0; i < data.num_verts; ++i) {
		// Move to next vertex to the right to analyze
		if((data.vertices[indices[constrain(a + 1, data.num_verts)] * 3] < data.vertices[indices[constrain(b - 1, data.num_verts)] * 3] || b == right_index) && a != right_index) {
			// Checks if the next top vertex is to the left of the next bottom vertex or bottom vertex is the rightmost vertex. Evaluates to false if the top vertex is the rightmost vertex.
			// Moves the top vertex rightwards (clockwise)
			a = constrain(a + 1, data.num_verts);
			last = current;
			current = a;
		} else {
			// Moves the bottom vertex rightwards (counterclockwise)
			b = constrain(b - 1, data.num_verts);
			last = current;
			current = b;
		}

		remaining_vertices.push_back(last);

		/* If the next vertex is on the bottom half of the polygon and the previous
		 * vertices that don't form triangles are on the top half (or vice-versa),
		 * then the current point and all of the previous points will form a series
		 * of triangles shaped similarly to a chinese fan. If a fan of triangles is
		 * formed, add each of the triangles in the fan.
		 */
		DEBUG(indices[a] << ", " << indices[b] << ", " << indices[left_index] << " | " << indices_index << ", " << ((data.num_verts - 2) * 3));
		if(indices_index < (data.num_verts - 2) * 3 && a != left_index && b != left_index) { // On top half and neither top or bottom vertices are the leftmost vertex
			if((current == a && current - 1 != last) || (current == b && current + 1 != last)) { // Last vertex was not on same half of the partition as the current vertex
#ifdef DEBUG_MODE
				DEBUG("Fan");
				std::string str = "";
				for(auto i : remaining_vertices) str += std::to_string(indices[i]) + " ";
				DEBUG("Remaining vertices: " << str);
#endif

				int num_remaining_vertices = remaining_vertices.size();
				for(int j = 0; j < num_remaining_vertices - 1; ++j) {
					// Add vertices in clockwise order
					int vert_a = remaining_vertices.front();
					remaining_vertices.erase(remaining_vertices.begin());
					int vert_b = remaining_vertices.front();
					if(current == vert_a || current == vert_b) {
						// TODO: Figure out why this is needed. Shouldn't the current vertex not be in the remaining vertices?
						DEBUG("ABORTING FAN: " << indices[current] << ", " << indices[vert_a] << ", " << indices[vert_b]);
						break;
					}

					// Add triangle of fan to indices
					data.indices[indices_index++] = indices[current];
					if(data.vertices[indices[vert_a] * 3 + 1] < data.vertices[indices[vert_b] * 3 + 1]) { // If first vertex is to the left of the second
						data.indices[indices_index++] = indices[vert_a];
						data.indices[indices_index++] = indices[vert_b];
					} else {
						data.indices[indices_index++] = indices[vert_b];
						data.indices[indices_index++] = indices[vert_a];
					}

					DEBUG("Resultant indices: " << data.indices[indices_index - 3] << ", " << data.indices[indices_index - 2] << ", " << data.indices[indices_index - 1]);
				}
#ifdef DEBUG_MODE
				str = "";
				for(auto i : remaining_vertices) str += std::to_string(indices[i]) + " ";
				DEBUG("Remaining vertices: " << str);

				std::cout << std::endl;
#endif
			} else { // If the last vertex was on the same half as the current one and a fan is not formed, check if a triangular ear is formed
				// TODO: see if this if statement can be simplified
				if(std::find(remaining_vertices.begin(), remaining_vertices.end(), constrain(current - 2, data.num_verts)) != remaining_vertices.end()) {
					// TODO: replace current - 1 with last
					double old_slope = double(data.vertices[indices[constrain(current - 2, data.num_verts)] * 3 + 1] - data.vertices[indices[constrain(current - 1, data.num_verts)] * 3 + 1]) /
						double(data.vertices[indices[constrain(current - 2, data.num_verts)] * 3] - data.vertices[indices[constrain(current - 1, data.num_verts)] * 3]);
					double new_slope = double(data.vertices[indices[constrain(current - 2, data.num_verts)] * 3 + 1] - data.vertices[indices[constrain(current, data.num_verts)] * 3 + 1]) /
						double(data.vertices[indices[constrain(current - 2, data.num_verts)] * 3] - data.vertices[indices[constrain(current, data.num_verts)] * 3]);

					if(current == a && new_slope < old_slope) { // Ear on top
						DEBUG("Upper ear around " << indices[current]);
						data.indices[indices_index++] = indices[current];
						data.indices[indices_index + 2] = indices[remaining_vertices.back()];
						remaining_vertices.pop_back();
						data.indices[indices_index++] = indices[remaining_vertices.back()]; // TODO: this may not necessarily be current - 2
						++indices_index;
						DEBUG("Resultant indices: " << data.indices[indices_index - 3] << ", " << data.indices[indices_index - 2] << ", " << data.indices[indices_index - 1] << std::endl);
					} else if(current == b && new_slope > old_slope) { // Ear on bottom
						DEBUG("Lower ear around " << indices[current]);
						data.indices[indices_index++] = indices[current];
						data.indices[indices_index++] = indices[remaining_vertices.back()];
						remaining_vertices.pop_back();
						data.indices[indices_index++] = indices[remaining_vertices.back()];
						DEBUG("Resultant indices: " << data.indices[indices_index - 3] << ", " << data.indices[indices_index - 2] << ", " << data.indices[indices_index - 1] << std::endl);
					}
				}
			}
		}
	}

#ifdef DEBUG_MODE
	std::string indices_str = "";
	for(int i = 0; i < (data.num_verts - 2) * 3; ++i) {
		indices_str += "(";
		indices_str += std::to_string(data.indices[start_index + i]) + ", ";
		indices_str += std::to_string(data.indices[start_index + (++i)]) + ", ";
		indices_str += std::to_string(data.indices[start_index + (++i)]);
		indices_str += ") ";
	}
	DEBUG("Partition indices: " << indices_str << std::endl);
#endif

	start_index += data.num_verts;
}

GLData gen_gl_data(Vertices vertices) {
	GLData data;
	data.num_verts = vertices.size();
	data.num_elements = (vertices.size() - 2) * 3;

	data.vertices = new GLfloat[data.num_verts * 3];
	for(int i = 0; i < data.num_verts; ++i) {
		// Format vertices for OpenGL
		data.vertices[(i)* 3] = vertices[i].x;
		data.vertices[(i)* 3 + 1] = vertices[i].y;
		data.vertices[(i)* 3 + 2] = 0.0f;
	}

	data.indices = new GLuint[data.num_elements];
	int indices_index = 0; // Index of gl_indices to add to
	int index = 0;

	// Divide polygon into y-monotone partitions and triangulate each partition
	std::vector<std::vector<int>> partitions = partition(data);
	for(std::vector<int> indices : partitions) {
		triangulate(data, indices, index, indices_index);
	}

	data.verts_size = sizeof(GLfloat) * data.num_verts * 3;
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

} // boa
