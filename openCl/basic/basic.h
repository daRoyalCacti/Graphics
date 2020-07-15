#pragma once
#include "../required/instance.h"
#include "../required/error_checking.h"
#include <stdexcept>

namespace openCl {
	struct run_basic {
		instance oinstance;

		cl_program program;
		cl_kernel kernel;

		run_basic(instance& o_instance) : oinstance(o_instance) {}

		void create_program_object(const char* programPath);

		void build_program();

		void create_kernel_objects(const char*  programName);

		void create_buffer(cl_mem& data, const cl_mem_flags flags, const unsigned size_total); //size_total = sizeof(float) * size
		
		template <typename T>
		void write_to_buffer(cl_mem& data, const T input, const unsigned size_total){	//needs templates
			//writing data to buffers

			const cl_int err = clEnqueueWriteBuffer(
					oinstance.command_queue,
					data,				//the memory buffer object to write to
					CL_TRUE,			//true for blocking write -- not returned immediately
					0,				//offset of the buffer object to write to
					size_total,			//size in bytes of data to write
					input,				//pointer to the buffer in host memory to read data from
					0,				//number of events in events list
					NULL,				//events list that need to be completed before this will execute
					NULL				//event object to return on completion
				);

			if (cl_check_result(err, "clEnqueueWriteBuffer") ) 
				throw std::runtime_error("clEnqueueWriteBuffer failed");
		}

		void set_kernel_argument(cl_mem* data, const unsigned index);

		void enqueue_kernel(const unsigned size);	//not size_total

		template <typename T>
		void read_from_buffer(cl_mem& data, T output, const unsigned size_total) {
			//reading data back
			const cl_int err = clEnqueueReadBuffer(
					oinstance.command_queue,
					data,				//memory buffer to read from
					CL_TRUE,			//true for block writing -- not returned immediately
					0,				//offset in the buffer object to read from
					size_total,			//size in bytes of data being read
					output,				//pointer to buffer in host memory to store data
					0,				//number of events
					NULL,				//list of events that need to be completed first
					NULL				//event object to return on completion
				);

			if (cl_check_result(err, "clEnqueueReadBuffer") )
				throw std::runtime_error("clEnqueuReadBuffer failed");

		}

		void cleanup();	//cl_mem must be cleanup before ran

	};
}
