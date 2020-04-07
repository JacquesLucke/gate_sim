#include <iostream>

#include "bas/map.h"
#include "bas/multi_map.h"
#include "bas/stack.h"
#include "bas/vector_set.h"

#include "glad/glad.h"

#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using bas::ArrayRef;
using bas::Map;
using bas::MultiMap;
using bas::size_t;
using bas::Stack;
using bas::Vector;
using bas::VectorSet;

using uint = unsigned int;

struct float2 {
    float x, y;
};

enum class SocketType { Input, Output };

class ComponentUI {
  protected:
    Vector<float2> m_relative_positions;
    Vector<float2> m_normalized_outgoing_directions;
    float2 m_size;

  public:
    virtual void draw(ImDrawList *draw_list,
                      ArrayRef<float2> positions,
                      float scale) const = 0;

    ArrayRef<float2> relative_positions() const
    {
        return m_relative_positions;
    }

    ArrayRef<float2> normalized_outgoing_directions() const
    {
        return m_normalized_outgoing_directions;
    }

    float2 size()
    {
        return m_size;
    }
};

class ComponentSockets {
  private:
    Vector<uint> m_widths;
    Vector<SocketType> m_types;
};

class Component {
  private:
    ComponentUI *m_ui;
    ComponentSockets *m_sockets;
};

class ComponentInstance {
  private:
    float2 m_position;
    Component *m_component;
};

struct SocketID {
    ComponentInstance *instance;
    uint index;
};

class Wire {
    SocketID from;
    Vector<SocketID> to;
};

class Circuit {
    VectorSet<ComponentInstance *> m_components;
    Map<SocketID, Wire *> m_incoming_wires;
    Map<SocketID, Wire *> m_outgoing_wires;
};

class BoxComponentUI : public ComponentUI {
  public:
    BoxComponentUI(uint left_sockets,
                   uint right_sockets,
                   uint top_sockets,
                   uint bottom_sockets)
        : ComponentUI()
    {
        uint vertical_amount = std::max(left_sockets, right_sockets);
        uint horizontal_amount = std::max(top_sockets, bottom_sockets);

        float margin = 20;
        float distance = 10;

        m_size = {0, 0};
        if (horizontal_amount >= 2) {
            m_size.x = 2 * margin + (horizontal_amount - 1) * distance;
        }
        else {
            m_size.x = 2 * margin;
        }
        if (vertical_amount >= 2) {
            m_size.y = 2 * margin + (vertical_amount - 1) * distance;
        }
        else {
            m_size.y = 2 * margin;
        }
    }
};

struct MySettings {
    int a;
    int b;
};

static MySettings my_settings;

static Stack<MySettings> undo_stack;

static void push_undo_step()
{
    undo_stack.push(my_settings);
    std::cout << "Push undo step\n";
}

static void pop_undo_step()
{
    if (undo_stack.size() <= 1) {
        std::cout << "End of undo stack\n";
        return;
    }

    undo_stack.pop();
    my_settings = undo_stack.peek();
    std::cout << "Pop undo step\n";
}

static void push_undo_after_edit()
{
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        push_undo_step();
    }
}

static bool is_key_down(GLFWwindow *window, int key)
{
    return glfwGetKey(window, key) == GLFW_PRESS;
}

int main()
{
    if (!glfwInit()) {
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(
        640, 480, "My Title", nullptr, nullptr);
    if (window == nullptr) {
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(nullptr);

    bool z_was_down = false;
    push_undo_step();

    float rect_pos_x = 10.0f;

    double last_mouse_x = 0.0f;
    double last_mouse_y = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        bool z_is_down = is_key_down(window, GLFW_KEY_Z);

        if (is_key_down(window, GLFW_KEY_LEFT_CONTROL) && !z_was_down &&
            z_is_down) {
            pop_undo_step();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();

        ImGui::SetNextWindowPos({0, 0});
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        ImGui::SetNextWindowSize({(float)width, (float)height});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGui::Begin("My Window",
                     nullptr,
                     ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoInputs);

        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
        // rect_pos_x = mouse_x;
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        if (!ImGui::GetIO().WantCaptureMouse &&
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            BAS_UNUSED_VAR(last_mouse_y);
            rect_pos_x += mouse_x - last_mouse_x;
        }

        draw_list->AddRectFilled({rect_pos_x, 10},
                                 {rect_pos_x + 100, 200},
                                 ImColor(0.8f, 0.8f, 0.3f));
        const char *text = "Hello World";
        draw_list->AddText({rect_pos_x, 100}, IM_COL32_WHITE, text, text + 8);

        ImDrawList *foreground_list = ImGui::GetForegroundDrawList();
        foreground_list->AddLine({0, 0}, {300, 300}, ImColor(255, 0, 0), 4);

        ImGui::End();

        ImGui::PopStyleVar();

        ImGui::Begin("Other Window");
        ImGui::SliderInt("A", &my_settings.a, 0, 100);
        push_undo_after_edit();
        ImGui::InputInt("B", &my_settings.b);
        push_undo_after_edit();
        ImGui::End();

        ImGui::Render();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        z_was_down = z_is_down;
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;
    }

    ImGui_ImplGlfw_Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
