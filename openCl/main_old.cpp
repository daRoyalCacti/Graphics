#include "openCl/required/instance.h"
#include "openCl/basic/basic.h"
#include <chrono>
#include <iostream>
#include <cmath>

//checking if GLFW works
#include <GLFW/glfw3.h>
GLFWwindow* window;

//checking if GLM works
#include <glm/glm.hpp>
glm::vec3 pos;

//checking if can compile with vulkan
#include "vulkan/vulkan.h"
VkDescriptorPool Test;



const uint32_t size_data = 1e3;

double data[size_data];
double results[size_data];

cl_mem input_data;
cl_mem output_data;


int main() {
  auto start = std::chrono::high_resolution_clock::now();

  #pragma omp parallel for
  for (uint32_t i = 0; i < size_data; i++) { //to be made more concrete later
    data[i] = i/100.0;
  }


  try {
	openCl::instance openCli;
	openCli.query_platform();
	openCli.query_device();
	openCli.create_context();
	openCli.create_command_queue();



	openCl::run_basic basic(openCli);
	basic.create_program_object("/home/george/Documents/Projects/Major-1/GPU_code/square.cl");
	basic.build_program();
	basic.create_kernel_objects("square");

	basic.create_buffer(input_data, CL_MEM_READ_ONLY, sizeof(data));
	basic.create_buffer(output_data, CL_MEM_WRITE_ONLY, sizeof(data));

	basic.write_to_buffer(input_data, data, sizeof(data));

	basic.set_kernel_argument(&input_data,0);
	basic.set_kernel_argument(&output_data,1);

	basic.enqueue_kernel(size_data);
	basic.read_from_buffer(output_data, results, sizeof(data));

	clReleaseMemObject(input_data);
	clReleaseMemObject(output_data);
	basic.cleanup();

  openCli.cleanup();

    for (unsigned i = 0; i < size_data; i++) {
      if (results[i] - data[i] * data[i]> 0.01) {
        std::cerr << "value " << i << " is wrong" << std::endl;
      }
    }
  } catch (const std::exception& e) {
  	std::cerr << e.what() << std::endl;
  }


  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "Completed in \t\t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;


  return EXIT_SUCCESS;
}
