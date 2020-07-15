#include "ubos.h"
#include "../vulkan_main/vulkan_help.h"
#include <iostream>
#include <cstring>

void UniformBuffer::create(VkDevice device, VkPhysicalDevice physicalDevice){
	//the buffer that contains the UBO doata for the shader
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	uniformBuffers.resize(size);
    uniformBuffersMemory.resize(size);
    for (size_t i = 0; i < size; i++) {
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        	uniformBuffers[i], uniformBuffersMemory[i]);
    }

		#ifdef testing2
		VkDeviceSize bufferSize2 = sizeof(LightingObject);
		lightingBuffers.resize(size);
	    lightingBuffersMemory.resize(size);
	    for (size_t i = 0; i < size; i++) {
	        createBuffer(device, physicalDevice, bufferSize2, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	        	lightingBuffers[i], lightingBuffersMemory[i]);
	    }
		#endif

}

void UniformBuffer::cleanup_swapChain(VkDevice device){
	for (size_t i = 0; i < size; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);

				#ifdef testing2
				vkDestroyBuffer(device, lightingBuffers[i], nullptr);
        vkFreeMemory(device, lightingBuffersMemory[i], nullptr);
				#endif
    }
}

void UniformBuffer::update(uint32_t currentImage, VkDevice device){
	ubo.proj[1][1] *= -1;
	void* data;
		//map uniformBuffersMemory to data (?)
    vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
		//use this map to change ubo (?)
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniformBuffersMemory[currentImage]);

		#ifdef testing2
		void* data2;
		vkMapMemory(device, lightingBuffersMemory[currentImage], 0, sizeof(lo), 0, &data2);
    memcpy(data2, &lo, sizeof(lo));
    vkUnmapMemory(device, lightingBuffersMemory[currentImage]);
		#endif
}


//pure on request of the compiler
__attribute__((pure)) glm::mat4 ubo_model::frame(unsigned frame_num) {
		if (modulous) {
			return models[frame_num % (total_frames)];
		} else {
			if (frame_num > total_frames) {
				return models[total_frames - 1];
			} else {
				return models[frame_num];
			}
		}
}
