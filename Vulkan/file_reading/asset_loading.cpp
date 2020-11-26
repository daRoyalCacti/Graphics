#include "asset_loading.h"
#include "../global.h"
#include "../../General_Graphics/movement.h"

#include <stdexcept>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>   //for std::unqiue_ptr

namespace files {
  const std::string mesh_dir = global::asset_dir + std::string("/Meshes/test");
  const std::string tex_dir = global::asset_dir + std::string("/textures");
  std::mutex mtx;
  namespace simple_static {

    void read_mesh(std::vector<staticSimpleMesh> &mesh) {
      #ifdef detailed_timing
        auto start = std::chrono::steady_clock::now();
      #endif

      unsigned data_first;  //for resizing arrays and such
      //reading said data
      std::ifstream data_file(mesh_dir + "/data_s_mesh.bin", std::ios::binary);
      data_file.read((char*)&data_first, sizeof(unsigned) );
      //data_file_first.close();
      const unsigned no_mesh = data_first;
      const unsigned no_data = data_first * 2; //number of verticies and indicies not including the first line

      std::unique_ptr<unsigned[]> reading_data = std::make_unique<unsigned[]>(no_data); //variable to be pointed to that contains no vertices and indices for all meshes

      data_file.read((char*)&reading_data[0], no_data * sizeof(unsigned));
      data_file.close();

      mesh.resize(no_mesh);

      unsigned *data_ptr = &reading_data[0];

      for (unsigned k = 0; k < no_mesh; k++) {
        mesh[k].vertices.resize(*data_ptr++);
        mesh[k].indices.resize(*data_ptr++);

        std::ifstream vertices_file(mesh_dir + "/vertex_simple_static_" + std::to_string(k) + ".bin", std::ios::binary);
        vertices_file.read((char*)&mesh[k].vertices[0], sizeof(Vertex) * mesh[k].vertices.size());
        vertices_file.close();

        std::ifstream indices_file(mesh_dir + "/indices_simple_static_" + std::to_string(k) + ".bin", std::ios::binary);
        indices_file.read((char*)&mesh[k].indices[0], sizeof(uint32_t) * mesh[k].indices.size());
        indices_file.close();
      }


      #ifdef detailed_timing
        auto end = std::chrono::steady_clock::now();
        mtx.lock();
        std::cout << "\t Mesh reading took \t\t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
        mtx.unlock();
      #endif
    }



    void read_texture(std::vector<pixels> &tex) {
      #ifdef detailed_timing
      	auto start = std::chrono::steady_clock::now();
      #endif

      const std::string file_title = "textures_s_";
      const std::string file_extension = ".bin";

      uint32_t no_image;
      std::ifstream data_file_first(std::string(global::asset_dir)+"/textures/"+file_title+ "data" + file_extension, std::ios::binary);
      if (!data_file_first.is_open()) {
        throw std::runtime_error("Can not open file " + std::string(global::asset_dir)+"/textures/"+file_title+ "data" + file_extension);
      }
      data_file_first.read((char*)&no_image, sizeof(no_image) ); data_file_first.close();

      tex.resize(no_image);
      //std::vector<uint32_t> image_dimensions;
      //image_dimensions.resize(no_image * 2);

      std::unique_ptr<uint32_t[]> alldata = std::make_unique<uint32_t[]>(no_image * 2 + 1);  //sizes consist of width and height, 1 for intial reading
      std::ifstream data_file(std::string(global::asset_dir)+"/textures/"+file_title+ "data" + file_extension, std::ios::binary); data_file.read((char*)&alldata[0], (no_image * 2 + 1) * sizeof(uint32_t) ); data_file.close();

      uint32_t* data_ptr = &alldata[1];

      unsigned counter = 0;
      for (unsigned i = 0; i < no_image; i++) {
        tex[i].texWidth = *data_ptr++;
        tex[i].texHeight = *data_ptr++;
        /*tex[i].resize(imageWidth * imageHeight * 4);
        image_dimensions[counter++] = imageWidth;
        image_dimensions[counter++] = imageHeight;*/
        tex[i].pixels.resize(tex[i].texWidth * tex[i].texHeight * 4);

        std::ifstream image_file(std::string(global::asset_dir)+"/textures/" + file_title+ std::to_string(i) + file_extension, std::ios::binary);
        if (!image_file.is_open()) {
          throw std::runtime_error("Can not open file " + std::string(global::asset_dir)+"/textures/" + file_title+ std::to_string(i) + file_extension);
        }
        image_file.read((char*)&tex[i].pixels[0], sizeof(tex[i].pixels[0]) * tex[i].pixels.size() ); image_file.close();
      }

      /*uint32_t first_data;
      std::ifstream t_data_file_first(std::string(global::asset_dir)+"/Meshes/t_data" + file_extension, std::ios::binary); t_data_file_first.read((char*)&first_data, sizeof(uint32_t) ); t_data_file_first.close();

      image_ids.resize(first_data + 1);
      std::ifstream t_data_file(std::string(global::asset_dir)+"/Meshes/t_data" + file_extension, std::ios::binary); t_data_file.read((char*)&image_ids[0], (first_data + 1) * sizeof(uint32_t) ); t_data_file.close();
      image_ids.erase(image_ids.begin()); //it needs all the data without the first element. So just removing the first element
      */

      alldata.reset(0);

      #ifdef detailed_timing
      	auto end = std::chrono::steady_clock::now();
        mtx.lock();
      	std::cout << "\t Static mesh texture reading took \t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
        mtx.unlock();
      #endif
    }




    void read_ubo(std::vector<ubo_model> &movement) {
      #ifdef detailed_timing
        auto start = std::chrono::steady_clock::now();
      #endif

      const std::string file_extension = ".bin";
      const int rotation_elements = 4, translation_elements = 3;
      const std::string data_file_n = "/data_s_ubo";

      unsigned no_ubo_mesh;
      std::ifstream data_file_first(std::string(mesh_dir)+data_file_n + file_extension, std::ios::binary); data_file_first.read((char*)&no_ubo_mesh, sizeof(unsigned) ); data_file_first.close();

      std::unique_ptr<unsigned[]> alldata = std::make_unique<unsigned[]>(no_ubo_mesh * 3 + 1);

      //no_ubo_mesh = test_reading_data_first;
      //if (no_ubo_mesh != (no_mesh + no_m_mesh)) throw std::runtime_error("number of ubo model matrices does not equal the number of meshes");
      movement.resize(no_ubo_mesh);

      std::ifstream data_file(std::string(mesh_dir)+data_file_n + file_extension, std::ios::binary); data_file.read((char*)&alldata[0], (no_ubo_mesh * 3 + 1) * sizeof(unsigned) ); data_file.close();
      unsigned* testptr = &alldata[1];

      std::unique_ptr<unsigned[]> no_frames = std::make_unique<unsigned[]>(no_ubo_mesh);


      std::vector<float> reading_rotations;
      std::vector<float> reading_translation;

      float *rotations_ptr, *translations_ptr;

      for (size_t i = 0; i < static_cast<size_t>(no_ubo_mesh); i++) { //for each mesh
        no_frames[i] = *testptr++;
        movement[i].models.resize(no_frames[i]);

        reading_rotations.resize(rotation_elements);
        reading_translation.resize(translation_elements);

        float rotation_angle;
        glm::vec3 rotation_vector, translation_vector;

        for (size_t j = 0; j < static_cast<size_t>(no_frames[i]); j++) { //for every frame of movement for a given mesh
          //reading the rotations for every frame
          std::ifstream rotations_file(static_cast<std::string>(std::string(global::asset_dir)+"/Meshes/rotations_") + std::to_string(i) + "_" + std::to_string(j) + file_extension, std::ios::binary);
          if (!rotations_file.is_open()) {throw std::runtime_error(static_cast<std::string>("failed to open rotations file ") + std::to_string(i) + file_extension);}
          rotations_file.read((char*)&reading_rotations[0], no_frames[i] * rotation_elements * sizeof(float)); rotations_file.close();


          //reading the translations for every frame
          std::ifstream translations_file(static_cast<std::string>(std::string(global::asset_dir)+"/Meshes/translations_") + std::to_string(i) + "_" + std::to_string(j) + file_extension, std::ios::binary);
          if (!translations_file.is_open()) {throw std::runtime_error(static_cast<std::string>("failed to open translations file ") + std::to_string(i) + file_extension);}
          translations_file.read((char*)&reading_translation[0], no_frames[i] * translation_elements * sizeof(float) ); translations_file.close();

          rotations_ptr = &reading_rotations[0];
          translations_ptr = &reading_translation[0];

          //glm doesnt behave nicely if the values are not set in this way
          rotation_angle = *rotations_ptr++;
          rotation_vector.x = *rotations_ptr++;
          rotation_vector.y = *rotations_ptr++;
          rotation_vector.z = *rotations_ptr++;

          translation_vector.x = *translations_ptr++;
          translation_vector.y = *translations_ptr++;
          translation_vector.z = *translations_ptr++;

          if (rotation_vector == glm::vec3(0.0f, 0.0f, 0.0f)) { //glm::rotate doesnt behave nicely if the rotation axis is the 0 vector
            movement[i].models[j] = glm::translate(glm::mat4(1.0f), translation_vector);
          } else {
            movement[i].models[j] = glm::translate(glm::rotate(glm::mat4(1.0f), rotation_angle, rotation_vector), translation_vector);
          } //end if

        } //end inner loop

      } //end outer loop

      alldata.reset();


      #ifdef detailed_timing
        auto end = std::chrono::steady_clock::now();
        mtx.lock();
        std::cout << "\t Static mesh UBO reading took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
        mtx.unlock();
      #endif
    }

  }

  namespace simple_moving {
    void read_mesh(std::vector<moving_simple_mesh> &mesh) {
      #ifdef detailed_timing
      	auto start = std::chrono::steady_clock::now();
      #endif
      unsigned data_first;  //for resizing arrays and such
      std::ifstream data_file(mesh_dir + "/data_m_mesh.bin", std::ios::binary);
      if (!data_file.is_open()) {
        throw std::runtime_error("Can not open file " + mesh_dir + "/data_m_mesh.bin");
      }
      data_file.read( (char*)&data_first, sizeof(unsigned));

      const unsigned no_mesh = data_first;

      std::unique_ptr<unsigned[]> all_data = std::make_unique<unsigned[]>(no_mesh * 3); //number of vertices, indicies and frames + first value
      mesh.resize(no_mesh);

      data_file.read( (char*)&all_data[0], (no_mesh * 3) * sizeof(unsigned) );
      data_file.close();

      unsigned* data_ptr = &all_data[0];

      for (unsigned i = 0; i < no_mesh; i++) {
        const unsigned no_vertices = *data_ptr++;
        const unsigned no_indices = *data_ptr++;
        const unsigned no_frames = *data_ptr++;

        mesh[i].Meshes.resize(no_frames);
        mesh[i].indices.resize(no_indices);

        //reading indices
        std::ifstream indices_file(mesh_dir + "/indices_simple_moving_" + std::to_string(i) + ".bin", std::ios::binary);
        if (!indices_file.is_open()) {
          throw std::runtime_error("Can not open file " + mesh_dir + "/indices_simple_moving_" + std::to_string(i) + ".bin");
        }
        indices_file.read( (char*)&mesh[i].indices[0], sizeof(mesh[i].indices[0]) * mesh[i].indices.size());
        indices_file.close();

        //reading vertices
        for (unsigned j = 0; j < no_frames; j++) {
          mesh[i].Meshes[j].resize(no_vertices);

          std::ifstream vertices_file(mesh_dir + "/vertex_simple_moving_" + std::to_string(i) + "_" + std::to_string(j) + ".bin", std::ios::binary);
          if (!vertices_file.is_open()) {
            throw std::runtime_error("Can not open file " + mesh_dir + "/vertex_simple_moving_" + std::to_string(i) + "_" + std::to_string(j) + ".bin");
          }
          vertices_file.read( (char*)&mesh[i].Meshes[j][0], no_vertices * 3 * sizeof(Vertex) );
          vertices_file.close();
        }

      }


      #ifdef detailed_timing
      	auto end = std::chrono::steady_clock::now();
        mtx.lock();
      	std::cout << "\t Moving mesh reading took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
        mtx.unlock();
      #endif
    }



    void read_texture(std::vector<pixels> &tex) {
      #ifdef detailed_timing
      	auto start = std::chrono::steady_clock::now();
      #endif

      const std::string file_title = "textures_m_";
      const std::string file_extension = ".bin";

      uint32_t no_image;
      std::ifstream data_file_first(std::string(global::asset_dir)+"/textures/"+file_title+ "data" + file_extension, std::ios::binary);
      if (!data_file_first.is_open()) {
        throw std::runtime_error("Can not open file " + std::string(global::asset_dir)+"/textures/"+file_title+ "data" + file_extension);
      }
      data_file_first.read((char*)&no_image, sizeof(no_image) ); data_file_first.close();

      tex.resize(no_image);
      //std::vector<uint32_t> image_dimensions;
      //image_dimensions.resize(no_image * 2);

      std::unique_ptr<uint32_t[]> alldata = std::make_unique<uint32_t[]>(no_image * 2 + 1);  //sizes consist of width and height, 1 for intial reading
      std::ifstream data_file(std::string(global::asset_dir)+"/textures/"+file_title+ "data" + file_extension, std::ios::binary); data_file.read((char*)&alldata[0], (no_image * 2 + 1) * sizeof(uint32_t) ); data_file.close();

      uint32_t* data_ptr = &alldata[1];

      unsigned counter = 0;
      for (unsigned i = 0; i < no_image; i++) {
        tex[i].texWidth = *data_ptr++;
        tex[i].texHeight = *data_ptr++;
        /*tex[i].resize(imageWidth * imageHeight * 4);
        image_dimensions[counter++] = imageWidth;
        image_dimensions[counter++] = imageHeight;*/
        tex[i].pixels.resize(tex[i].texWidth * tex[i].texHeight * 4);

        std::ifstream image_file(std::string(global::asset_dir)+"/textures/" + file_title+ std::to_string(i) + file_extension, std::ios::binary);
        if (!image_file.is_open()) {
          throw std::runtime_error("Can not open file " + std::string(global::asset_dir)+"/textures/" + file_title+ std::to_string(i) + file_extension);
        }
        image_file.read((char*)&tex[i].pixels[0], sizeof(tex[i].pixels[0]) * tex[i].pixels.size() ); image_file.close();
      }

      /*uint32_t first_data;
      std::ifstream t_data_file_first(std::string(global::asset_dir)+"/Meshes/t_data" + file_extension, std::ios::binary); t_data_file_first.read((char*)&first_data, sizeof(uint32_t) ); t_data_file_first.close();

      image_ids.resize(first_data + 1);
      std::ifstream t_data_file(std::string(global::asset_dir)+"/Meshes/t_data" + file_extension, std::ios::binary); t_data_file.read((char*)&image_ids[0], (first_data + 1) * sizeof(uint32_t) ); t_data_file.close();
      image_ids.erase(image_ids.begin()); //it needs all the data without the first element. So just removing the first element
      */

      alldata.reset(0);

      #ifdef detailed_timing
      	auto end = std::chrono::steady_clock::now();
        mtx.lock();
      	std::cout << "\t Moving mesh texture reading took \t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
        mtx.unlock();
      #endif
    }

    void read_ubo(std::vector<ubo_model> &movement) {
      #ifdef detailed_timing
        auto start = std::chrono::steady_clock::now();
      #endif

      const std::string file_extension = ".bin";
      const int rotation_elements = 4, translation_elements = 3;
      const std::string data_file_n = "/data_m_ubo";

      unsigned no_ubo_mesh;
      /*std::ifstream data_file_first(std::string(mesh_dir)+data_file_n + file_extension, std::ios::binary);
      if (!data_file_first.is_open()) {
        throw std::runtime_error("Can not open file " + std::string(mesh_dir)+data_file_n + file_extension);
      }
      data_file_first.read((char*)&no_ubo_mesh, sizeof(unsigned) ); data_file_first.close();

      std::unique_ptr<unsigned[]> alldata = std::make_unique<unsigned[]>(no_ubo_mesh * 3 + 1);  //rotation, translation, scale and first data

      //no_ubo_mesh = test_reading_data_first;
      //if (no_ubo_mesh != (no_mesh + no_m_mesh)) throw std::runtime_error("number of ubo model matrices does not equal the number of meshes");
      movement.resize(no_ubo_mesh);

      std::ifstream data_file(std::string(mesh_dir)+data_file_n + file_extension, std::ios::binary); data_file.read((char*)&alldata[0], (no_ubo_mesh * 3 + 1) * sizeof(unsigned) ); data_file.close();
      unsigned* testptr = &alldata[1];  */
      /*unsigned test_reading_data_first;
      std::ifstream data_file_first(std::string(global::asset_dir)+"/Meshes/ubo_data" + file_extension, std::ios::binary); data_file_first.read((char*)&test_reading_data_first, sizeof(unsigned) ); data_file_first.close();

      std::unique_ptr<unsigned[]> alldata = std::make_unique<unsigned[]>(test_reading_data_first * 3 + 1);

      no_ubo_mesh = test_reading_data_first;
      //if (no_ubo_mesh != (no_mesh + no_m_mesh)) throw std::runtime_error("number of ubo model matrices does not equal the number of meshes");
      movement.resize(no_ubo_mesh);

      std::ifstream data_file(std::string(global::asset_dir)+"/Meshes/ubo_data" + file_extension, std::ios::binary); data_file.read((char*)&alldata[0], (test_reading_data_first * 3 + 1) * sizeof(unsigned) ); data_file.close();
      unsigned* testptr = &alldata[1];*/


      std::unique_ptr<unsigned[]> no_frames = std::make_unique<unsigned[]>(no_ubo_mesh);


      std::vector<float> reading_rotations;
      std::vector<float> reading_translation;

      float *rotations_ptr, *translations_ptr;

      for (size_t i = 0; i < static_cast<size_t>(no_ubo_mesh); i++) { //for each mesh
        no_frames[i] = *testptr++;
        movement[i].models.resize(no_frames[i]);

        reading_rotations.resize(rotation_elements);
        reading_translation.resize(translation_elements);

        float rotation_angle;
        glm::vec3 rotation_vector, translation_vector;

        for (size_t j = 0; j < static_cast<size_t>(no_frames[i]); j++) { //for every frame of movement for a given mesh
          //reading the rotations for every frame
          std::ifstream rotations_file(static_cast<std::string>(std::string(global::asset_dir)+"/Meshes/rotations_") + std::to_string(i) + "_" + std::to_string(j) + file_extension, std::ios::binary);
          if (!rotations_file.is_open()) {throw std::runtime_error(static_cast<std::string>("failed to open rotations file ") + std::to_string(i) + file_extension);}
          rotations_file.read((char*)&reading_rotations[0], no_frames[i] * rotation_elements * sizeof(float)); rotations_file.close();


          //reading the translations for every frame
          std::ifstream translations_file(static_cast<std::string>(std::string(global::asset_dir)+"/Meshes/translations_") + std::to_string(i) + "_" + std::to_string(j) + file_extension, std::ios::binary);
          if (!translations_file.is_open()) {throw std::runtime_error(static_cast<std::string>("failed to open translations file ") + std::to_string(i) + file_extension);}
          translations_file.read((char*)&reading_translation[0], no_frames[i] * translation_elements * sizeof(float) ); translations_file.close();

          rotations_ptr = &reading_rotations[0];
          translations_ptr = &reading_translation[0];

          //glm doesnt behave nicely if the values are not set in this way
          rotation_angle = *rotations_ptr++;
          rotation_vector.x = *rotations_ptr++;
          rotation_vector.y = *rotations_ptr++;
          rotation_vector.z = *rotations_ptr++;

          translation_vector.x = *translations_ptr++;
          translation_vector.y = *translations_ptr++;
          translation_vector.z = *translations_ptr++;

          if (rotation_vector == glm::vec3(0.0f, 0.0f, 0.0f)) { //glm::rotate doesnt behave nicely if the rotation axis is the 0 vector
            movement[i].models[j] = glm::translate(glm::mat4(1.0f), translation_vector);
          } else {
            movement[i].models[j] = glm::translate(glm::rotate(glm::mat4(1.0f), rotation_angle, rotation_vector), translation_vector);
          } //end if

        } //end inner loop

      } //end outer loop

      alldata.reset();


      #ifdef detailed_timing
        auto end = std::chrono::steady_clock::now();
        mtx.lock();
        std::cout << "\t Moving mesh UBO reading took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
        mtx.unlock();
      #endif
    }
  }

}
