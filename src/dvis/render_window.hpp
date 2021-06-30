#ifndef DVIS_RENDER_WINDOW
#define DVIS_RENDER_WINDOW

#include <dray/data_model/collection.hpp>
#include <dray/rendering/camera.hpp>
#include <dray/aabb.hpp>

#include <vector>

// render service headers
#include <dray/io/blueprint_reader.hpp>
#include <dray/filters/mesh_boundary.hpp>
#include <dray/rendering/surface.hpp>
#include <dray/rendering/renderer.hpp>
#include <set>

class RenderService
{
protected:
  dray::Collection m_dataset;
  dray::AABB<3> m_bounds;
  std::vector<std::string> m_fields;
  dray::Renderer m_renderer;
  std::shared_ptr<dray::Surface> m_surface;

public:
  RenderService()
  {
    const std::string dataset_path =
      "/usr/workspace/showcase/devil_ray_cuda/src/tests/dray/data/warbly_cube_000000.root";
    load_root_file(dataset_path);
  }

  void load_root_file(const std::string root_file)
  {
    dray::Collection dataset = dray::BlueprintReader::load (root_file);
    dray::MeshBoundary boundary;
    m_dataset = boundary.execute(dataset);
    m_bounds = m_dataset.bounds();

    // TODO: just add method that returns a vector of setring inside
    // dray
    int num_domains = m_dataset.local_size();
    std::set<std::string> fset;
    for(int i = 0; i < num_domains; ++i)
    {
      dray::DataSet dom = m_dataset.domain(i);
      const std::vector<std::string> dom_fields = dom.fields();
      for(auto &f : dom_fields)
      {
        fset.insert(f);
      }
    }

    // now just put them into a vector
    m_fields.clear();
    for(auto &f : fset)
    {
      m_fields.push_back(f);
    }

    if(m_fields.size() == 0)
    {
      // this is bad
      std::cout<<"There are no fields!!!!!\n";
    }
    m_renderer.clear();
    m_surface = std::make_shared<dray::Surface>(m_dataset);
    m_surface->field(m_fields[0]);
    //surface->color_map().color_table(color_table);
    //m_surface->draw_mesh (true);
    //m_surface->line_color(line_color);
    m_renderer.add(m_surface);
  }

  dray::AABB<3> bounds() const
  {
    return m_bounds;
  }

  dray::Framebuffer render(dray::Camera &camera)
  {
    return m_renderer.render(camera);
  }

};

class RenderWindow
{
protected:

  //vtkm::cont::DataSet              m_dataset;

  int m_width;
  int m_height;
  GLuint textureID;

  dray::Camera                       m_camera;
  std::string                        m_field_name;
  std::string                        m_file_name;
  RenderService                      m_render_service;
  dray::Framebuffer                  m_framebuffer;

public:
  RenderWindow()
    : m_width(512),
      m_height(512)
  {
    //m_canvas.SetBackgroundColor(vtkm::rendering::Color(1.f, 1.f, 1.f));
    //vtkm::Vec<vtkm::Float32, 3> vec3(1.f,1.f, 1.f);
    //m_tracer.SetBackgroundColor(vec3);
    //m_bg_color[0] = 1.f;
    //m_bg_color[1] = 1.f;
    //m_bg_color[2] = 1.f;
    //m_bg_color[3] = 1.f;
    glGenTextures(1, &textureID);
    update_render_dims();
    m_camera.reset_to_bounds(m_render_service.bounds());
  }


  // this is a dummy framebuffer
  void update_render_dims()
  {
    m_camera.set_width(m_width);
    m_camera.set_height(m_height);
  }

  void update_texture()
  {
    m_framebuffer = m_render_service.render(m_camera);
    m_framebuffer.composite_background();
    // update frame_buffer
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 m_width,
                 m_height,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 m_framebuffer.colors().get_host_ptr());
                 //m_render_service.get_color_buffer());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  void render()
  {
    // custom window
    std::string wname = "Render Window";

    ImGui::SetNextWindowSize(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::Begin(wname.c_str(), NULL);
    ImVec2 content_size = ImGui::GetContentRegionAvail();
    if(content_size.x != m_width || content_size.y != m_height)
    {
      m_width  = content_size.x;
      m_height = content_size.y;
      update_render_dims();
    }

    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();

    // For some reason, I made the entire window a giant button
    // since I can associate the texture id with the button
    //ImGui::ImageButton((void*)(intptr_t)textureID,
    //                   ImVec2(m_width , m_height),
    //                   ImVec2(0,0),
    //                   ImVec2(1,1),
    //                   0);
    // this works but the mouse movements are screwed up
    ImGui::GetWindowDrawList()->AddImage(
       (void *)(intptr_t)textureID,
       ImVec2(ImGui::GetCursorScreenPos()),
       ImVec2(canvas_pos.x + m_width,canvas_pos.y + m_height),
       ImVec2(0, 0),
       ImVec2(1, 1));


    // Debug pixel colors
    //if (ImGui::IsItemHovered())
    //{
    //  ImGui::BeginTooltip();
    //  ImVec2 mouse_pos = ImGui::GetMousePos();
    //  ImVec2 image_pos;
    //  image_pos.x = mouse_pos[0] - canvas_pos[0];
    //  image_pos.y = mouse_pos[1] - canvas_pos[1];
    //  float color[4];
    //  m_render_service.get_color(image_pos.x, image_pos.y, color);
    //  std::stringstream ss;
    //  ss<<"         ["<<image_pos[0]<<", "<<image_pos[1]<<"] ";
    //  ss<<"c:["<<(int)(color[0]*255)<<", "<<(int)(color[1]*255)<<", "<<(int)(color[2]*255)<<"]";
    //  ImVec2 cpos = ImGui::GetCursorScreenPos();
    //  ImGui::Text("%s", ss.str().c_str());
    //  ImDrawList* draw_list = ImGui::GetWindowDrawList();

    //  ImU32 pixel_color = ImColor(color[0],color[1],color[2],color[3]);
    //  draw_list->AddCircleFilled(cpos, 20, pixel_color);
    //  ImGui::EndTooltip();

    //}

    handle_mouse(canvas_pos);

    //if(ImGui::IsWindowFocused())
    //{
    //  handle_keys();
    //}

    //ImGui::End();

    //conduit::Node state;
    //serialize(state);

    //m_render_service.publish(state);
    update_texture();
    ImGui::End();
  }

  void handle_mouse(const ImVec2 &canvas_pos)
  {
    static int x_last = -1;
    static int y_last = -1;

    // make sure we only respond to trackball clicks on the canvas
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 clicked = io.MouseClickedPos[0];
    ImVec2 ipos;
    ipos.x = clicked[0] - canvas_pos[0];
    ipos.y = clicked[1] - canvas_pos[1];
    bool inside_canvas = ipos.x >= 0 && ipos.x < m_width && ipos.y >= 0 && ipos.y < m_height;

    if(ImGui::IsMouseDragging(0) && ImGui::IsWindowFocused() && inside_canvas)
    {
      // render window active so respond to mouse events
      ImVec2 mouse_pos = ImGui::GetMousePos();
      if(x_last != -1 && y_last != -1)
      {
        int x_diff = mouse_pos.x - x_last;
        int y_diff = mouse_pos.y - y_last;
        if(x_diff != 0 && y_diff != 0)
        {
          float x1 = float(x_last * 2) / float(m_width) - 1.0f;
          float y1 = float(y_last * 2) / float(m_height) - 1.0f;
          float x2 = float(mouse_pos.x * 2) / float(m_width) - 1.0f;
          float y2 = float(mouse_pos.y * 2) / float(m_height) - 1.0f;
          m_camera.trackball_rotate(x1, y1, x2, y2);
        }
      }
      x_last = mouse_pos.x;
      y_last = mouse_pos.y;

    }

    if(ImGui::IsMouseReleased(0))
    {
      x_last = -1;
      y_last = -1;
    }
  }

  void kill()
  {
    //conduit::Node state;
    //serialize(state);
    //state["kill"] = "true";
    //m_render_service.publish(state);
  }


};
#endif
