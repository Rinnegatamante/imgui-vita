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
// indirect front touch enabled: drag pointer with finger
// indirect front touch disabled: pointer jumps to finger
void ImGui_ImplVitaGL_UseIndirectFrontTouch(bool val);
void ImGui_ImplVitaGL_UseRearTouch(bool val); // turn rear panel touch on or off
void ImGui_ImplVitaGL_MouseStickUsage(bool val); // Left mouse stick and trigger buttons control  mouse pointer
// GamepadUsage uses the Vita buttons to navigate and interact with UI elements
void ImGui_ImplVitaGL_GamepadUsage(bool val);
void ImGui_ImplVitaGL_UseCustomShader(bool val);
