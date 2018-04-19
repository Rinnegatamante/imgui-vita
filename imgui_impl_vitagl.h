// ImGui SDL2 binding with OpenGL (legacy, fixed pipeline)
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// Implemented features:
//  [X] User texture binding. Cast 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.
// Missing features:
//  [ ] SDL2 handling of IME under Windows appears to be broken and it explicitly disable the regular Windows IME. You can restore Windows IME by compiling SDL with SDL_DISABLE_WINDOWS_IME.

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the sdl_opengl3_example/ folder**
// See imgui_impl_sdl.cpp for details.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <vitasdk.h>

IMGUI_API bool        ImGui_ImplVitaGL_Init();
IMGUI_API void        ImGui_ImplVitaGL_Shutdown();
IMGUI_API void        ImGui_ImplVitaGL_NewFrame();
IMGUI_API void        ImGui_ImplVitaGL_RenderDrawData(ImDrawData* draw_data);
IMGUI_API bool        ImGui_ImplVitaGL_ProcessEvent(SceCtrlData* pad);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplVitaGL_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplVitaGL_CreateDeviceObjects();
