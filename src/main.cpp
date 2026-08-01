#include <iostream>
#include <stdlib.h>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/ioctl.h>
#include <unistd.h>
#include "linal.cpp"
#include "painter.cpp"
#include "rasterizer.cpp"

//#define WIDTH 80
//#define HEIGHT 40

int main (int argc, char** argv)
{
    struct winsize w;
    ioctl(STDOUT_FILENO,TIOCGWINSZ, &w);

    int WIDTH = w.ws_col;
    int HEIGHT = w.ws_row;

    int SHADOW_MAP_WIDTH = 200;
    int SHADOW_MAP_HEIGHT = 200;

    std::vector<linal::Triangle> plane = {
        {
            {{0.2f,0.2f,0.0f}, {0.0f,0.0f,1.0f}}
           ,{{-0.2f,0.2f,0.0f}, {0.0f,0.0f,1.0f}}
           ,{{0.2f,-0.2f,0.0f}, {0.0f,0.0f,1.0f}}

        }
       ,{
            {{-0.2f,-0.2f,0.0f}, {0.0f,0.0f,1.0f}}
           ,{{0.2f,-0.2f,0.0f}, {0.0f,0.0f,1.0f}}
           ,{{-0.2f,0.2f,0.0f}, {0.0f,0.0f,1.0f}}
        }
    };

    std::vector<linal::Triangle> plane2 = {
        {
            {{0.5f,0.5f,-0.2f}, {0.0f,0.0f,1.0f}}
           ,{{-0.5f,0.5f,-0.2f}, {0.0f,0.0f,1.0f}}
           ,{{0.5f,-0.5f,-0.2f}, {0.0f,0.0f,1.0f}}

        }
       ,{
            {{-0.5f,-0.5f,-0.2f}, {0.0f,0.0f,1.0f}}
           ,{{0.5f,-0.5f,-0.2f}, {0.0f,0.0f,1.0f}}
           ,{{-0.5f,0.5f,-0.2f}, {0.0f,0.0f,1.0f}}
        }
    };
    std::vector<std::vector<linal::Triangle>> models = {plane, plane2};
    glm::mat4 model     = glm::mat4(1.0f);

    glm::vec3 camera_pos = glm::vec3(-1.0f,-5.0f,2.0f);
    glm::vec3 target    = glm::vec3(0.0f,0.0f,0.0f);
    glm::mat4 view      = glm::lookAt(camera_pos, target, glm::vec3(0.0f,0.0f,1.0f));

    float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    float vfov = 2.0f * atan(tan(glm::radians(40.0f) / 2.0f) / aspect);
    glm::mat4 proj = glm::perspective(vfov, aspect / 1.9f, 0.1f, 100.0f);

    // Shadow map stuff
    linal::Light light1;
    light1.pos = glm::vec3(5.0f,1.0f,3.0f);
    light1.intensity = 32.0f;
    light1.view = glm::lookAt(light1.pos, target, glm::vec3(0.0f,0.0f,1.0f));
    float shadow_map_aspect = SHADOW_MAP_WIDTH / (float)SHADOW_MAP_HEIGHT;
    light1.proj = glm::perspective(glm::radians(30.0f), shadow_map_aspect, 1.0f, 20.0f);

    light1.sm_width = SHADOW_MAP_WIDTH;
    light1.sm_height = SHADOW_MAP_HEIGHT;
    light1.shadow_map = (float**)calloc(light1.sm_height, sizeof(float*));
    for (int i = 0; i < SHADOW_MAP_HEIGHT; i++)
    {
      light1.shadow_map[i] = (float*)calloc(light1.sm_width, sizeof(float));
      std::fill(light1.shadow_map[i], light1.shadow_map[i] + light1.sm_width, 1.0f);
    }

    size_t light_num = 1;
    linal::Light lights[1] = {light1};

    glm::uint8** canvas = (glm::uint8**)calloc(HEIGHT, sizeof(glm::uint8*));
    for (int i = 0; i < HEIGHT; i++)
      canvas[i] = (glm::uint8*)calloc(WIDTH, sizeof(glm::uint8));

    float** z_buffer = (float**)calloc(HEIGHT, sizeof(float*));
    for (int i = 0; i < HEIGHT; i++)
    {
      z_buffer[i] = (float*)calloc(WIDTH, sizeof(float));
      std::fill(z_buffer[i], z_buffer[i] + WIDTH, 1.0f);
    }


    std::cout << "\x1b[?25l";

    using clock = std::chrono::high_resolution_clock;
    const auto frame_duration = std::chrono::microseconds(16667);

    glm::uint64 c = 0;
    while (true)
    {
        auto frame_start = clock::now();

        model = glm::rotate(model, glm::radians(1.0f), glm::vec3(0.0f,0.0f,1.0f));
        for (int i = 0; i < models.size(); i++)
        {
          for (int j = 0; j < light_num; j++)
          {
            rasterizer::rasterize(nullptr, lights[j].shadow_map, 0, nullptr, models[i], model, lights[j].view, lights[j].proj, lights[j].sm_width, lights[j].sm_height);
          }
        }

        for (int i = 0; i < models.size(); i++)
        {
          rasterizer::rasterize(canvas, z_buffer, light_num, lights, models[i], model, view, proj, WIDTH, HEIGHT);
        }


        std::cout << "\x1b[H";
        painter::render(canvas, WIDTH, HEIGHT);
        c++;
        std::cout << "\x1b[H" << c;
        std::cout << std::flush;

        for (int j = 0; j < HEIGHT; j++)
        {
            memset(canvas[j], 0, WIDTH * sizeof(glm::uint8));
            std::fill(z_buffer[j], z_buffer[j] + WIDTH, 1.0f);
        }
        for (int i = 0; i < light_num; i++)
        {
          for (int j = 0; j < lights[i].sm_height; j++)
              std::fill(lights[i].shadow_map[j], lights[i].shadow_map[j] + lights[i].sm_width, 1.0f);
        }

        auto frame_end = clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start);
        if (elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }

        //std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    std::cout << "\x1b[?25h";

    for (int i = 0; i < HEIGHT; i++)
    {
      free(canvas[i]);
      free(z_buffer[i]);
    }
    free(canvas);
    free(z_buffer);

    return 0;
}
