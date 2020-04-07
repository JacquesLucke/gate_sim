#include <iostream>

#include "bas/stack.h"

#include "glad/glad.h"

#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using bas::Stack;

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
                         ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::SetCursorPosY(100);
        for (int i = 0; i < 10; i++) {
            ImGui::SetCursorPosX(200);
            ImGui::Text("My Label");
        }

        ImGui::SliderInt("A", &my_settings.a, 0, 100);
        push_undo_after_edit();
        ImGui::InputInt("B", &my_settings.b);
        push_undo_after_edit();

        ImGui::End();

        ImGui::PopStyleVar();

        ImGui::Begin("Other Window");
        ImGui::Text("Hello World");
        ImGui::End();

        ImGui::Render();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        z_was_down = z_is_down;
    }

    ImGui_ImplGlfw_Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
