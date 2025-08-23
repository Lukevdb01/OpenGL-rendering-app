#include "Application.h"

#include <iostream>
#include <string>
#include "Model/Model.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


GLFWwindow* window = nullptr;

Application::Application()
{
    // -----------------------------
    // GLFW Initialization
    // -----------------------------
    if (!glfwInit())
    {
        std::cerr << "[Error] Failed to initialize GLFW" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1280, 720, "Docking OpenGL Engine", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "[Error] Failed to create GLFW window" << std::endl;
        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync

    // -----------------------------
    // Load OpenGL functions
    // -----------------------------
    if (!gladLoadGL())
    {
        std::cerr << "[Error] Failed to initialize GLAD" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    glEnable(GL_DEPTH_TEST);

    // -----------------------------
    // ImGui Initialization
    // -----------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
}

Application::~Application()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        window = nullptr;
    }
}

void Application::Run()
{
    Shader shaderProgram("default.vert", "default.frag");
    shaderProgram.Activate();
    glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), 0.5f, 0.5f, 0.5f);

    Camera camera(1280, 720, glm::vec3(0.0f, 6.0f, 8.0f));
    Model ground("models/character/character.gltf");
    Model rick("models/rick/rick.gltf");

    double prevTime = glfwGetTime();
    unsigned int frameCounter = 0;

    // Framebuffer setup for Scene
    GLuint sceneFBO, sceneTexture, sceneRBO;
    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    glGenTextures(1, &sceneTexture);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture, 0);

    glGenRenderbuffers(1, &sceneRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1280, 720);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, sceneRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[Error] Scene framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int fbWidth = 1280, fbHeight = 720; // framebuffer dimensions

    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - prevTime;
        frameCounter++;

        if (deltaTime >= 1.0 / 30.0)
        {
            double fps = frameCounter / deltaTime;
            double msPerFrame = 1000.0 * deltaTime / frameCounter;
            std::string title = "Docking OpenGL Engine - " + std::to_string(fps) + " FPS / " + std::to_string(msPerFrame) + " ms";
            glfwSetWindowTitle(window, title.c_str());

            prevTime = currentTime;
            frameCounter = 0;
        }

        // -----------------------------
        // ImGui Frame
        // -----------------------------
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // DockSpace setup
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        ImGui::Begin("DockSpace Window", nullptr, window_flags);
        ImGui::DockSpace(ImGui::GetID("MyDockSpace"), ImVec2(0.0f, 0.0f), dockspace_flags);
        ImGui::End();
        ImGui::PopStyleVar(2);

        // -----------------------------
        // Scene window
        // -----------------------------
        ImGui::Begin("Scene");
        ImVec2 sceneSize = ImGui::GetContentRegionAvail();

        // Resize framebuffer if Scene window size changed
        int newWidth = (int)sceneSize.x;
        int newHeight = (int)sceneSize.y;
        if (newWidth != fbWidth || newHeight != fbHeight)
        {
            fbWidth = newWidth;
            fbHeight = newHeight;

            // Resize color texture
            glBindTexture(GL_TEXTURE_2D, sceneTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fbWidth, fbHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

            // Resize depth-stencil renderbuffer
            glBindRenderbuffer(GL_RENDERBUFFER, sceneRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbWidth, fbHeight);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        ImGui::Image((void*)(intptr_t)sceneTexture, sceneSize, ImVec2(0, 1), ImVec2(1, 0)); // Flip UV Y

        // Camera input only when mouse is in Scene window
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mousePos = io.MousePos;
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        bool mouseInScene = mousePos.x >= winPos.x && mousePos.x <= winPos.x + winSize.x &&
            mousePos.y >= winPos.y && mousePos.y <= winPos.y + winSize.y;

        if (mouseInScene && ImGui::IsWindowFocused())
        {
            camera.Inputs(window);
        }

        ImGui::End();

        // -----------------------------
        // Debug window
        // -----------------------------
        ImGui::Begin("Debug Window");
        ImGui::Text("FPS: %.1f", 1.0 / deltaTime);
        ImGui::Text("Camera Pos: (%.1f, %.1f, %.1f)", camera.Position.x, camera.Position.y, camera.Position.z);
        ImGui::End();

        // -----------------------------
        // Render to Scene framebuffer
        // -----------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update camera projection using correct aspect ratio
        camera.updateMatrix(45.0f, 0.1f, 100.0f, (float)fbWidth / (float)fbHeight);

        ground.Draw(shaderProgram, camera);
        rick.Draw(shaderProgram, camera);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // -----------------------------
        // Render ImGui
        // -----------------------------
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_context);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    shaderProgram.Delete();
    glDeleteFramebuffers(1, &sceneFBO);
    glDeleteTextures(1, &sceneTexture);
    glDeleteRenderbuffers(1, &sceneRBO);
}
