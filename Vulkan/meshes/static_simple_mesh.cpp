#include <iostream>
#include <cstring>
#include "static_simple_mesh.h"
//#include "../../General_Graphics/Pixels.h"
#include "../vulkan_general/Buffer.h"
#include "../vulkan_main/vulkan_help.h"

#include "../global.h"


void staticSimpleMesh::cleanup(VkDevice device) {
	tex.cleanup(device);
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
	sb.cleanup(device);

	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);
}


void staticSimpleMesh::createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool* descriptorPool, VkDevice device, VkSampler textureSampler) {
	std::vector<VkDescriptorSetLayout> layouts(ub.size, descriptorSetLayout);
	//the descriptor set allocation
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = *descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(ub.size);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(ub.size);
		//allocating the descriptor set handles
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < ub.size; i++) {
				//descriptors that refer to buffers are configured with VkDescriptorBufferInfo
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = ub.uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = tex.textureImageView;
        imageInfo.sampler = textureSampler;

				#ifdef testing2
				VkDescriptorBufferInfo lightingInfo = {};
				lightingInfo.buffer = ub.lightingBuffers[i];
				lightingInfo.offset = 0;
				lightingInfo.range = sizeof(LightingObject);
				#endif

				//a VkWriteDescriptorSet is used in the update the configuration of the descriptor sets
				#ifdef testing2
				std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};
				#else
	       std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
				#endif

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];													//specify the descriptor set to update
        descriptorWrites[0].dstBinding = 0;																			//the uniform buffer index
        descriptorWrites[0].dstArrayElement = 0;																//the first index in the descriptor array -- not using an array so this is just 0
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;																//how many descriptor array elements to update -- not using array so just 1
        descriptorWrites[0].pBufferInfo = &bufferInfo;													//for descriptors that refer to buffer data
        descriptorWrites[0].pImageInfo = nullptr;																//for descriptors that refer to image data
        descriptorWrites[0].pTexelBufferView = nullptr;													//for descriptors that refer to buffer views

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

				#ifdef testing2
				descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];													//specify the descriptor set to update
        descriptorWrites[2].dstBinding = 2;																			//the uniform buffer index
        descriptorWrites[2].dstArrayElement = 0;																//the first index in the descriptor array -- not using an array so this is just 0
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[2].descriptorCount = 1;																//how many descriptor array elements to update -- not using array so just 1
        descriptorWrites[2].pBufferInfo = &lightingInfo;													//for descriptors that refer to buffer data
        descriptorWrites[2].pImageInfo = nullptr;																//for descriptors that refer to image data
        descriptorWrites[2].pTexelBufferView = nullptr;													//for descriptors that refer to buffer views
				#endif


				//Actually updating the descriptor sets
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}


void staticSimpleMesh::draw(std::vector<VkCommandBuffer> commandBuffers, VkPipelineLayout pipelineLayout, size_t i) {
	VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

		//binding the right descriptor set for each swap chain image
    vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);


	vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0,0);
	//vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);

}


void staticSimpleMesh::createVertexBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkPhysicalDevice physicalDevice) {
	bufferSize = sizeof(vertices[0]) * vertices.size();

	if (staging){
		sb.create_buffer_vertex(bufferSize, device, physicalDevice, &vertexBuffer, &vertexBufferMemory, commandPool, graphicsQueue, vertices);
	} else {
    	createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);

		void* data;
    	vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);
    	memcpy(data, vertices.data(), (size_t) bufferSize);
    	vkUnmapMemory(device, vertexBufferMemory);
	}
}

void staticSimpleMesh::updateVertexBuffer(VkDevice device, std::vector<Vertex> _vertices, VkCommandPool commandPool, VkQueue graphicsQueue) {
	if (staging){
		sb.updateVertexBuffer(device, &vertexBuffer, _vertices, commandPool, graphicsQueue);
	} else {
		void* data;
    	vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);
    	memcpy(data, _vertices.data(), (size_t) bufferSize);
    	vkUnmapMemory(device, vertexBufferMemory);
	}

}


void staticSimpleMesh::createIndexBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkPhysicalDevice physicalDevice){

	VkDeviceSize _bufferSize = sizeof(indices[0]) * indices.size();

	GBF::stagingBufferObject _sb;

  _sb.create_buffer_index(_bufferSize, device, physicalDevice, &indexBuffer, &indexBufferMemory, commandPool, graphicsQueue, indices);
  _sb.cleanup(device);

}
