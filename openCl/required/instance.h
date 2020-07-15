#pragma once

#include <CL/cl.hpp>


namespace openCl {
class instance {
	//creates an environment for executing code on 1 GPU using openCL
public:
  cl_int err;

  cl_platform_id platform_id; //used to identify an opencl platform
  cl_uint num_platforms;
  const cl_uint platform_entries = 1;

  cl_device_id device_id; //used to identify an opencl compute device
  cl_uint num_of_devices;
  const cl_uint device_entries = 1;
  //const cl_device_type device_type = CL_DEVICE_TYPE_GPU; // other possibilities : CL_DEVICE_TYPE_CPU, CL_DEVICE_TYPE_ACCELERATOR, CL_DEVICE_TYPE_DEFAULT, CL_DEVICE_TYPE_ALL

  cl_context context;
  cl_context_properties properties[3]; //complicated and i dont really understand how it works

  cl_command_queue command_queue;

  
  void query_platform();

  void query_device(const cl_device_type device_type = CL_DEVICE_TYPE_GPU);

  void create_context();

  void create_command_queue();

  void cleanup();

  //void init();


};

}
