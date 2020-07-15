#include "instance.h"
#include "error_checking.h"
#include <iostream>
#include <string>
#include <cmath>
#include <chrono>
#include <stdexcept>

#define timing

namespace openCl {
void instance::query_platform() {
  //query for 1 openCl platform
  //===============================================================================================================================================================================================================
  err = clGetPlatformIDs( //used to retrieve a list of available platforms
    platform_entries, //num_entries : the number of platform entries (will be added to platform)
    &platform_id, //*platform : sets var to a list of found openCl platforms
    &num_platforms //*num_platforms : the number of platforms available
  );
  //clGetPlatformInfo(..) can be used to get information about the platform : see https://www.khronos.org/registry/OpenCL/sdk/1.0/docs/man/xhtml/clGetPlatformInfo.html

  //error checking
  if (cl_check_result(err, "clGetPlatformIDs")) {
	  throw std::runtime_error("clGetPlatformIDs Failed");
  }
}

void instance::query_device(const cl_device_type device_type) {// = 	CL_DEVICE_TYPE_GPU) {
  //query for 1 GPU compute device
  //default device type is GPU
  //===============================================================================================================================================================================================================

  err = clGetDeviceIDs( //used to search for compute devices
    platform_id,
    device_type, //the device type to search for
    device_entries, //num_entries : the number of devices to search for (in essence - is likely more complicated than that)
    &device_id, //*devices : the list of device ids
    &num_of_devices //*num_devices : the number of OpenCl devices for the given type found
  );
  //clGetDeviceInfo(..) can be used to find the capabilities for the found device -- very similar to clinfo in the shell

  if (cl_check_result(err, "clGetDeviceIDs")) {
	  throw std::runtime_error("clGetDeviceIDs failed");
  }

}

void instance::create_context() {
  //create a context
  //contexts are used by openCl to manage command queues, program objects, kernel objects, and the sharing of memory objects
  //===============================================================================================================================================================================================================
  properties[0] = CL_CONTEXT_PLATFORM; //as by specification
  properties[1] = (cl_context_properties)platform_id; //as by specification -- cannot static_cast
  properties[2] = 0; //properties list must be terminated with a 0

  context = clCreateContext( //is quite slow
    properties,
    device_entries, //num_devices : number of devices in the device_id list - this was specified during the finding for devices
    &device_id, //*devices : the device id list
    NULL, //*pfn_notify : pointer to an error callback function
    NULL, //*user_data : argument to passs to the callback functin
    &err
  );

  if (cl_check_result(err, "clCreateContext")) {
	  throw std::runtime_error("clCreateContext failed");
  }

}

void instance::create_command_queue() {
  //create a command queue
  //command queues allow commands to be sent to compute devices associated with a specified context
  //===============================================================================================================================================================================================================
  const cl_command_queue_properties command_queue_properties[] = {0}; //commented below has details
  /*const cl_command_queue_properties command_queue_properties[] = {
    CL_QUEUE_PROPERTIES,
    CL_QUEUE_ON_DEVICE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, //bit field values of CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, CL_QUEUE_PROFILING_ENABLE, CL_QUEUE_ON_DEVICE, CL_QUEUE_ON_DEVICE_DEFAULT
    CL_QUEUE_SIZE, //size of the device queue in bytes
    CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE, // is default - should always be less than this value but must be less than CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE
    0 //must be terminated with a 0
  };*/

  command_queue = clCreateCommandQueueWithProperties( //for earlier versions of opencl clCreateCommandQueue needs to be used
    context,
    device_id,
    command_queue_properties, //properties for the queue
    &err
  );

  if (cl_check_result(err, "clCreateCommandQueueWithProperties")) {
	  throw std::runtime_error("clCreateCommandQueueWithProperties failed");
  }

}



void instance::cleanup() {
  //clean up
  //===============================================================================================================================================================================================================

  clReleaseCommandQueue(command_queue);
  clReleaseContext(context);

}


/*
void instance::init() {
  #ifdef timing
  	auto start1 = std::chrono::high_resolution_clock::now();
  #endif
  query_platform();
  #ifdef timing
  	auto end1 = std::chrono::high_resolution_clock::now();
  	std::cout << "Platform querying took \t\t\t" << std::chrono::duration <double, std::milli>(end1 - start1).count() << "ms" << std::endl;
  	auto start2 = std::chrono::high_resolution_clock::now();
  #endif
  query_device();
  #ifdef timing
  	auto end2 = std::chrono::high_resolution_clock::now();
  	std::cout << "Device querying took \t\t\t" << std::chrono::duration <double, std::milli>(end2 - start2).count() << "ms" << std::endl;
  	auto start3 = std::chrono::high_resolution_clock::now();
  #endif
  create_context();
  #ifdef timing
  	auto end3 = std::chrono::high_resolution_clock::now();
  	std::cout << "Context creation took \t\t\t" << std::chrono::duration <double, std::milli>(end3 - start3).count() << "ms" << std::endl;
  	auto start4 = std::chrono::high_resolution_clock::now();
  #endif
  create_command_queue();
  #ifdef timing
  	auto end4 = std::chrono::high_resolution_clock::now();
  	std::cout << "Command queue creation took \t\t" << std::chrono::duration <double, std::milli>(end4 - start4).count() << "ms" << std::endl;
  #endif
}
*/


}
