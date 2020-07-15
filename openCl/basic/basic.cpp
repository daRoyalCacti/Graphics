#include "basic.h"
#include <iostream>
#include <sstream>
#include <fstream>


namespace openCl {
	
	void run_basic::create_program_object(const char* programPath) {
		//creates the program object to hold all kernel functions
		
		cl_int err; //to hold any errors found

		//reading the program code into memory using strings
		std::string programCode;
		try {
			std::ifstream programFile;
			programFile.open(programPath);
			
			std::stringstream programStream;
			programStream << programFile.rdbuf();

			programFile.close();
			programCode = programStream.str();
		}
		catch (std::ifstream::failure& e) {
			throw std::runtime_error("Failed to read program at : " + std::string(programPath) );
		}

		const char* ProgramSource = programCode.c_str();

		program = clCreateProgramWithSource(
				oinstance.context,
				1, //the number of strings in the next parameter
				(const char**) &ProgramSource, //the array of strings that make up the source code
				NULL,	//the length of each string
				&err
			);

		if (cl_check_result(err, "clCreateProgramWithSource") )
			throw std::runtime_error("clCreateProgramWithSource failed");

	}





	void run_basic::build_program() {
		//Compiling and linking the program object (from the source code from create_program_object)

		const cl_int err = clBuildProgram(
				program,
				0,	//number of devices in device list 
				NULL, 	//device list -- NULL means all device (means above parameter meaningless
				NULL,	//a string of buld options
				NULL,	//callback function
				NULL	//data arguments for callback function
			);

		//error checking
		//first logging any compiling errors
		//later, doing standard error checking
		if (err == CL_BUILD_PROGRAM_FAILURE) {
			std::cerr << "\t Failed to build program executable" << std::endl;
			
			//logging the build output to standard output
			char buffer[4096];
			size_t length;

			const cl_int err2 = clGetProgramBuildInfo(
					program,
					oinstance.device_id,	//the device the executable was built for	
					CL_PROGRAM_BUILD_LOG, 	//could also be CL_PROGRAM_BUILD_OPTIONS
					sizeof(buffer),		//size of the buffer to write to
					buffer,			//the actual buffer
					&length			//the acutal size in bytes of the log
				);
			
			//if getting the build info failed
			if (cl_check_result(err2, "clGetProgramBuildInfo")) 
				throw std::runtime_error("clGetProgramBuildInfo failed");
			

			//priting build log
			std::cout << "\nBuild Log: " << std::endl;
			std::printf("%s\n", buffer);
			throw std::runtime_error("Bulding program has failed");
		}
		
		//standard error checking
		if (cl_check_result(err, "clBuildProgram") ) 
			throw std::runtime_error("clBuildProgram failed");

	}







	void run_basic::create_kernel_objects(const char* programName) {
		//creating the kernel objects
		//kernel objects are an encapsulation of a __kernel function

		cl_int err;

		kernel = clCreateKernel(
			program,
			programName,	//the name of the kernel
			&err
		);
		
		if (cl_check_result(err, "clCreateKernel") ) 
			throw std::runtime_error("clCreateKernel failed");		
	}




	
	void run_basic::create_buffer(cl_mem& data, const cl_mem_flags flags, const unsigned size_total) {
		//creates buffers to hold the memory
		
		cl_int err;

		data = clCreateBuffer(
				oinstance.context,
				flags,			//bit field used to specify the usage of memory
				size_total,		//size in bytes of the buffer to allocate
				NULL,			//pointer to buffer data
				&err
			);

		if (cl_check_result(err, "clCreateBuffer") ) 
			throw std::runtime_error("clCreateBuffer failed");
	}
	

	



	void run_basic::set_kernel_argument(cl_mem* data, const unsigned index) {
		//setting kernel arguments
		//i.e. the arguments in the __kernel function

		const cl_int err = clSetKernelArg(
				kernel,
				index,		//kernel index
				sizeof(cl_mem),	//arg size
				//&data		//arg value
				//std::forward<cl_mem>(*data)
				data
			);

		if (cl_check_result(err, "clSetKernelArg") ) 
			throw std::runtime_error("clSetKernelArg failed");
	
	}



	
	void run_basic::enqueue_kernel(const unsigned size) {
		//Program executation

		size_t global = size;
		const cl_int err = clEnqueueNDRangeKernel(
				oinstance.command_queue,
				kernel,
				1,	//number of dimensions
				NULL, 	//reserved for future versions
				&global,//global work size
				NULL,	//local work size
				0,	//number of events
				NULL,	//list of events that first need to be completed
				NULL	//event object to return on completion
			);

		if (cl_check_result(err, "clEnqueueNDRangeKernel") ) 
			throw std::runtime_error("clEnqueueNDRangeKernel failed");
	}


	



	void run_basic::cleanup() {
		clReleaseProgram(program);
		clReleaseKernel(kernel);
	}



}
