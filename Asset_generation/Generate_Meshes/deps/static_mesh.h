#pragma once
#include "vertex.h"
#include "movement.h"
#include <vector>
#include <cstring>  //for memcpy

struct static_mesh {
  std::vector<Vertex> Vertices;
  std::vector<uint32_t> Indices;
  std::vector<glm::vec3> Normals;
  std::vector<move> Movement;
  bool has_normals = false;
  uint32_t texture_id;

  inline void set_vertices(const std::vector<float> vertexes, const std::vector<float> uvs) {
    if (vertexes.size()/3.0f - uvs.size()/2.0f > 0.0001) {
      std::cerr << "vectex and uv vector sizes are of non compatible sizes" << std::endl;
    }

    if (vertexes.size() % 3 != 0) {
      std::cerr << "vertices size must be a multiple of 3" << std::endl;
    }

    if (uvs.size() % 2 != 0) {
      std::cerr << "uvs size must be a multiple of 2" << std::endl;
    }

    Vertices.resize(vertexes.size()/3); //each vertex has 3 vertices and 2 uvs -- could also be the size of uvs/2

    unsigned j = 0, k = 0;
    for (unsigned i = 0; i < Vertices.size(); i++) {
      Vertices[i] = {{vertexes[j++], vertexes[j++], vertexes[j++]}, {uvs[k++], uvs[k++]}};
    }

  }

  inline void set_indices(const std::vector<uint32_t> indexes) {
    if (indexes.size() % 3 != 0) {
      std::cerr << "indices size must be a multiple of 3" << std::endl;
    }

    Indices = indexes;

  }

  inline void set_texid(const uint32_t id) {
    texture_id = id;
  }

  inline void set_movement(std::vector<float> angles, std::vector<glm::vec3> rvec, std::vector<glm::vec3> trans) {
    if (angles.size() != rvec.size()) {
      std::cerr << "number of angles and the number of rotation vectors must be equal" << std::endl;
    }
    if (angles.size() != trans.size()) {
      std::cerr << "number of angles and the number of translations must be equal" << std::endl;
    }
    if (trans.size() != rvec.size()) {
      std::cerr << "number of translations and the number of rotation vectors must be equal" << std::endl;
    }

    Movement.resize(angles.size());
    for (unsigned i = 0; i < Movement.size(); i++) {
      Movement[i] = { {angles[i] , rvec[i] }, trans[i]};
    }


  }

};
