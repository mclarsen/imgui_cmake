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

#include "imGuIZMOquat.h"
#include "vgMath.h"

#include "imguifilesystem.h"

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

  // For imGuIZMO, declare static or global variable or member class quaternion
  quat qRot = quat(1.f, 0.f, 0.f, 0.f);
  mat4 modelMatrix = mat4_cast(qRot);
  dray::Camera initial_camera;
  // two helper functions, not really necessary (but comfortable)
  void setRotation(const quat &q) { qRot = q; }
  quat& getRotation() { return qRot; }
  // Transfer Function Alphas
  bool tf_mouse_down = false;
  float tf_alphas[512] = {};  // The values of tf_array range from 0 to the height of the transfer function rectangle (255).
  ImVec2 mouse_drag = ImVec2(0.0f, 0.0f);

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
    initial_camera = m_camera;
  }

  void render_controls()
  {
    // call another class that handles all the UI menu/controls
    ImGui::SetNextWindowSize(ImVec2(200, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Control Window", NULL, ImGuiWindowFlags_MenuBar);
    ImGuiIO& io = ImGui::GetIO();

    bool file_explorer = false;
    bool session_save = false;
    bool session_load = false;
    /********** MENU BAR ***********/
    if (ImGui::BeginMenuBar())
    {
      //***** File System *****/
      if (ImGui::BeginMenu("File"))
      {
        ImGui::MenuItem("(demo menu)", NULL, false, false);
        if (ImGui::MenuItem("Open", "Ctrl+O"))
        {
          file_explorer = true;
        }
        // TODO: Open Recent capabilities
        // if (ImGui::BeginMenu("Open Recent"))
        // {
        //   ImGui::MenuItem("fish_hat.c");
        //   ImGui::MenuItem("fish_hat.inl");
        //   ImGui::MenuItem("fish_hat.h");
        //   if (ImGui::BeginMenu("More.."))
        //   {
        //     ImGui::MenuItem("Hello");
        //     ImGui::MenuItem("Sailor");
        //     if (ImGui::BeginMenu("Recurse.."))
        //     {
        //       ShowExampleMenuFile();
        //       ImGui::EndMenu();
        //     }
        //     ImGui::EndMenu();
        //   }
        //   ImGui::EndMenu();
        // }
        if (ImGui::MenuItem("Save", "Ctrl+S")) 
        {
          // session_save = true;
        }
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
  
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();

      // TODO: Save As capabilities
      // if (ImGui::MenuItem("Save As..")) 
      // {
      //
      // }
      if (ImGui::MenuItem("Load Session", ""))
      {
        // session_load = true;
      } 

      static ImGuiFs::Dialog open_dlg;
      std::string file_path = open_dlg.chooseFileDialog(file_explorer);
      if(file_path != "")
      {
        //DEBUG
        std::cout << "File path is: " << file_path << "\n";
        // load_dataset(file_path);
        m_render_service.load_root_file(file_path);
      }

      // static ImGuiFs::Dialog save_dlg;
      // std::string session_file = save_dlg.saveFileDialog(session_save);
      // if(session_file != "")
      // {
      //   save_session(session_file);
      // }

      // static ImGuiFs::Dialog load_dlg;
      // session_file = load_dlg.chooseFileDialog(session_load);
      // if(session_file != "")
      // {
      //   load_session(session_file);
        // }
    }

    /********** CAMERA CONTROLS ***********/
    if (ImGui::CollapsingHeader("Camera"))
    {
      // ***** Reset Camera Position *****:
      if (ImGui::Button("Reset Camera"))
      {
        m_camera.reset_to_bounds(m_render_service.bounds());
      }

      // TODO: Do any of these need to be normalized? Normalize if necessary.
      // ***** Camera Position *****:
      dray::Vec<float, 3> cam_pos = m_camera.get_pos();
      static float vec3f_cam_pos[3] = { 0.0f, 0.0f, 0.0f };
      vec3f_cam_pos[0] = cam_pos[0];
      vec3f_cam_pos[1] = cam_pos[1];
      vec3f_cam_pos[2] = cam_pos[2];
      ImGui::DragFloat3("Camera Position", vec3f_cam_pos, 0.1f, -10.0f, 10.0f);
      cam_pos[0] = vec3f_cam_pos[0];
      cam_pos[1] = vec3f_cam_pos[1];
      cam_pos[2] = vec3f_cam_pos[2];
      m_camera.set_pos(cam_pos);

      // ***** Camera Look At *****:
      dray::Vec<float, 3> cam_look_at = m_camera.get_look_at();
      static float vec3f_cam_look_at[3] = { 0.0f, 0.0f, 0.0f };
      vec3f_cam_look_at[0] = cam_look_at[0];
      vec3f_cam_look_at[1] = cam_look_at[1];
      vec3f_cam_look_at[2] = cam_look_at[2];
      ImGui::DragFloat3("Camera Look At", vec3f_cam_look_at, 0.1f, -10.0f, 10.0f);
      cam_look_at[0] = vec3f_cam_look_at[0];
      cam_look_at[1] = vec3f_cam_look_at[1];
      cam_look_at[2] = vec3f_cam_look_at[2];
      m_camera.set_look_at(cam_look_at);

      // ***** Camera Up *****:
      dray::Vec<float, 3> cam_up = m_camera.get_up();
      static float vec3f_cam_up[3] = { 0.0f, 0.0f, 0.0f };
      vec3f_cam_up[0] = cam_up[0];
      vec3f_cam_up[1] = cam_up[1];
      vec3f_cam_up[2] = cam_up[2];
      ImGui::DragFloat3("Camera Up", vec3f_cam_up, 0.1f, -10.0f, 10.0f);
      cam_up[0] = vec3f_cam_up[0];
      cam_up[1] = vec3f_cam_up[1];
      cam_up[2] = vec3f_cam_up[2];
      m_camera.set_up(cam_up);

      // ***** Camera Zoom *****:
      static float zoom = 1.0f;
      ImGui::SliderFloat("Zoom", &zoom, 0.01f, 10.0f);
      m_camera.set_zoom(zoom);

      // ***** Camera Fov *****:
      static float fov = 60.0f;
      fov = m_camera.get_fov();
      ImGui::SliderFloat("Fov", &fov, 0.01f, 180.0f);
      m_camera.set_fov(fov);

      // ***** imGuIZMO *****:
      quat qt = getRotation();
      // TODO: Rotate guizmo to match the camera.
      if(ImGui::gizmo3D("##gizmo1", qt, 240 /*,  mode */)) 
      {  
        setRotation(qt); 

        // If the modelMatrix has changed, then a rotation has occured.
        bool r = false;
        for (int i = 0; i < 4; ++i)
        {
          if (r)
          {
            break;
          }
          for (int j = 0; j < 4; ++j)
          {
            if (modelMatrix[i][j] != mat4_cast(qt)[i][j]) 
            {
              r = true;
              break;
            }
          }
        }
        if (r)
        {
          // Now you have a vgMath mat4 modelMatrix with rotation then can build MV and MVP matrix. To translate mat4 into dray::Matrix<float, 4, 4>: 
          //  The dray::Matrix is made of 4 dray::Vec<T,4> vectors. The mat4 matrix is made of 4 vgMath Vec4 vectors.
          //  Assign the row-column data for the rotation matrix based on the corresponding row-column of the modelMatrix.

          mat4 delta_rotation = transpose(modelMatrix) * mat4_cast(qt);
          dray::Matrix<float, 4, 4> rotate;
          for (int i = 0; i < 4; ++i)
          {
            for (int j = 0; j < 4; ++j)
            {
              rotate[i][j] = delta_rotation[i][j];     
            }
          }

          dray::Matrix<float, 4, 4> trans = translate (-m_camera.get_look_at());
          dray::Matrix<float, 4, 4> inv_trans = translate (m_camera.get_look_at());

          dray::Matrix<float, 4, 4> view = m_camera.view_matrix ();
          view (0, 3) = 0;
          view (1, 3) = 0;
          view (2, 3) = 0;

          dray::Matrix<float, 4, 4> inverseView = view.transpose ();

          dray::Matrix<float, 4, 4> full_transform = inv_trans * inverseView * rotate * view * trans;

          m_camera.set_pos(transform_point (full_transform, m_camera.get_pos()));
          m_camera.set_look_at(transform_point (full_transform, m_camera.get_look_at()));
          m_camera.set_up(transform_vector (full_transform, m_camera.get_up()));

          modelMatrix = mat4_cast(qt);
        }
      } else {
        // Rotate ImGuizmo. TODO: Fix the rotate_guizmo method. I suspect the problem lies in the matrix multiplication math.
        //rotate_guizmo();
      }
      // ***** DEBUG Guizmo *****:
      if (ImGui::Button("Test Quat"))
      {
        //DEBUG
        // Determine if mat4 to quat then quat to mat4 are equivalent.
        mat4 mat4_test_before = mat4_cast(qt);
        quat q_test = matrix_to_quat(mat4_test_before);
        mat4 mat4_test_after = mat4_cast(q_test);
        std::cout << "\nIs mat4_test_before equivalent to mat4_test_after?\n";
        for (int i = 0; i < 4; ++i)
        {
          std::cout << "Row: " << i << "\n";
          for (int j = 0; j < 4; ++j)
          {
            std::cout << "mat4_test_before[" << i << "][" << j << "]: " << mat4_test_before[i][j] 
              << ", vs new_gizmo_orientation[[" << i << "][" << j << "]: " << mat4_test_after[i][j] << "\n";
          }
        }
      }
    }

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

    // TODO: Transfer Function Window
    /********** TRANSFER FUNCTIONS ***********/
    if (ImGui::CollapsingHeader("Transfer Functions"))
    {
      if (ImGui::Button("Transfer Function Test Button"))
      {
        std::cout << "Button tested!\n";
      } 

      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      
      // Use Get and Set CursorScreenPos to control where the "draw cursor" is.
      ImVec2 pos = ImGui::GetCursorScreenPos();
      ImVec2 marker_min;  // Top left corner of a rectangle.
      ImVec2 marker_max;  // Bottome right corner of a rectangle.
      int tf_rect_width = 512;
      int tf_rect_height = 255;

      // Free form transfer function
      ImGui::BeginGroup();
      {
        // TODO: Radial / check box for fixing width instead of relative scaling. Currently, relative scaling is commented out below.
        //int tf_rect_width = (ImGui::GetWindowWidth() > 100) ? ImGui::GetWindowWidth() : 100; 
        tf_rect_width = 512;
        dray::ColorTable my_bar("temperature");
        dray::Array<dray::Vec<float, 4>> color_map;
        my_bar.sample(tf_rect_width, color_map);
        dray::Vec<float,4> *color_ptr = color_map.get_host_ptr();

        // Draw color_map gradient rectangle.
        // For every item in the color_map...
        pos = ImGui::GetCursorScreenPos();
        int cm_length = color_map.size();
        for (int i = 0; i < cm_length; i++)
        {
          // Draw a rectangle 1 pixel wide of a color based on the ith index of color_map.
          marker_min = ImVec2(pos.x + i, pos.y);
          marker_max = ImVec2(pos.x + i + 1, pos.y + tf_rect_height);
          draw_list->AddRectFilled(marker_min, marker_max, IM_COL32(color_map.get_value(i)[0] * 255, color_map.get_value(i)[1] * 255,
                                                                    color_map.get_value(i)[2] * 255, color_map.get_value(i)[3] * 255));
        }

        // Draw the value array histogram. 
        const float values[5] = { 0.5f, 0.20f, 0.80f, 0.60f, 0.25f };
        float max_value = *max_element(begin(values), end(values));
        int bins = sizeof(values) / sizeof(values[0]);
        float bin_width = tf_rect_width / bins;
        float bin_height = tf_rect_height;
        float bin_offset = 0;
        // For every bin in the array of values...
        for (int i = 0; i < bins; i++)
        {
          bin_height = tf_rect_height * (values[i] / max_value);
          bin_offset = tf_rect_height * (1 - values[i] / max_value);
          marker_min = ImVec2(pos.x + i * bin_width, pos.y + bin_offset);
          marker_max = ImVec2(pos.x + (i + 1) * bin_width, pos.y + bin_height + bin_offset);
          draw_list->AddRectFilled(marker_min, marker_max, IM_COL32(50, 50, 50, 150));
        }

        // Draw the free form Transfer Function, which determines the values in the tf_alphas array.
        // Determine if the mouse is clicked within the transfer function rectangle.
        ImVec2 tf_mouse_pos = io.MousePos;
        ImVec2 delta_mouse_drag = ImVec2(0.0f, 0.0f);
        if (tf_mouse_pos.x >= pos.x && tf_mouse_pos.x <= pos.x + tf_rect_width && 
            tf_mouse_pos.y >= pos.y && tf_mouse_pos.y <= pos.y + tf_rect_height && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
          tf_mouse_down = true;
        } 

        // If the mouse was clicked within tf_rect and is held down, adjust the delta drag.
        if (tf_mouse_down && ImGui::IsMouseDown(ImGuiMouseButton_Left)) 
        {
          delta_mouse_drag = ImGui::GetMouseDragDelta(0, 0.0f) - mouse_drag;  // This is the change in the mouse drag since last frame.
          mouse_drag = ImGui::GetMouseDragDelta(0, 0.0f);  

          // Prevent the window from being dragged.
          io.ConfigWindowsMoveFromTitleBarOnly = true;

          // Set tf_alphas values based on the clicked mouse position, including any distance dragged through this frame.
          int alpha_index = tf_mouse_pos.x - pos.x;  // This is the location of the mouse pointer this frame.
          int i = alpha_index - delta_mouse_drag.x;
          int n = alpha_index;
          // i and n are reversed if dragging is in the leftward (negative) direction.
          if (delta_mouse_drag.x < 0) 
          {
            i = alpha_index;
            n = alpha_index - delta_mouse_drag.x;
          }

          // From where the mouse began to where it was dragged through on the x axis, adjust each cooresponding index of the tf_alphas array.
          for (i; i <= n; i++)
          {
            int tf_alpha_lerp = (tf_rect_height - io.MousePos.y + pos.y -
                                ((i - alpha_index + delta_mouse_drag.x) / delta_mouse_drag.x) * (delta_mouse_drag.y)); 

            // If there is no mouse drag, no linear interpolation needed.
            if (delta_mouse_drag.x == 0)
            {
              tf_alpha_lerp = tf_rect_height - io.MousePos.y + pos.y;
            }

            // If the mouse rose above the tf rectangle, clamp tf_alpha_lerp to the max value AKA 255
            if (tf_alpha_lerp > tf_rect_height)
            {
              tf_alpha_lerp = tf_rect_height;
            } 
            // Otherwise, if the mouse dropped below the tf rectangle, clamp tf_alpha_lerp to the min value, 0. 
            else if (tf_alpha_lerp < 0)
            {
              tf_alpha_lerp = 0;
            }

            // If the mouse was contained between the tf rectangle left and right bounds.
            if (i >= 0 && i < tf_rect_width)
            {
              tf_alphas[i] = tf_alpha_lerp;
            }
            // Otherwise, if the current index would fall outside to the left of the tf rectangle
            else if (i < 0)
            {
              tf_alphas[0] = tf_alpha_lerp;
            }
            // Otherwise, if the current index would fall outside to the right of the tf rectangle
            else if (i >= tf_rect_width)
            {
              tf_alphas[tf_rect_width - 1] = tf_alpha_lerp;
            }
          }
        } 

        // Draw the alpha rectangle pixel column by pixel column
        for (int i = 0; i < cm_length; i++)
        {
          marker_min = ImVec2(pos.x + i, pos.y + tf_rect_height - tf_alphas[i]);
          marker_max = ImVec2(pos.x + i + 1, pos.y + tf_rect_height);
          draw_list->AddRectFilled(marker_min, marker_max, IM_COL32(200, 200, 200, 200));
        }

        // Check if the user released the Mouse Drag.
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) 
        {
          io.ConfigWindowsMoveFromTitleBarOnly = false;
          tf_mouse_down = false;
          mouse_drag = ImVec2(0.0f, 0.0f);
        }

        // Update CursorScreenPos to just after the TransferFunction rectangle.
        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + tf_rect_height));
      }
      ImGui::EndGroup();

      // TODO: Second TF Rectangle which can be manipulated using dots and linear interpolation between them.
      ImGui::BeginGroup();
      {
        // TODO: Radial / check box for fixing width instead of relative scaling. Currently, relative scaling is commented out below.
        //int tf_rect_width = (ImGui::GetWindowWidth() > 100) ? ImGui::GetWindowWidth() : 100; 
        tf_rect_width = 512;
        dray::ColorTable my_bar("rainbow");
        dray::Array<dray::Vec<float, 4>> color_map;
        my_bar.sample(tf_rect_width, color_map);
        dray::Vec<float,4> *color_ptr = color_map.get_host_ptr();

        // Draw color_map gradient rectangle.
        // For every item in the color_map...
        pos = ImGui::GetCursorScreenPos();
        int cm_length = color_map.size();
        for (int i = 0; i < cm_length; i++)
        {
          // Draw a rectangle 1 pixel wide of a color based on the ith index of color_map.
          marker_min = ImVec2(pos.x + i, pos.y);
          marker_max = ImVec2(pos.x + i + 1, pos.y + tf_rect_height);
          draw_list->AddRectFilled(marker_min, marker_max, IM_COL32(color_map.get_value(i)[0] * 255, color_map.get_value(i)[1] * 255,
                                                                    color_map.get_value(i)[2] * 255, color_map.get_value(i)[3] * 255));
        }

        // Draw the value array histogram. 
        const float values[5] = { 0.2f, 0.3f, 0.7f, 0.4f, 0.5f };
        float max_value = *max_element(begin(values), end(values));
        int bins = sizeof(values) / sizeof(values[0]);
        float bin_width = tf_rect_width / bins;
        float bin_height = tf_rect_height;
        float bin_offset = 0;
        // For every bin in the array of values...
        for (int i = 0; i < bins; i++)
        {
          bin_height = tf_rect_height * (values[i] / max_value);
          bin_offset = tf_rect_height * (1 - values[i] / max_value);
          marker_min = ImVec2(pos.x + i * bin_width, pos.y + bin_offset);
          marker_max = ImVec2(pos.x + (i + 1) * bin_width, pos.y + bin_height + bin_offset);
          draw_list->AddRectFilled(marker_min, marker_max, IM_COL32(50, 50, 50, 150));
        }

        // TODO: Draw the Transfer Function line between the points desired, which determines the values in the tf_alphas array.
        // Determine if the mouse is clicked within the transfer function rectangle.
        ImVec2 tf_mouse_pos = io.MousePos;
        ImVec2 delta_mouse_drag = ImVec2(0.0f, 0.0f);
        if (tf_mouse_pos.x >= pos.x && tf_mouse_pos.x <= pos.x + tf_rect_width && 
            tf_mouse_pos.y >= pos.y && tf_mouse_pos.y <= pos.y + tf_rect_height && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
          tf_mouse_down = true;
        } 

        // If the mouse was clicked within tf_rect and is held down, adjust the delta drag.
        if (tf_mouse_down && ImGui::IsMouseDown(ImGuiMouseButton_Left)) 
        {
          delta_mouse_drag = ImGui::GetMouseDragDelta(0, 0.0f) - mouse_drag;  // This is the change in the mouse drag since last frame.
          mouse_drag = ImGui::GetMouseDragDelta(0, 0.0f);  

          // Prevent the window from being dragged.
          io.ConfigWindowsMoveFromTitleBarOnly = true;

        } 
        
        // Check if the user released the Mouse Drag.
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) 
        {
          io.ConfigWindowsMoveFromTitleBarOnly = false;
          tf_mouse_down = false;
          mouse_drag = ImVec2(0.0f, 0.0f);
        }

        // Update CursorScreenPos to just after the TransferFunction rectangle.
        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + tf_rect_height));
      }
      ImGui::EndGroup();
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
                       ImVec2(0,1),
                       ImVec2(1,0),
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

  // This is a method that takes a mat4, the matrix type used by the guizmo widget in the vgmath.cpp file, and coverts it into a quaternion.
  quat matrix_to_quat(mat4 m)
  {
    quat q;

    float trace = m[0][0] + m[1][1] + m[2][2]; 
    if( trace > 0 ) {
      float s = 0.5f / sqrtf(trace+ 1.0f);
      q.w = 0.25f / s;
      q.x = ( m[1][2] - m[2][1] ) * s;
      q.y = ( m[2][0] - m[0][2] ) * s;
      q.z = ( m[0][1] - m[1][0] ) * s;
    } else {
      if ( m[0][0] > m[1][1] && m[0][0] > m[2][2] ) {
        float s = 2.0f * sqrtf( 1.0f + m[0][0] - m[1][1] - m[2][2]);
        q.w = (m[1][2] - m[2][1] ) / s;
        q.x = 0.25f * s;
        q.y = (m[1][0] + m[0][1] ) / s;
        q.z = (m[2][0] + m[0][2] ) / s;
      } else if (m[1][1] > m[2][2]) {
        float s = 2.0f * sqrtf( 1.0f + m[1][1] - m[0][0] - m[2][2]);
        q.w = (m[2][0] - m[0][2] ) / s;
        q.x = (m[1][0] + m[0][1] ) / s;
        q.y = 0.25f * s;
        q.z = (m[2][1] + m[1][2] ) / s;
      } else {
        float s = 2.0f * sqrtf( 1.0f + m[2][2] - m[0][0] - m[1][1]);
        q.w = (m[0][1] - m[1][0] ) / s;
        q.x = (m[2][0] + m[0][2] ) / s;
        q.y = (m[2][1] + m[1][2] ) / s;
        q.z = 0.25f * s;
      }
    }

    return q;
  }

  // TODO: NOTE! This does not correctly rotate the guizmo. I suspect that the fault lies in the matrix math. 
  void rotate_guizmo()
  {
    // Get the view matrix and the inverse translation for the camera.
    dray::Matrix<float, 4, 4> trans = translate (-m_camera.get_look_at());    
    dray::Matrix<float, 4, 4> inv_trans = translate (m_camera.get_look_at());
    dray::Matrix<float, 4, 4> view = m_camera.view_matrix ();
    view (0, 3) = 0;
    view (1, 3) = 0;
    view (2, 3) = 0;

    // Matrix math to obtain the orientation of the camera which can be used to set the guizmo.
    dray::Matrix<float, 4, 4> new_gizmo_orientation = inv_trans * view;

    // Set the guizmo modelMatrix.
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        modelMatrix[i][j] = new_gizmo_orientation[i][j];     
      }
    }

    // Convert the modelMatrix into a quaternion. 
    quat q = matrix_to_quat(modelMatrix);
    
    setRotation(q);
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
        // int x_diff = x_last - mouse_pos.x;
        // int y_diff = y_last - mouse_pos.y;
        int x_diff = mouse_pos.x - x_last;
        int y_diff = mouse_pos.y - y_last;
        if(x_diff != 0 && y_diff != 0)
        {
          float x1 = float(x_last * 2) / float(m_width) - 1.0f;
          float y1 = float(y_last * 2) / float(m_height) - 1.0f;
          float x2 = float(mouse_pos.x * 2) / float(m_width) - 1.0f;
          float y2 = float(mouse_pos.y * 2) / float(m_height) - 1.0f;
          m_camera.trackball_rotate(x1 , y1, x2, y2);

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

  // NOTE: I think there's an inconsistency with how these work. They're not symmetric; W does not mirror S, etc. 
    // This undesired behavior can be seen if you move the camera with a key and then try to move it back again with what should be it's inverse.
    // Particularly, I think there's some incorrect adjustment of the lookat point, which is causing the rotations to behave unexpectedly.
    // Alternatively, there could be an issue where it's missing normalization or a move factor.
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

  // ***** File System Helpers ***** //
  // void save_session(const std::string session_name)
  // {
  //   conduit::Node session;
  //   session["dataset/file"] = m_dataset_file;
  //   session["dataset/field_index"] = m_active_field_idx;

  //   m_render_window.serialize(session["renderer"]);
  //   session.save(session_name);
  // }

  // void load_session(const std::string session_name)
  // {
  //   conduit::Node session;
  //   session.load(session_name);

  //   std::string file = session["dataset/file"].as_string();
  //   int active_field = session["dataset/field_index"].to_int32();

  //   load_dataset(file, active_field);

  //   m_render_window.deserialize(session["renderer"]);
  // }

  // void load_dataset(std::string file, int field_index = 0)
  // {
  //   m_dataset = DataSetReader::load_dataset(file);
  //   m_dataset_file = file;
  //   m_active_field_idx = field_index;
  //   m_render_window.set_data(m_dataset,
  //                            m_dataset.GetField(m_active_field_idx).GetName(),
  //                            m_dataset_file);
  //   m_histogram.build(m_dataset, m_active_field_idx, 256);
  //   m_bounds = m_dataset.GetCoordinateSystem().GetBounds();
  //   m_scalar_range = m_dataset.GetField(m_active_field_idx).GetRange().GetPortalConstControl().Get(0);
  //   m_render_window.set_histogram(m_histogram);
  // }
};
#endif
