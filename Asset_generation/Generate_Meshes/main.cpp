#include <iostream>
#include <chrono>
#include "./deps/boring_test.h"
#include "./deps/main_writing.h"
#include "./deps/helper.h"
#include "./deps/shapes.h"
#include "./deps/cartesian.h"

#include "./deps/static_mesh.h"
#include "./deps/moving_mesh.h"

int main() {
  const int sphere_points = 20;
  const unsigned test_x = 4, test_y = 3;
  auto start0 = std::chrono::high_resolution_clock::now();
  uint32_t id;

  int test_data[test_x * test_y * 3]{};
  int counter = 0;
  for (unsigned i = 0; i < test_x; i++) {
    for (unsigned j = 0; j < test_y; j++) {
      test_data[counter++] = i;       //x
      test_data[counter++] = j;       //y
      test_data[counter++] = i + j;   //z
    }
  }

  static_mesh cube;
  cube.set_vertices(shapes::cube::vertices(), shapes::cube::uvs());
  cube.set_indices(shapes::cube::indices());
  cube.set_texid(0);
  cube.set_movement(boring::angle1, boring::rvec1, boring::tran1);
  //id = new_static_vertex(shapes::cube::vertices(), shapes::cube::uvs());
  //new_static_indices(shapes::cube::indices());
  //static_mesh_has_texture(id, id);

  static_mesh sphere;
  sphere.set_vertices(shapes::sphere::vertices(sphere_points), shapes::sphere::uvs(sphere_points));
  sphere.set_indices(shapes::sphere::indices(sphere_points));
  sphere.set_texid(1);
  sphere.set_movement(boring::angle2, boring::rvec2, boring::tran2);


  //id = new_static_vertex(shapes::sphere::vertices(sphere_points), shapes::sphere::uvs(sphere_points));
  //new_static_indices(shapes::sphere::indices(sphere_points));
  //static_mesh_has_texture(id, id);

  static_mesh plane;
  plane.set_vertices(cartesian_to_verticies(test_data, test_x, test_y, false), cartesian_to_texture(test_data, test_x, test_y, false));
  plane.set_indices(cartesian_to_indices(test_x, test_y, true));
  plane.set_texid(2);
  plane.set_movement(boring::angle3, boring::rvec3, boring::tran3);

  //id = new_static_vertex(cartesian_to_verticies(test_data, test_x, test_y, false), cartesian_to_texture(test_data, test_x, test_y, false));
  //new_static_indices(cartesian_to_indices(test_x, test_y, true));
  //static_mesh_has_texture(id, id);

  static_mesh sphere2;
  sphere2.set_vertices(shapes::sphere::vertices(5), shapes::sphere::uvs(5));
  sphere2.set_indices(shapes::sphere::indices(5));
  sphere2.set_texid(2);
  sphere2.set_movement(boring::angle4, boring::rvec4, boring::tran4);

  //id = new_static_vertex(shapes::sphere::vertices(5), shapes::sphere::uvs(5));
  //new_static_indices(shapes::sphere::indices(5));
  //static_mesh_has_texture(id, id - 1);

  //boring::create_moving_meshes();

  moving_mesh basic;
  basic.set_vertices(boring::bVertices, boring::bUVs);
  basic.set_indices(boring::m_indices1);
  basic.set_texid(4);
  basic.set_movement(boring::angle5, boring::rvec5, boring::tran5);

  //boring::create_movement();
  auto end0 = std::chrono::high_resolution_clock::now();
  std::cout << "Mesh generation took\t" << std::chrono::duration <double, std::milli>(end0 - start0).count() << "ms" << std::endl;

  exit(EXIT_FAILURE);

  auto start1 = std::chrono::high_resolution_clock::now();
  write_all();
  auto end1 = std::chrono::high_resolution_clock::now();
  std::cout << "File writing took\t" << std::chrono::duration <double, std::milli>(end1 - start1).count() << "ms" << std::endl;


  //testing
  /*const uint32_t size_data = 1e3;
  float data[size_data];
  float results[size_data];

  #pragma omp parallel for
  for (uint32_t i = 0; i < size_data; i++) {
    data[i] = i/100;
  }

  openCl_instance openCl;
  openCl.init();
  openCl.square(data, results, size_data);
  openCl.cleanup();*/


  return 0;
}
