#pragma once
#include "static_mesh.h"

struct moving_mesh : public static_mesh {
  std::vector<std::vector<Vertex>> Vertices;
  inline void set_vertices(const std::vector<std::vector<float>> vertexes, const std::vector<std::vector<float>> uvs) {
    if (vertexes.size() != uvs.size()) {
      std::cerr << "different number of elements in vertex and uv array" << std::endl;
    }
    Vertices.resize(vertexes.size());

    for (unsigned l = 0; l < Vertices.size(); l++){

      if (vertexes[l].size()/3.0f - uvs[l].size()/2.0f > 0.0001) {
        std::cerr << "vectex and uv vector sizes are of non compatible sizes" << std::endl;
      }

      if (vertexes[l].size() % 3 != 0) {
        std::cerr << "vertices size must be a multiple of 3" << std::endl;
      }

      if (uvs[l].size() % 2 != 0) {
        std::cerr << "uvs size must be a multiple of 2" << std::endl;
      }

      Vertices[l].resize(vertexes[l].size()/3); //each vertex has 3 vertices and 2 uvs -- could also be the size of uvs/2

      unsigned j = 0, k = 0;
      for (unsigned i = 0; i < Vertices.size(); i++) {
        Vertices[l][i] = {{vertexes[l][j++], vertexes[l][j++], vertexes[l][j++]}, {uvs[l][k++], uvs[l][k++]}};
      }

    }
  }


};
