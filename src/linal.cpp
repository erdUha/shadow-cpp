#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace linal {

  struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
  };

  struct Triangle {
    struct Vertex v1, v2, v3;
  };

  struct Light {
    glm::vec3 pos;
    float intensity;

    glm::mat4 view;
    glm::mat4 proj;
    int sm_width;
    int sm_height;
    float** shadow_map;
  };
}
