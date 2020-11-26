#include <iostream>
#include <glm/glm.hpp>
#include <fstream>

#include "dependencies/camera.h"
#include "dependencies/examples.h"

const uint32_t no_frames = 60 * 10;

//Camera_Generation::Camera main_camera(Camera_Generation::Camera::get_main_output(), no_frames);
Camera_Generation::Camera main_camera(no_frames);

namespace Camera_Examples = Camera_Generation::Examples;

const unsigned frames_for_rotation = 60 * 5;

int main() {
  //main_camera.set_positions(Camera_Examples::pos::stationary(glm::vec3(0,0,0), main_camera.no_frames));
  //main_camera.set_pitch(Camera_Examples::pitch::stationary(0, main_camera.no_frames));
  //main_camera.set_yaw(Camera_Examples::yaw::unif_rotate(frames_for_rotation, main_camera.no_frames));

  main_camera.write("/home/george/Documents/Projects/Major-1/Assets/Camera/",
                    Camera_Examples::pos::stationary(glm::vec3(0,0,0), main_camera.no_frames),
                    Camera_Examples::pitch::stationary(0, main_camera.no_frames),
                    Camera_Examples::yaw::unif_rotate(frames_for_rotation, main_camera.no_frames));

  return 0;
}
