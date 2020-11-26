#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace texture_generation {
  struct bitmap_t {
    const unsigned channels = 4; //r,g,b,a
    uint8_t* pixels;
    unsigned width;
    unsigned height;


    bitmap_t(const size_t width_, const size_t height_) : height(height_), width(width_){
      pixels = new uint8_t[width_*height_*channels];
    }



    uint8_t* pixel_at(int x, int y) {
      //a pointer to the pixles moved to the position given
      return pixels + (width * y + x) * channels;
    }

    //size_t width;
    //size_t height;


    //bool generate_png(std::string path);

    inline void set_pixel(const int x, const int y, const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t alpha) {
      uint8_t* pixel = pixel_at(x, y);
      *pixel++ = red;
      *pixel++ = green;
      *pixel++ = blue;
      *pixel = alpha;
    }

    /*inline void update_size() {
      pixels = new uint8_t[width * height * 4];
    }

    inline void set_dim(const size_t w, const size_t h) {
      width = w;
      height = h;
      update_size();
    }*/

    inline void cleanup() {
      delete [] pixels;
    }

    //void generate_bin(std::string path);


  };
}
