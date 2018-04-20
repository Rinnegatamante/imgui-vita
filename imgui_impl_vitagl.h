// Implemented features:
//  [X] User texture binding. Cast 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

#include <vitasdk.h>

IMGUI_API bool        ImGui_ImplVitaGL_Init();
IMGUI_API void        ImGui_ImplVitaGL_Shutdown();
IMGUI_API void        ImGui_ImplVitaGL_NewFrame();
IMGUI_API void        ImGui_ImplVitaGL_RenderDrawData(ImDrawData* draw_data);
IMGUI_API bool        ImGui_ImplVitaGL_ProcessEvent(SceCtrlData* pad);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplVitaGL_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplVitaGL_CreateDeviceObjects();

void ImGui_ImplVitaGL_TouchUsage(bool val);
void ImGui_ImplVitaGL_KeysUsage(bool val);
void ImGui_ImplVitaGL_UseCustomShader(bool val);
