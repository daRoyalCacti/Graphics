#pragma once
#include <vector>		//for std::vector used throughout
#include <optional> //for std::optional used in the QueueFamilyIndices struct
#include <future>		//needed for std::async -- threading

#include "../../General_Graphics/Pixels.h"	//for pixels which is used to create a pointer which the texture data will be read into

#include <GLFW/glfw3.h>	//for the window which vulkan with draw to -- and the GLFWwindow data type needs to be passed to a function defined here
#include "../../General_Graphics/player_camera.h"	//used to define the extern camera
#include "../vulkan_general/ubos.h"								//for defining the uniform buffer object and the uniform buffer as well as the movement data type needed for moving meshes
#include "../meshes/static_simple_mesh.h"		//for the rendering and loading of static meshes
#include "../meshes/moving_simple_mesh.h"		//for the rendering and loading of animated meshes
#include "../global.h"			//for preprocessor definitions

#include <memory>			//for unique pointers

const std::vector<const char*> validationLayers = { //what validation layers to use
	"VK_LAYER_KHRONOS_validation"
	//, "VK_LAYER_LUNARG_api_dump" //testing to see if other layers would work
	//available layers are logged if gore_detail is enabled
};

const std::vector<const char*> deviceExtensions = {	//what device extensions to use -- only need at swapchain at the moment
	"VK_KHR_swapchain"
	//, "VK_KHR_storage_buffer_storage_class", "VK_KHR_get_physical_device_properties2", "VK_KHR_8bit_storage" //testing to see if other extensions would work
	//available extensions are logged if gore_detail is enabled
};




struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};




class vulkanApp {
public:


	void run();

	void initWindow();
	void loadData();

	void initVulkan(std::future<void>* data_thread);
	void mainLoop();
	void cleanup();

	void drawFrame();


	vulkanApp() {

	}

	~vulkanApp() {

	}

	#ifdef naive_recording
	void take_screenshot(const char*);
	#endif


	GLFWwindow* window;

private:
	uint32_t no_mesh;
	uint32_t no_m_mesh;

	//int framecounter_pos = framecounter;

	VkDescriptorPool descriptorPool;

	std::unique_ptr<ubo_model[]> square_model;

	std::unique_ptr<staticSimpleMesh[]> staticMeshes;

	std::unique_ptr<moving_simple_mesh[]> movingMeshes;

	std::unique_ptr<pixels[]> imagePixels;

	#ifdef precalculated_player_camera
		uint32_t no_camera_positions;
		std::unique_ptr<glm::vec3[]> camera_positions;
		std::unique_ptr<float[]> camera_yaws;
		std::unqiue_ptr<float[]> camera_pitchs;
	#endif





	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	VkDevice device;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;


	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;


	std::vector<VkImageView> swapChainImageViews;


	VkRenderPass renderPass;


	VkDescriptorSetLayout descriptorSetLayout;

	VkPipelineLayout pipelineLayout;


	VkPipeline graphicsPipeline;

	std::vector<VkFramebuffer> swapChainFramebuffers;


	VkCommandPool commandPool;

	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;


	VkSampler textureSampler;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	size_t currentFrame = 0;

	bool framebufferResized = false;

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);



	void recreateSwapChain();



	void updateUniformBuffers(uint32_t currentImage);

	void cleanupSwapChain();

	void createInstance();

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	void setupDebugMessenger();

	void createSurface();

	void pickPhysicalDevice();

	void createLogicalDevice();

	void createSwapChain();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	void createImageViews();

	void createRenderPass();

	void createDescriptorSetLayout();

	void createGraphicsPipeline();

	void createFramebuffers();

	void createCommandPool();

	void createDepthResources();

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	VkFormat findDepthFormat();

	void createTextureImage();


	void createTextureImageView();

	void createTextureSampler();

	void createVertexBuffer();

	void createIndexBuffer();

	void createUniformBuffers();

	void createDescriptorPool();

	void createDescriptorSets();


	void createCommandBuffers();

	void createSyncObjects();


	bool isDeviceSuitable(VkPhysicalDevice device);

	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	std::vector<const char*> getRequiredExtensions();

	bool checkValidationLayerSupport() ;

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);


	int rateDeviceSuitability(VkPhysicalDevice);


};
