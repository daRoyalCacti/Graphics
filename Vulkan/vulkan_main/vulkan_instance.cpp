#define GLFW_INCLUDE_VULKAN //getting GLFW to include the relevent vulkan headers
#include <iostream> //for cout can cerr
#include <chrono>		//for timing
#include <map>			//for the physical device selection process
#include "../user_inputs/window_inputs.h"	//for keyboard processing

#include "vulkan_instance.h"	//the main header file
#include "vulkan_help.h"			//for all helper functions used that are not stated in the main header file

#include <stdexcept>		//to throw runtime errors
#include <set>					//for std::set
#include "../global.h"			//for most preprocessor definitions seen -- mainly timing, detailed_timing, NODEBUG, gore_detail
#include "../file_reading/data.h"				//for all the data reading functions -- called in the 'loadData' function

#include "../../General_Graphics/player_camera.h"		//for camera vectors and for MVP matrices
#include "../vulkan_general/decriptorSetLayout.h"		//helper functions for creating the descriptor set layout
#include "../vulkan_general/graphicsPipeline.h"			//helper functions for creating the graphics pipeline
#include "../vulkan_general/callbacks.h"						//for all callback functions
#include "../error_checking/check_error.h"					//for easy error checking of vulkan functions

#include <thread>					//currently used for logging the number of threads -- plans to add concurrency
#include <GLFW/glfw3.h>		//for creating the window and getting keyboard and mouse inputs

#define GLM_FORCE_RADIANS										//to make glm use radians
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES	//to force glm to space vectors in the way that the shader wants
#define GLM_FORCE_ZERO_TO_ONE								//forces the depth buffer to be from 0 to 1 -- I think
#include <glm/glm.hpp>											//main glm header
#include <glm/gtc/matrix_transform.hpp>			//for glm::rotate, glm::translate and similar

//#include <future>		//for std::async and its return type std::future


namespace vk_settings {
	const char* window_name = "Vulkan";	//name of the window
	//for setting ubo.perspective using glm::perspective
	const float fov = 45.0f;
	const float near_plane = 0.1f;
	const float far_plane = 100.0f;

	//main settings
	const int WIDTH = 800;
	const int HEIGHT = 600;
	const int MAX_FRAMES_IN_FLIGHT = 2;

	//for swapchain creation
	const VkFormat swapChainFormat = VK_FORMAT_B8G8R8A8_UNORM;
	const VkColorSpaceKHR swapChainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	const VkBool32 swapChainClipped = VK_TRUE;

	const VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT; //for modes see  https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkCullModeFlagBits.html
}
namespace global {
	playerCamera camera;	//referencing the extern variable defined in the global header
	#ifdef precalculated_player_camera
		uint32_t total_frames;
	#endif
}

#ifdef NODEBUG
const bool enableValidationLayers = false;
#else
bool enableValidationLayers = true; //not const because validation layers are turned off if the requrested validation layers cannot be found
#endif


void vulkanApp::loadData() {
#ifdef timing
	auto start = std::chrono::steady_clock::now();
#endif

	std::future<uint32_t> mesh_no = std::async(&files::read_Simple_Static_mesh);

	std::future<uint32_t> m_mesh_no = std::async(&files::read_Simple_Moving_mesh);

	std::future<void> ubo_no = std::async(&files::read_ubo);

	std::future<uint32_t> image_no = std::async(&files::read_texture);

	#ifdef precalculated_player_camera
		//std::future<uint32_t> camera_positions_no = std::async(&files::read_camera);
	//files::read_camera(camera_positions, camera_yaws, camera_pitchs);
	#endif

	no_mesh = mesh_no.get();
	no_m_mesh = m_mesh_no.get();
	ubo_no.wait();
	uint32_t no_image = image_no.get();
	#ifdef precalculated_player_camera
		no_camera_positions = camera_positions_no.get();
		global::total_frames = no_camera_positions;
	#endif


	//keeping around for a while because having trouble with seg faults
	/*no_mesh = files::read_Simple_Static_mesh();

	no_m_mesh = files::read_Simple_Moving_mesh();

	files::read_ubo();*/





	//uint32_t no_image = files::read_texture();	//assumed to be the same as the number of mesh //!!need to update to be otherwise
																							//cannot be read async because it depends on the values read for the meshes
																							//wouldn't be any point because it stupid fast

	staticMeshes = std::make_unique<staticSimpleMesh[]>(no_mesh);
	movingMeshes = std::make_unique<moving_simple_mesh[]>(no_m_mesh);
	square_model = std::make_unique<ubo_model[]>(no_mesh + no_m_mesh);
	imagePixels = std::make_unique<pixels[]>(no_image);

	#ifdef precalculated_player_camera
		camera_positions = std::make_unique<glm::vec3[]>(no_camera_positions);
		camera_yaws = std::make_unique<float[]>(no_camera_positions);
		camera_pitchs = std::make_unique<float[]>(no_camera_positions);
	#endif

	//this is done very messy -- need to update in the future
	std::future<void> static_thread = std::async(&files::vk::Vertices_to_Simple_Static_Mesh, staticMeshes.get());
	std::future<void> moving_therad = std::async(&files::vk::Vertices_to_Simple_Moving_Mesh, movingMeshes.get());
	std::future<void> rotations_thread = std::async(&files::vk::Rotations_to_UBOs, square_model.get());
	std::future<void> texture_thread = std::async(&files::vk::Texture_to_Pixels, imagePixels.get());
	#ifdef precalculated_player_camera
		std::future<void> camera_thread = std::async(&files::vk::Camera_to_Vectors, camera_positions.get(), camera_yaws.get(), camera_pitchs.get());
	#endif

	static_thread.wait();
	moving_therad.wait();
	rotations_thread.wait();
	texture_thread.wait();
	#ifdef precalculated_player_camera
		camera_thread.wait();
	#endif

/*#ifdef precalculated_player_camera
	#ifdef detailed_timing
		auto start6 = std::chrono::steady_clock::now();
	#endif
	vk_files::Load_Camera_Positions(camera_positions, camera_yaws, camera_pitchs);
	#ifdef detailed_timing
		auto end6 = std::chrono::steady_clock::now();
		std::cout << "\t Camera reading took \t\t\t" << std::chrono::duration <double, std::milli>(end6 - start6).count() << "ms" << std::endl;
	#endif
#endif*/

#ifdef timing
	auto end = std::chrono::steady_clock::now();
	#ifdef detailed_timing
	std::cout << "----------------------------------------------------------------------------" << std::endl;
	#endif
	std::cout << "Data reading took \t\t\t\t\t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#ifdef detailed_timing
	std::cout << "----------------------------------------------------------------------------" << std::endl;
	#endif
#endif
}



void vulkanApp::run() {	//not used
#ifndef NODEBUG
	std::cout << "\n\n\n\n\n\n\n\n\n\n\n"; //to make the console look neater after compiling lots of files
	//only in debug becasue nothing gets written to the console during release
#endif
	loadData();

	initWindow();
	//initVulkan();
	mainLoop();
	cleanup();
}

void vulkanApp::initWindow() {
#ifdef timing
	auto start = std::chrono::steady_clock::now();
#endif
	//seems to be saying that vulkan is not supported despite the application running fine
	//if (glfwVulkanSupported() == GLFW_FALSE) {
	//	throw std::runtime_error("Vulkan is not minimally available");
	//}
	glfwInit(); //initilising the GLFW library

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);		//telling GLFW not to create an openGl context (GLFW was made for openGl)
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);			//telling GLFW that resizing is allowed (this requires special treatment with vulkan)


	window = glfwCreateWindow(vk_settings::WIDTH, vk_settings::HEIGHT, vk_settings::window_name, nullptr, nullptr);	//creating the acutal window with a given size and name
																																						//window, WIDTH and HEIGHT defined in the main header
																																						//window_name is defined at the top of this file
																																						//the 2 null pointers are which monitor to fullscreen on and which window whose context shares resources with
																																						// - the first param is not used because the window can open where ever it wants and the second param is only useful with openGl

	glfwSetWindowUserPointer(window, this);																//this sets 'window' to the current active window
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);		//giving GLFW a function to call when the window is resized
	glfwSetCursorPosCallback(window, mouse_callback);											//giving GLFW a function to call when the mouse is moved
	glfwSetScrollCallback(window, scroll_callback);												//giving GLFW a function to call when the scrollwheel is moved


	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);		//being specific that when the window opens, it should capture the cursor

#ifdef timing
	auto end = std::chrono::steady_clock::now();
	std::cout << "Window creation took \t\t\t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
#endif
}



void vulkanApp::initVulkan(std::future<void>* data_thread) {
	//because the beginning of the initilisation does not depend on any data, the thread the data function is being exectued on is passed here
	// - this will only increase performance for large enough meshes
#ifdef timing
	auto start = std::chrono::steady_clock::now();
#endif

	createInstance();

	setupDebugMessenger();

	createSurface();

	pickPhysicalDevice();

	createLogicalDevice();

	createSwapChain();

	createImageViews();

	createRenderPass();

	createDescriptorSetLayout();

	createGraphicsPipeline();

	createCommandPool();

	createDepthResources();

	createFramebuffers();

	data_thread->wait();

	createTextureImage();

	createTextureImageView();

	createTextureSampler();

	createVertexBuffer();

	createIndexBuffer();

	createUniformBuffers();

	createDescriptorPool();

	createDescriptorSets();

	createCommandBuffers();

	createSyncObjects();


#ifdef timing
	auto end = std::chrono::steady_clock::now();
	#ifdef detailed_timing
	std::cout << "----------------------------------------------------------------------------" << std::endl;
	#endif
	std::cout << "Vulkan initilisation took \t\t\t\t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#ifdef detailed_timing
	std::cout << "----------------------------------------------------------------------------" << std::endl;
	#endif
#endif

#ifndef NODEBUG
	//logging information about the device found
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	std::cout << "\nUsing " << indentifyDevice(physicalDeviceProperties.deviceType) << " " << physicalDeviceProperties.deviceName << std::endl;
	std::cout << "\trunning vulkan version " << (physicalDeviceProperties.apiVersion >> 22) << "." << ((physicalDeviceProperties.apiVersion >> 12) & 0x3ff) << "." << (physicalDeviceProperties.apiVersion & 0xfff) << std::endl;
	std::cout << "Found " << std::thread::hardware_concurrency() << " threads \n" << std::endl;
#endif
}




void vulkanApp::mainLoop() {	//repalced
/*
#ifdef player_camera
	while (!glfwWindowShouldClose(window)) {
#endif

#ifdef precalculated_player_camera
		while (!glfwWindowShouldClose(window) && framecounter_pos < no_camera_positions) {
#endif


#ifdef framerate
	auto start = std::chrono::high_resolution_clock::now();
#endif



		glfwPollEvents();
		processWindowInput(window, &camera);
		drawFrame();
		framecounter++;


		framecounter_pos = framecounter;

#ifdef framerate
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << "Framerate " << 1000/std::chrono::duration <float, std::milli>(end - start).count() << " fps" << std::endl;
#endif
}*/
}


void vulkanApp::cleanup() {


	vkDeviceWaitIdle(device);

#ifdef timing
	auto start = std::chrono::steady_clock::now();
#endif
#pragma omp parallel for
	for (uint32_t i = 0; i < no_mesh; i++) {
		staticMeshes[i].cleanup(device);
	}

#pragma omp parallel for
	for (uint32_t i = 0; i < no_m_mesh; i++) {
		movingMeshes[i].cleanup(device);
	}


	cleanupSwapChain();


	vkDestroySampler(device, textureSampler, nullptr);

	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);


	for (size_t i = 0; i < vk_settings::MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(device, commandPool, nullptr);

	vkDestroyDevice(device, nullptr);

	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	staticMeshes.reset();
	square_model.reset();
	movingMeshes.reset();
	imagePixels.reset();

	#ifdef precalculated_player_camera
		camera_positions.reset();
		camera_yaws.reset();
		camera_pitchs.reset();
	#endif

	glfwDestroyWindow(window);

	glfwTerminate();

#ifdef timing
	auto end = std::chrono::steady_clock::now();
	#ifdef detailed_timing
	std::cout << "----------------------------------------------------------------------------" << std::endl;
	#endif
	std::cout << "Cleanup took \t\t\t\t\t\t\t" << std::chrono::duration <double, std::milli>(end - start).count() << " ms" << std::endl;
	#ifdef detailed_timing
	std::cout << "----------------------------------------------------------------------------" << std::endl;
	#endif
#endif

}



void vulkanApp::recreateSwapChain() {
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createDepthResources();
	createFramebuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
}



void vulkanApp::drawFrame() {

	for (uint32_t i = 0; i < no_m_mesh; i++) {
		movingMeshes[i].updateVertexBuffer(device, global::framecounter, commandPool, graphicsQueue);
	}

	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	uint32_t imageIndex;

	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	//manual error checking needed because functions need to be called on given errors
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		vk_check_result(result);
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	#ifdef player_camera
	//unnecessary for predefined because the camera vectors are updated with the ubos
	global::camera.updateCameraVectors();
	#endif


	updateUniformBuffers(imageIndex);


	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	if (!vk_check_result(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]))) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		vk_check_result(result);
		throw std::runtime_error("failed to present swap chain image!");
	}


	currentFrame = (currentFrame + 1) % vk_settings::MAX_FRAMES_IN_FLIGHT;

}


void vulkanApp::updateUniformBuffers(uint32_t currentImage) {
	#ifdef precalculated_player_camera
	global::camera.Position = camera_positions[global::framecounter];
	global::camera.Yaw = camera_yaws[global::framecounter];
	global::camera.Pitch = camera_pitchs[global::framecounter];

	global::camera.updateCameraVectors();
	#endif

//#pragma omp parallel for
	for (uint32_t i = 0; i < no_mesh; i++) {
		uint64_t local_framecounter;
		if (global::framecounter > square_model[i].total_frames - 1) {
			local_framecounter = square_model[i].total_frames - 1;
		} else {
			local_framecounter = global::framecounter;
		}
		staticMeshes[i].ub.ubo.model = square_model[i].frame(local_framecounter);
#ifdef player_camera
		staticMeshes[i].ub.ubo.view = global::camera.view();
#else
#ifdef precalculated_player_camera
		staticMeshes[i].ub.ubo.view = global::camera.view();
#else

		staticMeshes[i].ub.ubo.view = glm::mat4(1.0f);
#endif
#endif

		#ifdef testing1
		staticMeshes[i].ub.ubo.foo = glm::vec2(0.0f, 0.0f);
		#endif
		#ifdef testing2
		staticMeshes[i].ub.lo.test1 = glm::mat4(1.0f);
		staticMeshes[i].ub.lo.test2 = glm::mat4(1.0f);
		#endif

		staticMeshes[i].ub.ubo.proj = glm::perspective(glm::radians(vk_settings::fov), swapChainExtent.width / (float)swapChainExtent.height, vk_settings::near_plane, vk_settings::far_plane);
		staticMeshes[i].ubo_update(currentImage, device);
	}

//#pragma omp parallel for
	for (uint32_t i = no_mesh; i < no_mesh + no_m_mesh; i++) {
		int j = i - no_mesh;
		uint64_t local_framecounter;
		if (global::framecounter > square_model[i].total_frames - 1) {
			local_framecounter = square_model[i].total_frames - 1;
		} else {
			local_framecounter = global::framecounter;
		}
		movingMeshes[j].ub.ubo.model = square_model[i].frame(local_framecounter);
#ifdef player_camera
		movingMeshes[j].ub.ubo.view = global::camera.view();
#else
#ifdef precalculated_player_camera
		movingMeshes[j].ub.ubo.view = global::camera.view();	//updated above

#else
		movingMeshes[j].ub.ubo.view = glm::mat4(1.0f);
#endif
#endif

#ifdef testing1
	movingMeshes[j].ub.ubo.foo = glm::vec2(0.0f, 0.0f);
#endif

#ifdef testing2
	movingMeshes[i].ub.lo.test1 = glm::mat4(1.0f);
	movingMeshes[i].ub.lo.test2 = glm::mat4(1.0f);
#endif

			movingMeshes[j].ub.ubo.proj = glm::perspective(glm::radians(vk_settings::fov), swapChainExtent.width / (float)swapChainExtent.height, vk_settings::near_plane, vk_settings::far_plane);
			movingMeshes[j].ubo_update(currentImage, device);
		}

}

void vulkanApp::cleanupSwapChain() {
	vkDestroyImageView(device, depthImageView, nullptr);
	vkDestroyImage(device, depthImage, nullptr);
	vkFreeMemory(device, depthImageMemory, nullptr);


	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}


	vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	vkDestroyPipeline(device, graphicsPipeline, nullptr);

	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	vkDestroyRenderPass(device, renderPass, nullptr);




	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
//#pragma omp parallel for
	for (uint32_t i = 0; i < no_mesh; i++) {
		staticMeshes[i].cleanup_swapChain(device);
	}

//#pragma omp parallel for
	for (uint32_t i = 0; i < no_m_mesh; i++) {
		movingMeshes[i].cleanup_swapChain(device);
	}


}

void vulkanApp::createInstance() {
	//the instance is the connection between the application and the vulkan api -- this function creates an instance
	//it is used to specify simple application info, extensions, and validation layers

	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	if (enableValidationLayers && !checkValidationLayerSupport()) { //what the error states
		//throw std::runtime_error("validation layers requested, but not available!");
		std::cerr << "validation layers requested, but not available!" << std::endl;
		std::cerr << "running without validation layers!!!" << std::endl;
		#ifndef NODEBUG	//compiler doesn't like it because it thinks a read only variable is having its value changed
		enableValidationLayers = false;
		#endif
	}


	VkApplicationInfo appInfo = {};														//information about the application -- technically optional
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";							//the name of the application
	appInfo.applicationVersion = VK_MAKE_VERSION(2, 0, 0);		//developer supplied version
	appInfo.pEngineName = "No Engine";												//the name of the engine used to create the application
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);					//developer supplied version for the engine -- there is no engine so this will aways stay as v1.0
	appInfo.apiVersion = VK_API_VERSION_1_0;									//the highest version of vulkan the application is designed to used -- not updated

	VkInstanceCreateInfo createInfo = {};														//tells the vulkan driver which global extensions and validation layers to use
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;													//specifing the application info set above


	auto extensions = getRequiredExtensions();																		//helper function to get the required global extensions needed by glfw and needed for validation layers to work
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());	//the number of extensions
	createInfo.ppEnabledExtensionNames = extensions.data();												//the string's of extensions




	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());	//the number of validatio layers to use
		createInfo.ppEnabledLayerNames = validationLayers.data();												//the const char*'s that specify what validation layers to use -- specified in the main header

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		populateDebugMessengerCreateInfo(debugCreateInfo);												//helper function to fill the necessary struct to determine what triggers a validation layer and what to do when it is triggered
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;	//the data the helper function created
	}	else {
		createInfo.enabledLayerCount = 0;		//if validation layers are not used then there are 0 validation layers
		createInfo.pNext = nullptr;					//nothing to say if a validation layer is triggered because there is non
	}

	if (!vk_check_result(vkCreateInstance(&createInfo, nullptr, &instance))) {	//creating the instance with settings 'createInfo' -- instance is defined in the main header
																																						//the nulltpr is for a function callback
		throw std::runtime_error("failed to create instance!");
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Instance creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif

}

void vulkanApp::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	//helper function used in createInstance(..) and also to setup the debug messenger
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; //bit mask of the severty of events that will cause the callback -- only one missing is VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT; //bitmask of what type of events will cause the callback function to be used -- all are used
	createInfo.pfnUserCallback = debugCallback;		//the callback function to call when a validation layer throws an error
}

void vulkanApp::setupDebugMessenger() {
	if (!enableValidationLayers) return; //debugging here referres to validation layers -- no need to set up events around validation layers where there are none

	#ifdef detailed_timing
		//after intial check because it will take no time and results is just a return
		auto start = std::chrono::steady_clock::now();
	#endif

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo); //helper function to fill the struct

	if (!vk_check_result(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger))) { 	//create the debug messenger for the message severity and message type defined in 'populateDebugMessengerCreateInfo'
																																																			//which will call the function 'debugCallback'
																																																			//the 'CreateDebugUtilsMessengerEXT' is defined in the class
																																																			//see https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers for how this works becuse i dont really understand
		throw std::runtime_error("failed to set up debug messenger!");
	}
	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Debug messenger creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;

	#endif
}

void vulkanApp::createSurface() {
	//====================================================================================================================================================================================
	//vulkan cannot be directly interfacted with the window system on its own.
	//To make a connection between Vulkan and the window system a WSI
	// - the first one is VK_KHR_surface (enabled in main header) which exposes a VKsurface object
	//the surface is used to present rendered images to
	//---------------------------------------------------------------------------------------------------
	//the surface used here will be taken directly from glfw
	//====================================================================================================================================================================================
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	if (!glfw_check_result(glfwCreateWindowSurface(instance, window, nullptr, &surface))) {
		//creating the surface using glfw -- can be done in raw vulkan but doesn't make much sense because glfw is being used
		//the surface creation handles different platforms on its own
		throw std::runtime_error("failed to create window surface!");
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Surface Creation took \t\t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}

void vulkanApp::pickPhysicalDevice() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); //finding the number of physical devices in the system with vulkan support

	if (deviceCount == 0) {		//a device that can run vulkan is of course needed
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
		//COME BACK TO LATER -- use another graphics api
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);	//a vector of all the devices
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());	//filling the vector

	#ifdef gore_detail
		//logging all devices capable of running vulkan
		//this is in gore_detail because it is not necessary to know the vast majority of the time
		VkPhysicalDeviceProperties physicalDeviceProperties;
		std::clog << "Possible physical devices: " << std::endl;
		for (const auto& current_device : devices) {
			//seems to find the 1 graphics card twice?
			vkGetPhysicalDeviceProperties(current_device, &physicalDeviceProperties);
			std::clog << "  " << indentifyDevice(physicalDeviceProperties.deviceType) << " " << physicalDeviceProperties.deviceName << std::endl;
		}
		std::clog << std::endl;;	//for formatting
	#endif

	std::multimap<int, VkPhysicalDevice> candiateDevices;	//maps because it will automatically sort the devices by which one is move favourable
	for (const auto& current_device : devices) {
		if (isDeviceSuitable(current_device)) {
			candiateDevices.insert(std::make_pair(rateDeviceSuitability(current_device), current_device));
		}
	}

	if (candiateDevices.rbegin()->first > 0) {
		physicalDevice = candiateDevices.rbegin()->second;
	} else {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	#ifdef gore_detail
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

		std::clog << "Available device extensions:" << std::endl;
		for (const auto& extension : availableExtensions) {
			std::clog << "  " << extension.extensionName << std::endl;
		}
		std::clog << std::endl;
	#endif

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Physical Device selection took \t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}

int vulkanApp::rateDeviceSuitability(VkPhysicalDevice pdevice) {
	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	vkGetPhysicalDeviceFeatures(pdevice,  &physicalDeviceFeatures);

	if (!physicalDeviceFeatures.geometryShader) {
		return 0;
	}
	if (!physicalDeviceFeatures.fillModeNonSolid) {
		return 0;
	}

	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(pdevice, &physicalDeviceProperties);

	int score = 0;

	if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {	//higher ratings are given for generally better devices
		score += 1000;
	} else if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
		score += 700;
	} else if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
		score += 200;
	}

	score += physicalDeviceProperties.limits.maxImageDimension2D;
	score += physicalDeviceProperties.limits.maxFramebufferWidth;
	score += physicalDeviceProperties.limits.maxFramebufferHeight;
	//probably should add more values to test for

	if (!physicalDeviceFeatures.wideLines) {
		score /= 1.2;
	}
	if (!physicalDeviceFeatures.largePoints) {
		score /= 1.2;
	}
	//wide lines and large points are preferred

	return score;
}

void vulkanApp::createLogicalDevice() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice); //finding the indices of the graphics queue and present queue
																																	//more indepth explaination is found inside to code for the function

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };	//the queue families to be set

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {		//filling the 'queueCreateInfos' structs
																												//this information is then passed to 'createInfo' which is used to create the logical device
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;		//the index of the queue family required
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;	//this is required to set priorities (between 0 and 1) to queues to influence their scheduling of command buffer execution
																												//this is not used used here so all priorities are set to 1
		queueCreateInfos.push_back(queueCreateInfo);
	}


	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;


	//the queues, device extensions, and validation layers and have all been check to see if they are available in 'isDeviceSuitable' which is called when picking the phyiscal device
	//here these are actually being enabled
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();														//the requied queues -- currently present and graphics

	VkPhysicalDeviceFeatures deviceFeatures = {};		//these device properties availablity should be checked in 'isDeviceSuitable' which is called in the picking of physical devices
																									//here it is acutally enabling the features
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.logicOp = VK_TRUE;
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();											//the required device extensions available in 'deviceExtensions' in the main header

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();											//the required validation layers set in 'validationLayers' in the main header
																																									//these are not required to be set here becuase they are set in instance creation, but previous vulkan versions made a distinction between instance extensions and device extensions - not anymore
	}
	else {
		createInfo.enabledLayerCount = 0;			//no validation layers if validation layers are not available
	}

	if (!vk_check_result(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device))) { //creating 1 logical device for the physical device
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);	//retrieving the queue handles for each queue family which is used to refenece the queues throughout the program
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Logical device creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}

void vulkanApp::createSwapChain() {
	//========================================================================================================================================================================
	//vulkan has no default framebuffer so it requires and infastructe that will own the buffers and render to them before said buffers get pushed to the screen
	//this is known as the swap chain
	//the swap chain is a queue of images that are waiting to be presented to the screen
	//------------------------------------------------------------------------------------------------------------------------------------------------------------
	//this application will acquire and image to draw to, then return it to the queue when it has been drawn to
	//========================================================================================================================================================================
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif


	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice); //all possible formats and color space pairs, and the presentatation modes available on the phyiscal device
																																										//SwapChainSupportDetails is defined in the main header

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);	//choosing the surface mode based on settings in the vk_settings namespace
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);	//choosing the present mode based on whether vsync is defined or not
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);									//setting the extent to the size of the window (or some other values in special cases)


	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;	//how many images does the swap chain have
																																					//recommended to request 1 more than the minimum
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {	//checking to make sure not ove rthe maximum and not less than 0 (0 means no max)
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	//filling the struct with the variables defined above
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;	//the amount of layers an image consists of -- 1 unless stereoscopic 3D application
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
	#ifdef naive_recording
		| VK_IMAGE_USAGE_TRANSFER_SRC_BIT
	#endif
	;																							//operations the images in the swap chain are used for
																																//here they are directly rendered to
																																//it is possible to to render to seperate images to perform post-processing and similar
																																// - VK_IMAGE_USAGE_TRANSFER_DST_BIT would then be used and memory operations used to transfer to the swap chain

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);	//returning the index of the graphics and present families
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {	//if they are 2 different queues
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;	//to avoid having to do ownership transferrs, images are sharded across multiple queue families -- NEED TO BE DONE
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {	//if they are the same queue
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;	//image is owned by one queue -- only is 1 queue so this is a good option
																															//this also offers the best performance
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;	//to specify any sort of rotations or flips
																																						//because that is not needed the current transform is used
																																						//if it were to be used, supportedTransforms would have to be checked in capabilities

	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;			//if the alpha channel should be used for blending with other windows in the window system
																																			//almost always should be opaque

	createInfo.presentMode = presentMode;
	createInfo.clipped = vk_settings::swapChainClipped;		//(if true) don't care about pixels that are obscured by another window infront or similar

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (!vk_check_result(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain))) { //creating the swap chain
		throw std::runtime_error("failed to create swap chain!");
	}


	//filling 'swapChainImages' with the images of the swapChain
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Swap chain creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}

__attribute__((pure)) VkSurfaceFormatKHR vulkanApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	//chosing a swapchain with desired properties

	//the VkSurfaceFormatKHR has 2 members - format and colorspce, so checking if those match what is desired
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == vk_settings::swapChainFormat && availableFormat.colorSpace == vk_settings::swapChainColorSpace) {
			return availableFormat;

		}
	}

	return availableFormats[0];	//return the first one if non have the desired features -- should do more here because this is not great -- should rank based on how good they are and return the best one
}

__attribute__((pure)) VkPresentModeKHR vulkanApp::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	//present mode describes the conditions for showing images to screen
	//there are 4 possiblites : VK_PRESENT_MODE_IMMEDIATE_KHR (straight from application to screen)
	//												:	VK_PRESENT_MODE_FIFO_KHR (vsync - the program has to wait if the queue is full)
	//												:	VK_PRESENT_MODE_FIFO_RELAXED_KHR (same as above but if the queue is empty display the last image rather than waiting)
	//												:	VK_PRESENT_MODE_MAILBOX_KHR (vsync - the program replaces old images if queue is full)

	for (const auto& availablePresentMode : availablePresentModes) {	//looking through all modes to see which one matches what is desired
		#ifdef vsync
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) { 	//used because it allows for triple buffering - is what will end up looking the best
			return availablePresentMode;
		}
		#else
		if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {	//used becasue it is the only one that does not sync with the screen - used main to test performance
			return availablePresentMode;
		}
		#endif
	}

	return VK_PRESENT_MODE_FIFO_KHR; //the only one that is guaranteed
}

VkExtent2D vulkanApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	//the swap extent is the resolution of the swap chain images -- almost always the resolution of the window being drawn to
	if (capabilities.currentExtent.width != UINT32_MAX) {	//some window managers allow for the extent to differ from the window - dealing with this special case
		return capabilities.currentExtent;	//no idea how this works
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);	//grabbing the size of the window to set the extent to
																											//this is initally the values of WIDTH, and HEIGHT defined in the main header

		VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };	//filling the necessary data type with the data

		//clamping the values between allowed max and min
		// - likely a better way of doing this
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));


		return actualExtent;
	}
}

SwapChainSupportDetails vulkanApp::querySwapChainSupport(VkPhysicalDevice p_device) {
	//this function queries the formats and color space pairs, and the presentatation modes available for the surface supplied by GLFW

	//SwapChainSupportDetails is defined in the main header
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device, surface, &details.capabilities); //queries the basic capabilities of the surface needed to create the swapchain at all

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, surface, &formatCount, nullptr); //querying the number of surface formats and color space pairs

	if (formatCount != 0) {	//extra check because suitablity checks are based on if details.formats has size 0
													//also 'vkGetPhysicalDeviceSurfaceFormatsKHR' is reasonably slow
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, surface, &formatCount, details.formats.data());	//getting the surface formats and color space pairs
	}


	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(p_device, surface, &presentModeCount, nullptr);		//querying the number presentation modes for a surface

	if (presentModeCount != 0) {	//extra check because suitablity checks are based on if details.formats has size 0
																//also 'vkGetPhysicalDeviceSurfacePresentModesKHR' is reasonably slow
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(p_device, surface, &presentModeCount, details.presentModes.data());		//getting the presentatation modes for the surface
	}

	return details;
}

void vulkanApp::createImageViews() {
	//an image view describes how to access an image and which part of the image to access
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	swapChainImageViews.resize(swapChainImages.size());	//each image has its own view by implementation

	//to fast to parallise -- just filling structs \/
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		swapChainImageViews[i] = createImageView(device, swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);	//create image view is defined in "vulkan_help"
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Image View creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif

}

void vulkanApp::createRenderPass() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;


	if (!vk_check_result(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass))) {
		throw std::runtime_error("failed to create render pass!");
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Render Pass creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}

void vulkanApp::createDescriptorSetLayout() {
	//A descriptor is a way for shaders to freely access resources like buffers and images
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	//Every binding needs to be described through a VkDescriptorSetLayoutBinding struct
	VkDescriptorSetLayoutBinding uboLayoutBinding = uboDescriptorLayoutBinding(0);					//for model, view, proj
	VkDescriptorSetLayoutBinding samplerLayoutBinding = samplerDescriptorLayoutBinding(1);	//for texture
	#ifdef testing2
	VkDescriptorSetLayoutBinding lightingLayoutBinding = uboDescriptorLayoutBinding(2);					//lighting test
	#endif

	#ifdef testing2
	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, samplerLayoutBinding, lightingLayoutBinding };
	#else
	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	#endif

	//all the descriptor bindings are combined into a single VkDescriptorSetLayout object
	//this is created using vkCreateDescriptorSetLayout which requires VkDescriptorSetLayoutCreateInfo
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (!vk_check_result(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout))) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Descriptor set layout creation took \t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif

}

void vulkanApp::createGraphicsPipeline() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	//shader modules are a thin wrapper around the shader bytecode
	#ifdef testing1
		#ifdef testing2
			VkShaderModule vertShaderModule = GB::shaderModule(device, std::string(global::asset_dir)+"/shaders/test2_vert.spv");
			VkShaderModule fragShaderModule = GB::shaderModule(device, std::string(global::asset_dir)+"/shaders/test2_frag.spv");
		#else
			VkShaderModule vertShaderModule = GB::shaderModule(device, std::string(global::asset_dir)+"/shaders/test_vert.spv");
			VkShaderModule fragShaderModule = GB::shaderModule(device, std::string(global::asset_dir)+"/shaders/test_frag.spv");
		#endif
	#else
		VkShaderModule vertShaderModule = GB::shaderModule(device, std::string(global::asset_dir)+"/shaders/simple_vert.spv");
		VkShaderModule fragShaderModule = GB::shaderModule(device, std::string(global::asset_dir)+"/shaders/simple_frag.spv");
	#endif

	///to acctually use shaders, they need to be assigned to a specific pipeline stage
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = GB::createVertexShaderInfo(vertShaderModule);
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = GB::createFragmentShaderInfo(fragShaderModule);

	//array that will be used to reference the shader objects in the pipeline creation
	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = GB::vertexInfoInput(&bindingDescription, &attributeDescriptions);

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = GB::assemblyInput(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	VkViewport viewport = GB::createViewport(0.0f, 0.0f, 1.0f, 1.0f, swapChainExtent);

	VkRect2D scissor = GB::createScissor(0.0f, 0.0f, 1.0f, 1.0f, swapChainExtent);


	VkPipelineViewportStateCreateInfo viewportState = GB::createViewportState(&viewport, &scissor);

	VkPipelineRasterizationStateCreateInfo rasterizer = GB::createRasterizer(VK_POLYGON_MODE_FILL, vk_settings::cull_mode, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	VkPipelineMultisampleStateCreateInfo multisampling = GB::multisamplingInfo();

	VkPipelineDepthStencilStateCreateInfo depthStencil = GB::depthStencilInfo();

	VkPipelineColorBlendAttachmentState colorBlendAttachment = GB::colorBlendAttachmentInfo();



	VkPipelineColorBlendStateCreateInfo colorBlending = GB::colorBlendStateInfo(&colorBlendAttachment);


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = GB::createPipelineLayoutInfo(&descriptorSetLayout);

	if (!vk_check_result(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout))){
			throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;

	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (!vk_check_result(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline))) {
			throw std::runtime_error("failed to create graphics pipeline!");
	}


	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Graphics pipeline creation took \t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}

void vulkanApp::createFramebuffers() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = { swapChainImageViews[i], depthImageView };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (!vk_check_result(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]))) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Framebuffer creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}

void vulkanApp::createCommandPool() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0;

	if (!vk_check_result(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool))) {
		throw std::runtime_error("failed to create command pool!");
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Command Pool creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}



void vulkanApp::createDepthResources() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	VkFormat depthFormat = findDepthFormat();

	createImage(device, physicalDevice, swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	depthImageView = createImageView(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t DepthResource creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}

VkFormat vulkanApp::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat vulkanApp::findDepthFormat() {
	return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}



void vulkanApp::createTextureImage() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif


/*
#pragma omp parallel for
	for (int i = 0; i < no_images; i++) {
		imagePixels[i].read_file(image_locations[i]);
	}*/

//#pragma omp parallel for
	for (uint32_t i = 0; i < no_mesh; i++) {
		staticMeshes[i].createTexture(imagePixels[i], device, physicalDevice, commandPool, graphicsQueue);
	}

//#pragma omp parallel for
	for (uint32_t i = no_mesh; i < no_mesh + no_m_mesh; i++) {
		uint32_t j = i - no_mesh;
		movingMeshes[j].createTexture(imagePixels[i], device, physicalDevice, commandPool, graphicsQueue);
	}
	/*
#pragma omp parallel for
	for (int i = 0; i < no_images; i++) {
		Squares[i].TextureCleanupCreation(device);
	}
	*/

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Texture Image creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}


void vulkanApp::createTextureImageView() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	for (uint32_t i = 0; i < no_mesh; i++) {
		staticMeshes[i].createTextureImageView(device);
	}

	for (uint32_t i = 0; i < no_m_mesh; i++) {
		movingMeshes[i].createTextureImageView(device);
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Texter Image View creation took \t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}



void vulkanApp::createTextureSampler() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (!vk_check_result(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler))) {
		throw std::runtime_error("failed to create texture sampler!");
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Texture Sampler creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}



void vulkanApp::createVertexBuffer() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

//#pragma omp parallel for
	for (uint32_t i = 0; i < no_mesh; i++) {
		staticMeshes[i].createVertexBuffer(device, commandPool, graphicsQueue, physicalDevice);
	}

//#pragma omp parallel for
	for (uint32_t i = 0; i < no_m_mesh; i++) {
		movingMeshes[i].createVertexBuffer(device, commandPool, graphicsQueue, physicalDevice);
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Vertex Buffer Creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}


void vulkanApp::createIndexBuffer() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

//#pragma omp parallel for
	for (uint32_t i = 0; i < no_mesh; i++) {
		staticMeshes[i].createIndexBuffer(device, commandPool, graphicsQueue, physicalDevice);
	}

//#pragma omp parallel for
	for (uint32_t i = 0; i < no_m_mesh; i++) {
		movingMeshes[i].createIndexBuffer(device,commandPool, graphicsQueue, physicalDevice);
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Index Buffer Creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}

void vulkanApp::createUniformBuffers() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

//#pragma omp parallel for
	for (uint32_t i = 0; i < no_mesh; i++) {
		staticMeshes[i].createUb((uint32_t)swapChainImages.size(), device, physicalDevice);
	}

//#pragma omp parallel for
	for (uint32_t i = 0; i < no_m_mesh; i++) {
		movingMeshes[i].createUb(static_cast<uint32_t>(swapChainImages.size()), device, physicalDevice);
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Uniform Buffer Creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}

void vulkanApp::createDescriptorPool() {
	//Descriptor sets can't be created directly, they must be allocated from a pool

/*#pragma omp parallel for
	for (int i = 0; i < no_images; i++) {
		Squares[i].createDescriptorPool(device); //!not sure
	}*/
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	//! not sure if complete
	//describing what descriptor tyes the descriptor sets are going to contain

	std::array<VkDescriptorPoolSize,2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size() * (no_mesh + no_m_mesh)); //descriptor pool for every mesh in every image of the swap chain
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;																				//this is becasue every mesh has its own ubo and image sample
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size() * (no_mesh + no_m_mesh)); //this can be seen by looking into the code for the static meshes (not sure if better way)

	//the poolSize structure is referenced here
	VkDescriptorPoolCreateInfo poolInfo = {};																															//poolinfo contains the information about poolsizes
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;																				//including the <didn't end up finishing>
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size() * (no_mesh + no_m_mesh));

	//creating the descriptor pool
	if (!vk_check_result(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool))) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Descriptor Pool creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}

void vulkanApp::createDescriptorSets() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

//#pragma omp parallel for
	for (uint32_t i = 0; i < no_mesh; i++) {
		staticMeshes[i].createDescriptorSet(descriptorSetLayout, &descriptorPool, device, textureSampler);
	}

	for (uint32_t i = 0; i < no_m_mesh; i++) {
		movingMeshes[i].createDescriptorSet(descriptorSetLayout, &descriptorPool, device, textureSampler);
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Descriptor Set creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}

void vulkanApp::createCommandBuffers() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	commandBuffers.resize(swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (!vk_check_result(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()))) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (!vk_check_result(vkBeginCommandBuffer(commandBuffers[i], &beginInfo))) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[i];

		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

//#pragma omp parallel for //might be able to make work
		for (uint32_t j = 0; j < no_mesh; j++) {
			staticMeshes[j].draw(commandBuffers, pipelineLayout, i);
		}

		for (uint32_t j = 0; j < no_m_mesh; j++) {
			movingMeshes[j].draw(commandBuffers, pipelineLayout, i);
		}

		vkCmdEndRenderPass(commandBuffers[i]);


		if (!vk_check_result(vkEndCommandBuffer(commandBuffers[i]))) {
			throw std::runtime_error("failed to record command buffer");
		}

	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Command Buffer creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif

}

void vulkanApp::createSyncObjects() {
	#ifdef detailed_timing
		auto start = std::chrono::steady_clock::now();
	#endif

	imageAvailableSemaphores.resize(vk_settings::MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(vk_settings::MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(vk_settings::MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


	for (size_t i = 0; i < vk_settings::MAX_FRAMES_IN_FLIGHT; i++) {
		if (!vk_check_result(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i])) ||
			!vk_check_result(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i])) ||
			!vk_check_result(vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]))) {

			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}

	#ifdef detailed_timing
		auto end = std::chrono::steady_clock::now();
		std::cout << "\t Sync Object creation took \t\t" << std::chrono::duration <double, std::milli>(end - start).count() << "ms" << std::endl;
	#endif
}



bool vulkanApp::isDeviceSuitable(VkPhysicalDevice p_device) {
	QueueFamilyIndices indices = findQueueFamilies(p_device);	//need to select which queue families (the types of data which can be submitted to device and what types of commands)
																														//this function will look for the queue families required for the application to work correctly
		//anything from drawing to uploading textures requires commands to be submitted to a queue

	bool extensionsSupported = checkDeviceExtensionSupport(p_device);	//checks to make sure the requested device extensions stated in 'deviceExtensions' in the main header are supported by the device

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(p_device); 	//this returns all the surface format and color space pairs, and present modes supported
																																									//all that is really needed is the number of each supported but this function is used primarily to create the swapchain
																																									//SwapChainSupportDetails is defined in the main header

		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty(); 	//this checks to make sure that at least 1 of both are supported`
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(p_device, &supportedFeatures);	//used to get the features the device supports
																															//at the moment it only must suppory anisotropy

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy; //anisoptropy must be available because the program uses it
}

bool vulkanApp::checkDeviceExtensionSupport(VkPhysicalDevice p_device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(p_device, nullptr, &extensionCount, nullptr); //getting the number of supported extensions

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(p_device, nullptr, &extensionCount, availableExtensions.data()); //getting all supported extensions
																																																				//these are logged in standard output if gore_detail is defined

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());		//using std::set so erase can be called, then the set can be checked if empty

	for (const auto& extension : availableExtensions) {
		//for each available extension on the device, remove it from the list of requested extensions
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty(); 	//if all the requested extensions have been removed, then they were all available
}

QueueFamilyIndices vulkanApp::findQueueFamilies(VkPhysicalDevice p_device) {
	//=====================================================================================================================================================
	//this function finds the indices of the present and graphics queue families
	//it also checks support that the relevent physical device can support a graphics queue
	//	and if relevent physical device and the surface returned by GLFW can support a present queue
	//the indices are those returned by 'vkGetPhysicalDeviceQueueFamilyProperties' why various other vulkan functions require
	//=====================================================================================================================================================

	QueueFamilyIndices indices;	//a data type defiend in the main header which constains infomation about the graphics queue and the present queue

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(p_device, &queueFamilyCount, nullptr); //retrieving the number of queue families for the device (the number of unique types of queues)

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount); //possible queue families are graphics, compute, transfer, sparse binding, or poteected
																																				//does not include presentation queue becuase that depends on the surface being presented to
	vkGetPhysicalDeviceQueueFamilyProperties(p_device, &queueFamilyCount, queueFamilies.data()); //retriving a list of queue families

	uint32_t i = 0; //this contains the index of the queue family which is needed for various functions
									//value would likely never reach anything large enough to justify storing it is a 32 bit integer, execpt this the type that the functions want it to be in
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;	//returning the index of the queue family -- this is needed for various functions
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(p_device, i, surface, &presentSupport); //checking whether a queue family (accessed by its index) has support for presentation to a given surface

		if (presentSupport) {
			indices.presentFamily = i;	//returning the index of the queue family -- this is needed for various functions
		}

		if (indices.isComplete()) {	//isComplete is a function that returns true if a value has been set for all the queues it stores
																//if all the queues needed are found there is no need to search through the rest
			break;
		}

		i++;
	}

	if (!indices.isComplete()) {	//if the relevent queues do not have an index assigned to them
		std::cerr << "Failed to find the queue families requested" << std::endl; //not sure if this needs to be a runtime error
	}

	return indices;	//returning the indices of the relevent queue families
}

std::vector<const char*> vulkanApp::getRequiredExtensions() {
	//helper function that gets the required instance extensions -- used in createInstance(..)

	//getting the GLFW required extensions -- these must be available for the application to work the way it is designed
	// - it would be possible to still get a lot of functionality if these were not available using offscreen rendering but the main purpose of this application is to be onscreen
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount /*pointer arithmatic*/); 	//converting the char* array to a vector of char* for easy addition of other validation layers
																																																										//this also makes it easy to pass the results into the necessary struct

	if (enableValidationLayers) {	//the instance extensions required for validation layers
		extensions.push_back("VK_EXT_debug_utils");
	}

	//getting the available extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> instance_extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instance_extensions.data());

	//checking to make sure the instance extensions requested are available
	bool layerFound;
	for (const char* layerName : extensions) { //for all required extensions
		layerFound = false;

		for (const auto& extension : instance_extensions) { //go through all available extensions until the required extension is found
			if (strcmp(layerName, extension.extensionName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {	//if the required extension is not in the list of available extensions
			throw std::runtime_error("Required instance extensions are not available - specifcally could not find " + (std::string)layerName);
		}
	}


	#ifdef gore_detail
		std::clog << "Available instance extensions:" << std::endl;

		for (const auto& extension : instance_extensions) {
			std::clog << "  " << extension.extensionName << std::endl;
		}
		std::clog << std::endl;
	#endif

	//extensions.push_back("VK_EXT_display_surface_counter"); //the values for these are shown if gore_detail is defined

	return extensions;
}

bool vulkanApp::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr); //get the number of available validation layers

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); //fill the vector with the validation layers

	#ifdef gore_detail
		std::clog << "Available validation layers:" << std::endl;
		for (const auto& layers : availableLayers) { //printing out all validation layers that are available
			std::clog << "  " << layers.layerName << std::endl;
		}
		std::clog << std::endl;
	#endif

	bool layerFound;
	for (const char* layerName : validationLayers) { //go through all requested validation layers -- 'validationLayers' is defined in the main header
		layerFound = false;

		for (const auto& layerProperties : availableLayers) {	//iterate through all available layers until it matches the requested layer
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) { //the validation layers request are not available
			return false;
		}
	}

	return true;
}

#ifdef naive_recording
	void vulkanApp::take_screenshot(const char* filename) {
		bool supportsBlit = true;

		//check blit support for source and destination
		//=============================================
		VkFormatProperties formatProps;

		//Check if the device supports blitting for optimal images
		vkGetPhysicalDeviceFormatProperties(physicalDevice, vk_settings::swapChainFormat, &formatProps);
		if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
			//std::cerr << "Device does not support blitting from optimal tiled images" << std::endl;
			supportsBlit = false;
		}

		//Check if the device supports blitting to linear images
		vkGetPhysicalDeviceFormatProperties(physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
		if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
			supportsBlit = false;
		}

		//===========================================
		//source for the copy is the last rendere swapchain image
		VkImage srcImage = swapChainImages[currentFrame];

		//Create the linear tiled destination image to copy to and to read the memory from
		VkImageCreateInfo imageCreateCI {};
		imageCreateCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateCI.imageType = VK_IMAGE_TYPE_2D;
		imageCreateCI.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageCreateCI.extent.width = swapChainExtent.width;
		imageCreateCI.extent.height = swapChainExtent.height;
		imageCreateCI.extent.depth = 1;
		imageCreateCI.arrayLayers = 1;
		imageCreateCI.mipLevels = 1;
		imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateCI.tiling = VK_IMAGE_TILING_LINEAR;
		imageCreateCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		//create the image
		VkImage dstImage;
		vk_check_result(vkCreateImage(device, &imageCreateCI, nullptr, &dstImage));

		//create memory to back up the image
		VkMemoryRequirements memRequirements;
		VkMemoryAllocateInfo memAllocInfo {};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkDeviceMemory dstImageMemory;
		vkGetImageMemoryRequirements(device, dstImage, &memRequirements);
		memAllocInfo.allocationSize = memRequirements.size;
		//testing -- getting memory type
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		const VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
			if ((memRequirements.memoryTypeBits & 1) == 1) {
				if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					memAllocInfo.memoryTypeIndex = i;
				}
			}
			memRequirements.memoryTypeBits >>= 1;
		}
		//need error checking
		//end testing

		vk_check_result(vkAllocateMemory(device, &memAllocInfo, nullptr, &dstImageMemory));
		vk_check_result(vkBindImageMemory(device, dstImage, dstImageMemory, 0));

		//Do the actual blit from the swapchain image to our host visible destination image
		VkCommandBufferAllocateInfo cmdBufAllocateInfo {};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = commandPool;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer copyCmd;
		vk_check_result(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));

		VkCommandBufferBeginInfo cmdBufInfo {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vk_check_result(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));

		//trasition destination image to transfer destination layout
		{
		VkImageMemoryBarrier imageMemoryBarrier {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.image = dstImage;
		imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		//transition swapchian image from present to transfer source layout
		{
		VkImageMemoryBarrier imageMemoryBarrier {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageMemoryBarrier.image = srcImage;
		imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		if (supportsBlit) {
			//define whole region to blit
			VkOffset3D blitSize;
			blitSize.x = swapChainExtent.width;
			blitSize.y = swapChainExtent.height;
			blitSize.z = 1;

			VkImageBlit imageBlitRegion {};
			imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlitRegion.srcSubresource.layerCount = 1;
			imageBlitRegion.srcOffsets[1] = blitSize;
			imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlitRegion.dstSubresource.layerCount = 1;
			imageBlitRegion.dstOffsets[1] = blitSize;

			//issue the blit command
			vkCmdBlitImage(copyCmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlitRegion, VK_FILTER_NEAREST);
		} else {
			//use image copy
			VkImageCopy imageCopyRegion {};
			imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopyRegion.srcSubresource.layerCount = 1;
			imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopyRegion.dstSubresource.layerCount = 1;
			imageCopyRegion.extent.width = swapChainExtent.width;
			imageCopyRegion.extent.height = swapChainExtent.height;
			imageCopyRegion.extent.depth = 1;
			vkCmdCopyImage(copyCmd,srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
		}

		//Transition destination image to general layout, which is the required layout for mapping the image memory later on
		{
		VkImageMemoryBarrier imageMemoryBarrier {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier.image = dstImage;
		imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		//Transition back the swap chain image after the blit is done
		{
		VkImageMemoryBarrier imageMemoryBarrier {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier.image = srcImage;
		imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}


		//flushing command buffer
		vk_check_result(vkEndCommandBuffer(copyCmd));

		VkSubmitInfo submitInfo {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyCmd;

		//Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fenceInfo {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = 0;//VK_FENCE_CREATE_SIGNALED_BIT;//VK_FLAGS_NONE;
		VkFence fence;
		vk_check_result(vkCreateFence(device, &fenceInfo, nullptr, &fence));

		//submit to the queue
		vk_check_result(vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence));	//not sure if graphics queue
		//wait for the fence to signal that command buffer has finished execting
		vk_check_result(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX/*DEFAULT_FENCE_TIMEOUT*/));

		vkDestroyFence(device, fence, nullptr);

		vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);

		//Get layout of the image
		VkImageSubresource subResource { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
		VkSubresourceLayout subResourceLayout;
		vkGetImageSubresourceLayout(device, dstImage, &subResource, &subResourceLayout);

		//Map image memory so we can start copying from it
		const char* data;
		vkMapMemory(device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
		data+= subResourceLayout.offset;

		std::ofstream file(filename, std::ios::out | std::ios::binary);

		//ppm header
		file << "P6\n" << swapChainExtent.width << "\n" << swapChainExtent.height << "\n" << 255 << "\n";

		// If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
		bool colorSwizzle = false;
		//not complete
		if (!supportsBlit) {
			std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
			colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), vk_settings::swapChainFormat) != formatsBGR.end());
		}

		//ppm binary pixel data
		for (uint32_t y = 0; y < swapChainExtent.height; y++) {
			unsigned *row = (unsigned*)data;
			for (uint32_t x = 0; x < swapChainExtent.width; x++) {
				if (colorSwizzle) {
					file.write((char*)row+2, 1);
					file.write((char*)row+1, 1);
					file.write((char*)row, 1);
				} else {
					file.write((char*)row, 3);
				}
				row++;
			}
			data += subResourceLayout.rowPitch;
		}
		file.close();

		//clean up
		vkUnmapMemory(device, dstImageMemory);
		vkFreeMemory(device, dstImageMemory, nullptr);
		vkDestroyImage(device, dstImage, nullptr);

	}
#endif
