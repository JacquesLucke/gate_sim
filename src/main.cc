#include <iostream>
#include <utility>

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

    float2(float x, float y) : x(x), y(y)
    {
    }

    friend float2 operator+(float2 a, float2 b)
    {
        return float2(a.x + b.x, a.y + b.y);
    }
};

static void swap_float(float &a, float &b)
{
    float tmp = a;
    a = b;
    b = tmp;
}

class rectf {
  private:
    float xmin, xmax;
    float ymin, ymax;

  public:
    rectf(float x1, float x2, float y1, float y2)
    {
        if (x1 > x2) {
            swap_float(x1, x2);
        }
        if (y1 > y2) {
            swap_float(y1, y2);
        }
        xmin = x1;
        xmax = x2;
        ymin = y1;
        ymax = y2;
    }

    static rectf FromPositionAndSize(float2 position, float2 size)
    {
        return rectf(
            position.x, position.x + size.x, position.y, position.y + size.y);
    }

    bool contains(float2 point) const
    {
        return xmin <= point.x && point.x <= xmax && ymin <= point.y &&
               point.y <= ymax;
    }

    float2 upper_left() const
    {
        return float2(xmin, ymax);
    }

    float2 lower_right() const
    {
        return float2(xmax, ymin);
    }
};

ImVec2 to_im(float2 vec)
{
    return ImVec2(vec.x, vec.y);
}

float2 box_size = {50, 50};

struct State {
    Vector<float2> box_positions;
    Vector<bool> box_selections;
    int a = 0;

    void add_box(float2 position)
    {
        box_positions.append(position);
        box_selections.append(false);
    }

    rectf get_box_rect(size_t index)
    {
        return rectf::FromPositionAndSize(box_positions[index], box_size);
    }
};

static State state;

static Stack<State> undo_stack;

static void push_undo_step()
{
    undo_stack.push(state);
    std::cout << "Push undo step\n";
}

static void pop_undo_step()
{
    if (undo_stack.size() <= 1) {
        std::cout << "End of undo stack\n";
        return;
    }

    undo_stack.pop();
    state = undo_stack.peek();
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

    // double last_mouse_x = 0.0f;
    // double last_mouse_y = 0.0f;

    state.add_box({100, 100});
    state.add_box({400, 200});
    push_undo_step();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
        float2 mouse_position = {(float)mouse_x, (float)mouse_y};

        bool imgui_uses_mouse = ImGui::GetIO().WantCaptureMouse;

        if (!imgui_uses_mouse) {
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) ==
                GLFW_PRESS) {
                for (size_t i : state.box_positions.index_range()) {
                    if (state.get_box_rect(i).contains(mouse_position)) {
                        state.box_selections[i] = true;
                    }
                }
            }
        }

        bool z_is_down = is_key_down(window, GLFW_KEY_Z);

        if (is_key_down(window, GLFW_KEY_LEFT_CONTROL) && !z_was_down &&
            z_is_down) {
            pop_undo_step();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();

        {
            ImDrawList *draw_list = ImGui::GetBackgroundDrawList();
            for (size_t i : state.box_positions.index_range()) {
                rectf box = state.get_box_rect(i);
                ImColor color = ImColor(230, 80, 80);
                if (state.box_selections[i]) {
                    color.Value.x *= 0.6f;
                }
                if (!imgui_uses_mouse && box.contains(mouse_position)) {
                    color.Value.x *= 0.8f;
                }

                draw_list->AddRectFilled(
                    to_im(box.upper_left()), to_im(box.lower_right()), color);
            }
        }

        // ImGui::SetNextWindowPos({0, 0});
        // int width, height;
        // glfwGetWindowSize(window, &width, &height);
        // ImGui::SetNextWindowSize({(float)width, (float)height});
        // ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        // ImGui::Begin("My Window",
        //              nullptr,
        //              ImGuiWindowFlags_NoDecoration |
        //                  ImGuiWindowFlags_NoBringToFrontOnFocus |
        //                  ImGuiWindowFlags_NoInputs);

        // // rect_pos_x = mouse_x;
        // ImDrawList *draw_list = ImGui::GetWindowDrawList();
        // if (!ImGui::GetIO().WantCaptureMouse &&
        //     glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) ==
        //     GLFW_PRESS) { BAS_UNUSED_VAR(last_mouse_y); rect_pos_x +=
        //     mouse_x - last_mouse_x;
        // }

        // draw_list->AddRectFilled({rect_pos_x, 10},
        //                          {rect_pos_x + 100, 200},
        //                          ImColor(0.8f, 0.8f, 0.3f));
        // const char *text = "Hello World";
        // draw_list->AddText({rect_pos_x, 100}, IM_COL32_WHITE, text, text +
        // 8);

        // ImDrawList *foreground_list = ImGui::GetForegroundDrawList();
        // foreground_list->AddLine({0, 0}, {300, 300}, ImColor(255, 0, 0), 4);

        // ImGui::End();

        // ImGui::PopStyleVar();

        ImGui::Begin("Other Window");
        ImGui::SliderInt("A", &state.a, 0, 100);
        push_undo_after_edit();
        ImGui::End();

        ImGui::Render();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        z_was_down = z_is_down;
        // last_mouse_x = mouse_x;
        // last_mouse_y = mouse_y;
    }

    ImGui_ImplGlfw_Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
