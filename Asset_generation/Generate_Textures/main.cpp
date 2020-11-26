#include <iostream>
#include <array>

//#include "dependencies/bitmap.h"
#include "dependencies/gradients.h"
#include "dependencies/main_writing.h"

const int image_width = 100, image_height = 100;  //used only for testing

namespace texture_examples = texture_generation::examples;

int main() {
  /*texture_generation::bitmaps_t images(texture_generation::bitmaps_t::get_main_output());

  images.add_Bitmap(texture_examples::gradient1(image_width / 2, image_height / 2));
  images.add_Bitmap(texture_examples::gradient2(image_width, image_height));
  images.add_Bitmap(texture_examples::gradient3(image_width, image_height));
  images.add_Bitmap(texture_examples::gradient4(image_width, image_height));
  images.add_Bitmap(texture_examples::gradient5(image_width, image_height));

  //images.write_images_png();
  images.write_images_bin();*/

  texture_generation::bitmap_t image1 = texture_examples::gradient1(image_width / 2, image_height / 2);
  texture_generation::bitmap_t image2 = texture_examples::gradient2(image_width, image_height);
  texture_generation::bitmap_t image3 = texture_examples::gradient3(image_width, image_height);
  texture_generation::bitmap_t image4 = texture_examples::gradient4(image_width, image_height);
  texture_generation::bitmap_t image5 = texture_examples::gradient5(image_width, image_height);

  std::array<texture_generation::bitmap_t,4> images_s = {image1, image2, image3, image4};
  write_bin("textures_s_", images_s, "/home/george/Documents/Projects/Major-1/Assets/textures/");

  std::array<texture_generation::bitmap_t,1> images_m = {image5};
  write_bin("textures_m_", images_m, "/home/george/Documents/Projects/Major-1/Assets/textures/");


  //images.write_data();

  return 0;
}
