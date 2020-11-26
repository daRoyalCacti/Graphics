#include "bitmap.h"
#include <fstream>

template <typename T>
inline void write_bin(const std::string file_title, const T bitmaps, const std::string path) {
  std::vector<uint32_t> bitmapSizes;
  bitmapSizes.resize(bitmaps.size()*2);

  for (unsigned k = 0; k < bitmaps.size(); k++) {
    //writing the actual image to file
    std::ofstream output(path + file_title + std::to_string(k) + ".bin", std::ios::binary);
    output.write((char*)&bitmaps[k].pixels[0], (bitmaps[k].width * bitmaps[k].height * bitmaps[k].channels) * sizeof(uint8_t));
    output.close();

    //generating data to be written
    bitmapSizes[2*k] = bitmaps[k].width;
    bitmapSizes[2*k + 1] = bitmaps[k].height;
  }

  //writing data
  std::ofstream data(path + file_title+"data.bin", std::ios::binary);
  uint32_t no_images = bitmaps.size();
  data.write((char*)&no_images, sizeof(uint32_t));
  data.write((char*)&bitmapSizes[0], sizeof(uint32_t) * bitmapSizes.size());

  data.close();

}
