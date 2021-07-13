#ifndef DVIS_RENDER_WINDOW
#define DVIS_RENDER_WINDOW

#include <dray/data_model/collection.hpp>
#include <dray/rendering/camera.hpp>
#include <dray/aabb.hpp>
#include <dray/transform_3d.hpp>

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
  float m_move_factor;
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
    m_move_factor = sqrt(m_bounds.m_ranges[0].length() * m_bounds.m_ranges[0].length() +
                    m_bounds.m_ranges[1].length() * m_bounds.m_ranges[1].length() +  
                    m_bounds.m_ranges[2].length() * m_bounds.m_ranges[2].length()) / 100.f;

    // TODO: just add method that returns a vector of string inside
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

  float move_factor() 
  {
    return m_move_factor;
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

  void render_controls()
  {
    // call another class that handles all the UI menu/controls
    ImGui::SetNextWindowSize(ImVec2(200, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Control Window", NULL, ImGuiWindowFlags_MenuBar);

    // TODO: File system
    /********** FILE SYSTEM ***********/
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Menu"))
        {
            ShowExampleMenuFile();
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // TODO: Camera Controls
    /********** CAMERA CONTROLS ***********/
    if (ImGui::CollapsingHeader("Camera")) 
    {
      // ***** Camera Position *****:
      dray::Vec<float, 3> cam_pos = m_camera.get_pos();
      static float vec3f[3] = { 0.0f, 0.0f, 0.0f };
      vec3f[0] = cam_pos[0];
      vec3f[1] = cam_pos[1];
      vec3f[2] = cam_pos[2];
      ImGui::DragFloat3("Camera Position", vec3f, 0.1f, -10.0f, 10.0f);
      cam_pos[0] = vec3f[0];
      cam_pos[1] = vec3f[1];
      cam_pos[2] = vec3f[2];
      m_camera.set_pos(cam_pos);

      // ***** Reset Camera Position *****:
      if (ImGui::Button("Reset Camera"))
      {
        m_camera.reset_to_bounds(m_render_service.bounds());
      }

      // ***** Camera Zoom *****:
      static float zoom = 1.0f;
      ImGui::SliderFloat("Zoom", &zoom, 0.01f, 10.0f);
      m_camera.set_zoom(zoom);

      // ***** Camera Fov *****:
      static float fov = 60.0f;
      fov = m_camera.get_fov();
      ImGui::SliderFloat("Fov", &fov, 0.01f, 180.0f);
      m_camera.set_fov(fov);
    }
    

    // TODO: color picker
    /********** COLORS ***********/
    if (ImGui::CollapsingHeader("Colors")) 
    {
      // ***** Color Picker *****:
      // Generate a default palette. The palette will persist and can be edited.
      static bool saved_palette_init = true;
      static ImVec4 saved_palette[32] = {};
      if (saved_palette_init)
      {
          for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
          {
              ImGui::ColorConvertHSVtoRGB(n / 31.0f, 0.8f, 0.8f,
                  saved_palette[n].x, saved_palette[n].y, saved_palette[n].z);
              saved_palette[n].w = 1.0f; // Alpha
          }
          saved_palette_init = false;
      }

      static ImVec4 color = ImVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f);
      static ImVec4 backup_color;
      bool open_popup = ImGui::ColorButton("MyColor##3b", color);
      ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
      open_popup |= ImGui::Button("Palette Picker");
      if (open_popup)
      {
          ImGui::OpenPopup("mypicker");
          backup_color = color;
      }
      if (ImGui::BeginPopup("mypicker"))
      {
          ImGui::Text("MY CUSTOM COLOR PICKER WITH AN AMAZING PALETTE!");
          ImGui::Separator();
          ImGui::ColorPicker4("##picker", (float*)&color);
          ImGui::SameLine();

          ImGui::BeginGroup(); // Lock X position
          ImGui::Text("Current");
          ImGui::ColorButton("##current", color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
          ImGui::Text("Previous");
          if (ImGui::ColorButton("##previous", backup_color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40)))
              color = backup_color;
          ImGui::Separator();
          ImGui::Text("Palette");
          for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
          {
              ImGui::PushID(n);
              if ((n % 8) != 0)
                  ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);

              ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
              if (ImGui::ColorButton("##palette", saved_palette[n], palette_button_flags, ImVec2(20, 20)))
                  color = ImVec4(saved_palette[n].x, saved_palette[n].y, saved_palette[n].z, color.w); // Preserve alpha!

              // Allow user to drop colors into each palette entry. Note that ColorButton() is already a
              // drag source by default, unless specifying the ImGuiColorEditFlags_NoDragDrop flag.
              if (ImGui::BeginDragDropTarget())
              {
                  if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
                      memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 3);
                  if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
                      memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 4);
                  ImGui::EndDragDropTarget();
              }

              ImGui::PopID();
          }
          ImGui::EndGroup();
          ImGui::EndPopup();
      }
    }

    // TODO: field list
    /********** FIELD LIST ***********/
    if (ImGui::CollapsingHeader("Field List")) 
    {

    }

    ImGui::End();
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

    ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
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
    ImGui::ImageButton((void*)(intptr_t)textureID,
                       ImVec2(m_width , m_height),
                       ImVec2(0,0),
                       ImVec2(1,1),
                       0);
    // this works but the mouse movements are screwed up
    //ImGui::GetWindowDrawList()->AddImage(
    //   (void *)(intptr_t)textureID,
    //   ImVec2(ImGui::GetCursorScreenPos()),
    //   ImVec2(canvas_pos.x + m_width,canvas_pos.y + m_height),
    //   ImVec2(0, 0),
    //   ImVec2(1, 1));


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
    if(ImGui::IsWindowFocused())
    {
      handle_keys();
    }

    //conduit::Node state;
    //serialize(state);

    //m_render_service.publish(state);
    update_texture();
    ImGui::End();

    render_controls();
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

  /********** HANDLE KEYS **********/
  void handle_keys()
  {
    float m_move_factor = m_render_service.move_factor();
    if (ImGui::IsKeyPressed(GLFW_KEY_W))
    {
      dray::Vec<float, 3> look_at, pos, dir;
      look_at = m_camera.get_look_at();
      pos = m_camera.get_pos();
      dir = look_at - pos;
      dir.normalize();
      pos = pos + dir * m_move_factor;
      look_at = pos + dir * m_move_factor;
      m_camera.set_pos(pos);
      m_camera.set_look_at(look_at);
    }
    if (ImGui::IsKeyPressed(GLFW_KEY_S) &&
        !ImGui::IsKeyPressed(GLFW_KEY_LEFT_CONTROL))
    {
      dray::Vec<float, 3> look_at, pos, dir;
      look_at = m_camera.get_look_at();
      pos = m_camera.get_pos();
      dir = look_at - pos;
      dir.normalize();
      pos = pos - dir * m_move_factor;
      m_camera.set_pos(pos);
    }
    if (ImGui::IsKeyPressed(GLFW_KEY_A))
    {
      dray::Vec<float, 3> look_at, pos, dir, up, right;

      up = m_camera.get_up();
      look_at = m_camera.get_look_at();
      pos = m_camera.get_pos();

      dir = look_at - pos;
      right = cross(dir, up);
      right.normalize();

      pos = pos - right * m_move_factor;
      look_at = pos + dir;  // Missing m_move_factor? ***

      m_camera.set_pos(pos);
      m_camera.set_look_at(look_at);
    }
    if (ImGui::IsKeyPressed(GLFW_KEY_D))
    {
      dray::Vec<float, 3> look_at, pos, dir, up, right;

      up = m_camera.get_up();
      look_at = m_camera.get_look_at();
      pos = m_camera.get_pos();

      dir = look_at - pos;
      right = cross(dir, up);
      right.normalize();

      pos = pos + right * m_move_factor;
      look_at = pos + dir; // Missing m_move_factor? ***

      m_camera.set_pos(pos);
      m_camera.set_look_at(look_at);
    }
    if (ImGui::IsKeyPressed(GLFW_KEY_E))
    {
      dray::Vec<float, 3> look_at, pos, up;

      look_at = m_camera.get_look_at();
      pos = m_camera.get_pos();
      up = m_camera.get_up();

      pos = pos + up * m_move_factor;
      look_at = look_at - up * m_move_factor;

      m_camera.set_pos(pos);
      m_camera.set_look_at(look_at);
    }
    if (ImGui::IsKeyPressed(GLFW_KEY_C))
    {
      dray::Vec<float, 3> look_at, pos, up;

      look_at = m_camera.get_look_at();
      pos = m_camera.get_pos();
      up = m_camera.get_up();

      pos = pos - up * m_move_factor;
      look_at = look_at + up * m_move_factor;

      m_camera.set_pos(pos);
      m_camera.set_look_at(look_at);
    }
    if (ImGui::IsKeyPressed(GLFW_KEY_Q))
    {
      dray::Vec<float, 3> look_at, dir, up, pos;

      up = m_camera.get_up();
      look_at = m_camera.get_look_at();
      pos = m_camera.get_pos();
      dir = look_at - pos;
      float rot_fact = 2.f;
      auto rot_mat = dray::rotate(rot_fact, dir);
      up = dray::transform_vector(rot_mat, up);
      m_camera.set_up(up);
    }
    if (ImGui::IsKeyPressed(GLFW_KEY_Z))
    {
      dray::Vec<float, 3> look_at, dir, up, pos;

      up = m_camera.get_up();
      look_at = m_camera.get_look_at();
      pos = m_camera.get_pos();
      dir = look_at - pos;
      float rot_fact = -2.f;
      auto rot_mat = dray::rotate(rot_fact, dir);
      up = dray::transform_vector(rot_mat, up);
      m_camera.set_up(up);
    }

    if (ImGui::IsKeyPressed(GLFW_KEY_S) &&
        ImGui::IsKeyPressed(GLFW_KEY_LEFT_CONTROL))
    {
      save_image();
    }
  }

  void save_image()
  {
    //TODO: Save function for RenderService
    //m_render_service.save("snapshot.pnm");
  }

  /********** MENU BAR **********/
  static void ShowExampleMenuFile()
  {
    ImGui::MenuItem("(demo menu)", NULL, false, false);
    if (ImGui::MenuItem("New")) {}
    if (ImGui::MenuItem("Open", "Ctrl+O")) {}
    if (ImGui::BeginMenu("Open Recent"))
    {
      ImGui::MenuItem("fish_hat.c");
      ImGui::MenuItem("fish_hat.inl");
      ImGui::MenuItem("fish_hat.h");
      if (ImGui::BeginMenu("More.."))
      {
        ImGui::MenuItem("Hello");
        ImGui::MenuItem("Sailor");
        if (ImGui::BeginMenu("Recurse.."))
        {
          ShowExampleMenuFile();
          ImGui::EndMenu();
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Save", "Ctrl+S")) {}
    if (ImGui::MenuItem("Save As..")) {}

    ImGui::Separator();
    if (ImGui::BeginMenu("Options"))
    {
      static bool enabled = true;
      ImGui::MenuItem("Enabled", "", &enabled);
      ImGui::BeginChild("child", ImVec2(0, 60), true);
      for (int i = 0; i < 10; i++) ImGui::Text("Scrolling Text %d", i);
      ImGui::EndChild();
      static float f = 0.5f;
      static int n = 0;
      ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
      ImGui::InputFloat("Input", &f, 0.1f);
      ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Colors"))
    {
      float sz = ImGui::GetTextLineHeight();
      for (int i = 0; i < ImGuiCol_COUNT; i++)
      {
        const char* name = ImGui::GetStyleColorName((ImGuiCol)i);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), ImGui::GetColorU32((ImGuiCol)i));
        ImGui::Dummy(ImVec2(sz, sz));
        ImGui::SameLine();
        ImGui::MenuItem(name);
      }
      ImGui::EndMenu();
    }

    // Here we demonstrate appending again to the "Options" menu (which we already created above)
    // Of course in this demo it is a little bit silly that this function calls BeginMenu("Options") twice.
    // In a real code-base using it would make senses to use this feature from very different code locations.
    if (ImGui::BeginMenu("Options")) // <-- Append!
    {
      static bool b = true;
      ImGui::Checkbox("SomeOption", &b);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Disabled", false)) // Disabled
    {
      IM_ASSERT(0);
    }
    if (ImGui::MenuItem("Checked", NULL, true)) {}
    if (ImGui::MenuItem("Quit", "Alt+F4")) {}
  }
};
#endif
