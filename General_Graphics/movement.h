#pragma once
#include <glm/glm.hpp>

struct rotate {
  float angle;
  glm::vec3 r_vec;
};

struct move {
	rotate rotation;     //{angle, vecx, vecy, vecz}
	glm::vec3 translation;  //{x, y, z}
};
