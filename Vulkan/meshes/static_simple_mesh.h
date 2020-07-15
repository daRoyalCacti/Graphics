#pragma once
#include <vector>
#include "../vulkan_general/vertex.h"
#include "../vulkan_general/ubos.h"
#include "../vulkan_general/staging_buffer.h"

#include <string>
#include "../vulkan_general/Texture.h"

#include "../global.h"


void do_bar();

class staticSimpleMesh {
public:
	staticSimpleMesh(std::vector<Vertex> verts, std::vector<uint32_t> inds, bool stag = 1) : vertices(verts), indices(inds), staging(stag) {}
	staticSimpleMesh(){}

	inline void createUb(uint32_t size, VkDevice device, VkPhysicalDevice physicalDevice) {
		ub.size = size;
		ub.create(device, physicalDevice);
	}

	inline void cleanup_swapChain(VkDevice device) {
		ub.cleanup_swapChain(device);
	}
	void cleanup(VkDevice device);


	inline void ubo_update(uint32_t currentImage, VkDevice device) {
		ub.update(currentImage, device);
	}

	void createDescriptorPool(VkDevice device);
	void createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool *descriptorPool, VkDevice device, VkSampler textureSampler);

	inline void createTexture(pixels pix, VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue) {
		tex.create(pix, device, physicalDevice, commandPool, graphicsQueue);
	}

	inline void createTextureImageView(VkDevice device) {
		tex.createView(device);
	}

	void draw(std::vector<VkCommandBuffer> commandBuffers, VkPipelineLayout pipelineLayout, size_t i);

	void createVertexBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkPhysicalDevice physicalDevice);

	void createIndexBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkPhysicalDevice physicalDevice);

	void updateVertexBuffer(VkDevice device, std::vector<Vertex> vertices, VkCommandPool commandPool, VkQueue graphicsQueue);

	void init(); //!! need to actually do

	void TextureCleanupCreation(VkDevice device);

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	UniformBuffer ub;


	bool staging = 1;	//whether or not to use a staging buffer for vertices
										//should always be true

private:
	float posx, posy, posz, length1, length2;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	GBF::stagingBufferObject sb;
	VkDeviceSize bufferSize;

	//holds the descriptor set handles
	std::vector<VkDescriptorSet> descriptorSets;
	VkImageView textureImageView;
	VkBuffer vertexBuffer;
	VkBuffer indexBuffer;
	texture tex;

	VkDeviceMemory vertexBufferMemory;
	VkDeviceMemory indexBufferMemory;

};
