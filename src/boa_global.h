#ifndef BOA_GLOBAL_H
#define BOA_GLOBAL_H

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace boa {

// Constants
const float PI = glm::pi<float>();

// Macros
#ifdef DEBUG_MODE
#define DEBUG(msg) std::cout << "DEBUG (" << __FILE__ << ":" << __LINE__ << "): " << msg << std::endl;
#else
#define DEBUG(msg)
#endif

#define ERROR(msg) std::cerr << "ERROR! (" << __FILE__ << ":" << __LINE__ << "): " << msg << std::endl;

} // boa

#endif // BOA_GLOBAL_H
