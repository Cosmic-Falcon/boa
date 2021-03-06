#include <iostream>

#include <adder/adder.h>
#include <boa/boa.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

bool keys[1024];

// Camera
GLfloat camera_x = 0;
GLfloat camera_y = 0;


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode); // Register key presses
void key_parse(); // Act on key presses

int main() {
	adder::Polygon poly({{0, 0}, {144, 0}, {144, -72}, {72, -72}, {120, -48}, {48, -12}, {24, -24}, {72, -48}, {24 ,-72}, {0, -72}}, {256, 128});
	std::vector<glm::vec3> colors = {{0.2, 0.0, 0.8}, {0.2, 0.0, 0.8}, {1.0, 0.0, 0.8}, {1.0, 1.0, 0.0}, {1.0, 0.0, 0.8}, {1.0, 0.0, 0.8}, {1.0, 0.0, 0.8}, {1.0, 0.0, 0.8}, {1.0, 0.0, 0.8}, {1.0, 0.0, 0.8}};
	//poly.rotate(2*adder::PI/3, poly.get_pos());
	adder::Body body(100, 100, -.1, poly);

	boa::GLData poly_gl_data(poly.vertices(), 6);
	poly_gl_data.set_attribute(3, colors);

	boa::init(3, 3, GL_FALSE);
	GLFWwindow* window = boa::create_window(640, 480, "BOA TEST");
	glfwSetKeyCallback(window, key_callback);

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.2, 0.5, 1.0, 0.0);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Outline mode

	GLuint vertex_shader = boa::compile_shader("res/shaders/shader.vert", GL_VERTEX_SHADER);
	GLuint fragment_shader = boa::compile_shader("res/shaders/shader.frag", GL_FRAGMENT_SHADER);
	GLuint shader_program = boa::create_program({vertex_shader, fragment_shader});
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	
	// Create vertex array object
	// The vertex array object stores the states of the vertex and index buffer
	// objects defined inside of it, allowing them to be easily reused again.
	// The vertex and index buffer objects store the vertex and index data on
	// the gpu, respectively. They are cleared automatically after each drawing
	// cycle, hence the need for the vertex array object. I think.
	// TODO: find some way to abstract this
	GLuint vao, vbo, ibo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ibo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, poly_gl_data.get_verts_size(), poly_gl_data.get_vertices(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, poly_gl_data.get_indices_size(), poly_gl_data.get_indices(), GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	
	// Transformation matrices
	glm::mat4 model, view, projection;
	projection = glm::ortho(0.0f, 640.0f, 480.0f, 0.0f);


	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		key_parse();

		glUseProgram(shader_program);

		// Update view matrix with new camera position
		view = glm::translate(glm::mat4(), glm::vec3(camera_x, camera_y, 0.0f));
		// Load uniforms
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(vao);	

		// Render
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		model = glm::mat4();
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, glm::value_ptr(model));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glDrawElements(GL_TRIANGLES, poly_gl_data.get_num_elements(), GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	// Close when escape is pressed
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if(action == GLFW_PRESS) {
		keys[key] = true;
	} else if(action == GLFW_RELEASE) {
		keys[key] = false;
	}
}

void key_parse() {
	if(keys[GLFW_KEY_W]) {
		camera_y += 4;
	} if(keys[GLFW_KEY_A]) {
		camera_x += 4;
	} if(keys[GLFW_KEY_S]) {
		camera_y -= 4;
	} if(keys[GLFW_KEY_D]) {
		camera_x -= 4;
	}
}
