#pragma once
#include <iostream>
#include <cstring>
#include "linal.cpp"
#include "painter.cpp"

namespace rasterizer {
  void draw_triangle(const linal::Triangle& t, const size_t& light_num, linal::Light* lights, const glm::vec3 t_world_poss[], glm::vec3& view_pos, glm::uint8** canvas, float** z_buffer, const int& width, const int& height);

  void rasterize(glm::uint8** canvas,
                 float** z_buffer,
                 const size_t& number_of_lights,
                 linal::Light* lights,
                 const std::vector<linal::Triangle>& tris,
                 const glm::mat4& model,
                 const glm::mat4& view,
                 const glm::mat4& proj,
                 const int& width,
                 const int& height)
  {
    glm::mat4 mvp = proj * view * model;
    glm::vec3 view_pos;
    if (canvas != nullptr) view_pos = glm::vec3(glm::inverse(view)[3]);

    for (int i = 0; i < tris.size(); i++)
    {
      glm::vec3 tri_world_poss[3] = {
        glm::vec3(model * glm::vec4(tris[i].v1.pos, 1.0f)),
        glm::vec3(model * glm::vec4(tris[i].v2.pos, 1.0f)),
        glm::vec3(model * glm::vec4(tris[i].v3.pos, 1.0f))
      };
      glm::vec4 c1 = mvp * glm::vec4(tris[i].v1.pos, 1.0f);
      glm::vec4 c2 = mvp * glm::vec4(tris[i].v2.pos, 1.0f);
      glm::vec4 c3 = mvp * glm::vec4(tris[i].v3.pos, 1.0f);

      linal::Triangle tri_screen;
      tri_screen.v1.norm = glm::mat3(model) * tris[i].v1.norm;
      tri_screen.v2.norm = glm::mat3(model) * tris[i].v2.norm;
      tri_screen.v3.norm = glm::mat3(model) * tris[i].v3.norm;

      tri_screen.v1.pos = glm::vec3(c1) / c1.w;
      tri_screen.v1.pos += glm::vec3(1.0f,1.0f,0.0f);
      tri_screen.v1.pos *= glm::vec3(static_cast<float>(width)/2.0f,static_cast<float>(height)/2.0f,1.0f);
      tri_screen.v2.pos = glm::vec3(c2) / c2.w;
      tri_screen.v2.pos += glm::vec3(1.0f,1.0f,0.0f);
      tri_screen.v2.pos *= glm::vec3(static_cast<float>(width)/2.0f,static_cast<float>(height)/2.0f,1.0f);
      tri_screen.v3.pos = glm::vec3(c3) / c3.w;
      tri_screen.v3.pos += glm::vec3(1.0f,1.0f,0.0f);
      tri_screen.v3.pos *= glm::vec3(static_cast<float>(width)/2.0f,static_cast<float>(height)/2.0f,1.0f);

      if ((tri_screen.v1.pos.z > 1.0f && tri_screen.v2.pos.z > 1.0f && tri_screen.v3.pos.z > 1.0f) ||
          (tri_screen.v1.pos.z < -1.0f && tri_screen.v2.pos.z < -1.0f && tri_screen.v3.pos.z < -1.0f))
        continue;

      // TODO: split triangles that go behind near plane

      draw_triangle(tri_screen, number_of_lights, lights, tri_world_poss, view_pos, canvas, z_buffer, width, height);
    }

  }

  float edge_function(const glm::vec3& v1, const glm::vec3& v2, const glm::vec2& c)
  {
      return (c.x - v1.x)*(v2.y - v1.y) - (c.y - v1.y)*(v2.x - v1.x);
  }

  void frag_shader(glm::uint8& frag, const float& z_depth, const size_t& light_num, linal::Light* lights, const glm::vec3& view_pos, const glm::vec3& world_pos, const glm::vec3& norm);

  void draw_triangle(const linal::Triangle& t, const size_t& light_num, linal::Light* lights, const glm::vec3 t_world_poss[], glm::vec3& view_pos, glm::uint8** canvas, float** z_buffer, const int& width, const int& height)
  {
    int min_x = static_cast<int>(std::floor(glm::min(t.v1.pos.x, glm::min(t.v2.pos.x, t.v3.pos.x))));
    min_x = glm::clamp(min_x, 0, width-1);
    int max_x = static_cast<int>(std::ceil(glm::max(t.v1.pos.x, glm::max(t.v2.pos.x, t.v3.pos.x))));
    max_x = glm::clamp(max_x, 0, width-1);
    int min_y = static_cast<int>(std::floor(glm::min(t.v1.pos.y, glm::min(t.v2.pos.y, t.v3.pos.y))));
    min_y = glm::clamp(min_y, 0, height-1);
    int max_y = static_cast<int>(std::ceil(glm::max(t.v1.pos.y, glm::max(t.v2.pos.y, t.v3.pos.y))));
    max_y = glm::clamp(max_y, 0, height-1);

    float area = edge_function(t.v1.pos, t.v2.pos, glm::vec2(t.v3.pos));
    if (std::abs(area) < 0.00001f) return;

    for (int j = min_y; j <= max_y; j++)
    {
      for (int i = min_x; i <= max_x; i++)
      {

        glm::vec2 check = glm::vec2(static_cast<float>(i) + 0.5f, static_cast<float>(j) + 0.5f);
        float E12 = edge_function(t.v1.pos, t.v2.pos, check);
        float E23 = edge_function(t.v2.pos, t.v3.pos, check);
        float E31 = edge_function(t.v3.pos, t.v1.pos, check);

        bool isInside = (E12 <= 0 && E23 <= 0 && E31 <= 0);

        if (isInside)
        {
          float w1 = E23 / area;
          float w2 = E31 / area;
          float w3 = E12 / area;
          float z_depth = w1 * t.v1.pos.z + w2 * t.v2.pos.z + w3 * t.v3.pos.z;
          if (z_depth < -1.0f || z_depth > 1.0f) continue;

          if (z_depth > z_buffer[j][i]) continue;

          z_buffer[j][i] = z_depth;

          glm::vec3 interp_world_pos = w1 * t_world_poss[0] + w2 * t_world_poss[1] + w3 * t_world_poss[2];
            
          glm::vec3 interp_norm = w1 * t.v1.norm + w2 * t.v2.norm + w3 * t.v3.norm;
          interp_norm = glm::normalize(interp_norm);

          if (canvas == nullptr) continue;

          frag_shader(canvas[j][i], z_depth, light_num, lights, view_pos, interp_world_pos, interp_norm);
        }
      }
    }
  }

  bool in_shadow(const glm::vec3& world_pos, const linal::Light& light)
  {
    glm::mat4 light_vp = light.proj * light.view;
    glm::vec4 light_clip = light_vp * glm::vec4(world_pos, 1.0f);

    if (light_clip.w <= 0.0f) return false;

    glm::vec3 light_ndc = glm::vec3(light_clip) / light_clip.w;

    if (light_ndc.x < -1.0f || light_ndc.x > 1.0f ||
        light_ndc.y < -1.0f || light_ndc.y > 1.0f ||
        light_ndc.z < -1.0f || light_ndc.z > 1.0f)
      return false;

    glm::vec2 uv = (glm::vec2(light_ndc) + 1.0f) * 0.5f;

    int sx = glm::clamp(static_cast<int>(uv.x * light.sm_width), 0, light.sm_width - 1);
    int sy = glm::clamp(static_cast<int>(uv.y * light.sm_height), 0, light.sm_height - 1);

    const float bias = 0.005f;
    return light_ndc.z - bias > light.shadow_map[sy][sx];
  }

  void frag_shader(glm::uint8& frag, const float& z_depth, const size_t& light_num, linal::Light* lights, const glm::vec3& view_pos, const glm::vec3& world_pos, const glm::vec3& norm)
  {
    float total = 0.0f;
    for (size_t i = 0; i < light_num; i++)
    {
      if (in_shadow(world_pos, lights[i])) continue;

      glm::vec3 light_dir = glm::normalize(lights[i].pos - world_pos);
      glm::vec3 view_dir = glm::normalize(view_pos - world_pos);
      glm::vec3 half_dir = glm::normalize(light_dir + view_dir);

      glm::vec3 to_light = lights[i].pos - world_pos;
      float dist2 = glm::dot(to_light, to_light);
      if (dist2 == 0.0f) continue;

      float attenuation = 1.0f / dist2;

      float diffuse = glm::max(glm::dot(norm, light_dir), 0.0f) * lights[i].intensity * attenuation;

      total += diffuse * 255.0f;
    }
    frag = static_cast<glm::uint8>(glm::clamp(total, 0.0f, 255.0f));
  }
}
