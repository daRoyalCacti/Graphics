#pragma once
#include "static_simple_mesh.h"
#include "../vulkan_general/vertex.h"
#include <vector>
#include <iostream>
#include "../global.h"

struct moving_simple_mesh : public staticSimpleMesh
{
  //this is a thin wrapper around the static mesh to make using animated meshes easier to code
  //a lot of the initial setup is the same - just calling .Mesh.function() -- the only difference is for the 2 functions defined below
  //interpolation between frames might be added at somepoint
  bool modulous = false; //what to do if a frame of mesh being called is greater than the number of frames available
                        //true will make the animation repeat, false will make only the last frame show once completed
  unsigned frames = 1; //number of frames of animation - 1 frame makes it act as a static mesh

  moving_simple_mesh() {};
  template <typename T> //<- haven't tested to see if it works correctly
  moving_simple_mesh(bool _modulous, T _frames = 1) : modulous(_modulous), frames(_frames){}
  std::vector<std::vector<Vertex>> Meshes; // this stores all the frames of animation
                                          // it might be better to store only the differences in frames to save on memory

	inline void createVertexBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkPhysicalDevice physicalDevice){
    //this function is a work around -- as seen in the line below
    vertices = Meshes[0]; //<- is a work around and needs to be updated
  	staticSimpleMesh::createVertexBuffer(device, commandPool, graphicsQueue, physicalDevice);
  }

  template <typename T> //<- haven't tested to see if it works correctly
	inline void updateVertexBuffer(VkDevice device, T frameCounter, VkCommandPool commandPool, VkQueue graphicsQueue){
    //----------------------------------------------------------------------------------------------------------
    //the function is used to make drawing the animated meshes easier
    //there is no need to manually add a check to make sure the animation frame being requested is available
    // - this function will do this check in a way that is determined by the "modulous" variable
    //---------------------------------------------------------------------------------------------------------
   if (frameCounter >= frames) {
     if (modulous) {
       frameCounter %= (frames); // this is what will force the animation to repeat once it has finished
     } else {
       return; //no need to update the mesh because its last frame has been drawn
     }
   }
   staticSimpleMesh::updateVertexBuffer(device, Meshes[frameCounter], commandPool, graphicsQueue);
  }

};
