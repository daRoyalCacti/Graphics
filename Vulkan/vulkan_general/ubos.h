#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "../global.h"

//this is same as for the vertex shader
//the data in the matrices is binary compatible with the way the shader expects it
struct UniformBufferObject {
	#ifdef testing1
	glm::vec2 foo;
	#endif
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

#ifdef testing2
struct LightingObject {
		alignas(16) glm::mat4 test1;
		alignas(16) glm::mat4 test2;
};
#endif

struct UniformBuffer {
	UniformBufferObject ubo;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	uint32_t size;

	#ifdef testing2
	LightingObject lo;
	std::vector<VkBuffer> lightingBuffers;
	std::vector<VkDeviceMemory> lightingBuffersMemory;
	#endif

	void create(VkDevice device, VkPhysicalDevice physicalDevice);
	void cleanup_swapChain(VkDevice device);
	void update(uint32_t currentImage, VkDevice device);
};


struct ubo_model {
	std::vector<glm::mat4> models;
	unsigned total_frames;
	bool modulous = false;
	ubo_model(){};
	ubo_model(unsigned frames) : total_frames(frames) {};
	glm::mat4 frame(unsigned frame_num);
};
