#pragma once
#include <cmath>

#define GRADIENT " .-:*+%@$#"

namespace painter {
  void render (glm::uint8** canvas, int& width, int& height)
  {
    for (int j = height-1; j >= 0; j--)
    {
        for (int i = 0; i < width; i++)
        {
            int gr_frag = canvas[j][i] * (strlen(GRADIENT) - 1) / 255;
            std::cout << GRADIENT[gr_frag];
        }
        if (j > 0) std::cout << "\n";
    }
  }
}
