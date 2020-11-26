#pragma once
#include "../../General_Graphics/Pixels.h"
#include "../meshes/static_simple_mesh.h"
#include "../meshes/moving_simple_mesh.h"
#include "../vulkan_general/ubos.h"

#include <glm/glm.hpp>


namespace files {
  namespace simple_static {
    void read_mesh(std::vector<staticSimpleMesh>&);
    void read_texture(std::vector<pixels>&);
    void read_ubo(std::vector<ubo_model>&);
  }

  namespace simple_moving {
    void read_mesh(std::vector<moving_simple_mesh>&);
    void read_texture(std::vector<pixels>&);
    void read_ubo(std::vector<ubo_model>&);
  }

}
