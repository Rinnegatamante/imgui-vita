// Implemented features:
//  [X] User texture binding. Cast 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

#include <stdio.h>
#include <stdlib.h>
#include <vitaGL.h>
#include <math.h>
#include "imgui.h"
#include "imgui_impl_vitagl.h"
#include "imgui_vita_touch.h"

#define lerp(value, from_max, to_max) ((((value*10) * (to_max*10))/(from_max*10))/10)

// Data
static uint64_t	   g_Time = 0;
static bool		 g_MousePressed[3] = { false, false, false };
static GLuint	   g_FontTexture = 0;
static SceCtrlData g_OldPad;
static int hires_x = 0;
static int hires_y = 0;

float *startVertex = nullptr;
float *startTexCoord = nullptr;
uint8_t *startColor = nullptr;
uint16_t *startIndex = nullptr;
float *gVertexBuffer = nullptr;
float *gTexCoordBuffer = nullptr;
uint8_t *gColorBuffer = nullptr;
uint16_t *gIndexBuffer = nullptr;
uint32_t gCounter = 0;

bool touch_usage = false;
bool mousestick_usage = true;
bool gamepad_usage = false;
bool shaders_usage = false;

void LOG(const char *format, ...) {
	__gnuc_va_list arg;
	int done;
	va_start(arg, format);
	char msg[512];
	done = vsprintf(msg, format, arg);
	va_end(arg);
	int i;
	sprintf(msg, "%s\n", msg);
	FILE* log = fopen("ux0:/data/imgui.log", "a+");
	if (log != NULL) {
		fwrite(msg, 1, strlen(msg), log);
		fclose(log);
	}
}

// OpenGL2 Render function.
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so. 
void ImGui_ImplVitaGL_RenderDrawData(ImDrawData* draw_data)
{
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	ImGuiIO& io = ImGui::GetIO();
	int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
	int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
	if (fb_width == 0 || fb_height == 0)
		return;
	draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	// We are using the OpenGL fixed pipeline to make the example code simpler to read!
	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers, polygon fill.
	GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
	GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
	GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box); 
	//glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound

	// Setup viewport, orthographic projection matrix
	glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Render command lists
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
		const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
		
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
				glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
				
				float *vp = gVertexBuffer;
				float *tp = gTexCoordBuffer;
				uint8_t *cp = gColorBuffer;
				uint16_t *ip = gIndexBuffer;
				uint8_t *indices = (uint8_t*)idx_buffer;
				if (sizeof(ImDrawIdx) == 2){
					for (int idx=0; idx < pcmd->ElemCount; idx++){
						gIndexBuffer[0] = *((uint16_t*)(indices + sizeof(ImDrawIdx) * idx));
						float *vertices = (float*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos) + sizeof(ImDrawVert) * gIndexBuffer[0]);
						float *texcoords = (float*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv) + sizeof(ImDrawVert) * gIndexBuffer[0]);
						uint8_t *colors = (uint8_t*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col) + sizeof(ImDrawVert) * gIndexBuffer[0]);
						gIndexBuffer[0] = idx;
						gVertexBuffer[0] = vertices[0];
						gVertexBuffer[1] = vertices[1];
						gVertexBuffer[2] = 0.0f;
						gTexCoordBuffer[0] = texcoords[0];
						gTexCoordBuffer[1] = texcoords[1];
						gColorBuffer[0] = colors[0];
						gColorBuffer[1] = colors[1];
						gColorBuffer[2] = colors[2];
						gColorBuffer[3] = colors[3];
						gVertexBuffer += 3;
						gTexCoordBuffer += 2;
						gColorBuffer += 4;
						gIndexBuffer += 1;
					}
				}else{
					for (int idx=0; idx < pcmd->ElemCount; idx++){
						gIndexBuffer[0] = *((uint32_t*)(indices + sizeof(ImDrawIdx) * idx));
						float *vertices = (float*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos) + sizeof(ImDrawVert) * gIndexBuffer[0]);
						float *texcoords = (float*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv) + sizeof(ImDrawVert) * gIndexBuffer[0]);
						uint8_t *colors = (uint8_t*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col) + sizeof(ImDrawVert) * gIndexBuffer[0]);
						gIndexBuffer[0] = idx;
						gVertexBuffer[0] = vertices[0];
						gVertexBuffer[1] = vertices[1];
						gVertexBuffer[2] = 0.0f;
						gTexCoordBuffer[0] = texcoords[0];
						gTexCoordBuffer[1] = texcoords[1];
						gColorBuffer[0] = colors[0];
						gColorBuffer[1] = colors[1];
						gColorBuffer[2] = colors[2];
						gColorBuffer[3] = colors[3];
						gVertexBuffer += 3;
						gTexCoordBuffer += 2;
						gColorBuffer += 4;
						gIndexBuffer += 1;
					}
				}
				vglIndexPointerMapped(ip);
				if (shaders_usage){
					vglVertexAttribPointerMapped(0, vp);
					vglVertexAttribPointerMapped(1, tp);
					vglVertexAttribPointerMapped(2, cp);
				}else{
					vglVertexPointerMapped(vp);
					vglTexCoordPointerMapped(tp);
					vglColorPointerMapped(GL_UNSIGNED_BYTE, cp);
				}
				vglDrawObjects(GL_TRIANGLES, pcmd->ElemCount, GL_TRUE);
			}
			idx_buffer += pcmd->ElemCount;
			gCounter += pcmd->ElemCount;
			if (gCounter > 0x99900){
				gVertexBuffer = startVertex;
				gColorBuffer = startColor;
				gIndexBuffer = startIndex;
				gTexCoordBuffer = startTexCoord;
				gCounter = 0;
			}
		}
	}

	// Restore modified state
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	//glPopAttrib();
	glPolygonMode(GL_FRONT, (GLenum)last_polygon_mode[0]); glPolygonMode(GL_BACK, (GLenum)last_polygon_mode[1]);
	glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
	glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
bool ImGui_ImplVitaGL_ProcessEvent(SceCtrlData *pad)
{
	ImGuiIO& io = ImGui::GetIO();
	/*switch (event->type)
	{
	case SDL_MOUSEWHEEL:
		{
			if (event->wheel.x > 0) io.MouseWheelH += 1;
			if (event->wheel.x < 0) io.MouseWheelH -= 1;
			if (event->wheel.y > 0) io.MouseWheel += 1;
			if (event->wheel.y < 0) io.MouseWheel -= 1;
			return true;
		}
	case SDL_MOUSEBUTTONDOWN:
		{
			if (event->button.button == SDL_BUTTON_LEFT) g_MousePressed[0] = true;
			if (event->button.button == SDL_BUTTON_RIGHT) g_MousePressed[1] = true;
			if (event->button.button == SDL_BUTTON_MIDDLE) g_MousePressed[2] = true;
			return true;
		}
	case SDL_TEXTINPUT:
		{
			io.AddInputCharactersUTF8(event->text.text);
			return true;
		}
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		{
			int key = event->key.keysym.scancode;
			IM_ASSERT(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
			io.KeysDown[key] = (event->type == SDL_KEYDOWN);
			io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
			io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
			io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
			io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
			return true;
		}
	}*/
	return false;
}

bool ImGui_ImplVitaGL_CreateDeviceObjects()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// Upload texture to graphics system
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGenTextures(1, &g_FontTexture);
	glBindTexture(GL_TEXTURE_2D, g_FontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Store our identifier
	io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

	// Restore state
	glBindTexture(GL_TEXTURE_2D, last_texture);

	return true;
}

void	ImGui_ImplVitaGL_InvalidateDeviceObjects()
{
	if (g_FontTexture)
	{
		glDeleteTextures(1, &g_FontTexture);
		ImGui::GetIO().Fonts->TexID = 0;
		g_FontTexture = 0;
	}
}

bool	ImGui_ImplVitaGL_Init()
{
	
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	
	// Setup back-end capabilities flags
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDrawCursor = true;
	
	// Initializing buffers
	startVertex = (float*)malloc(sizeof(float) * 0x100000 * 3);
	startTexCoord = (float*)malloc(sizeof(float) * 0x100000 * 2);
	startColor = (uint8_t*)malloc(sizeof(uint8_t) * 0x100000 * 4);
	startIndex = (uint16_t*)malloc(sizeof(uint16_t) * 0x100000);
	
	gVertexBuffer = startVertex;
	gColorBuffer = startColor;
	gIndexBuffer = startIndex;
	gTexCoordBuffer = startTexCoord;
	
	vglMapHeapMem();
	
	// Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
	/*io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
	io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
	io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
	io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
	io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
	io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
	io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
	io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
	io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
	io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;*/

	io.ClipboardUserData = NULL;

	ImGui_ImplVitaGL_InitTouch();

	return true;
}

void ImGui_ImplVitaGL_Shutdown()
{
	// Destroy buffers
	free(gVertexBuffer);
	free(gTexCoordBuffer);
	free(gColorBuffer);
	free(gIndexBuffer);
	
	// Destroy OpenGL objects
	ImGui_ImplVitaGL_InvalidateDeviceObjects();
}

int mx, my;

void IN_RescaleAnalog(int *x, int *y, int dead) {

	float analogX = (float) *x;
	float analogY = (float) *y;
	float deadZone = (float) dead;
	float maximum = 32768.0f;
	float magnitude = sqrt(analogX * analogX + analogY * analogY);
	if (magnitude >= deadZone)
	{
		float scalingFactor = maximum / magnitude * (magnitude - deadZone) / (maximum - deadZone);
		*x = (int) (analogX * scalingFactor);
		*y = (int) (analogY * scalingFactor);
	} else {
		*x = 0;
		*y = 0;
	}
}

void ImGui_ImplVitaGL_PollLeftStick(SceCtrlData *pad, int *x, int *y)
{
	sceCtrlPeekBufferPositive(0, pad, 1);
	int lx = (pad->lx - 127) * 256;
	int ly = (pad->ly - 127) * 256;
	IN_RescaleAnalog(&lx, &ly, 7680);
	hires_x += lx;
	hires_y += ly;
	if (hires_x != 0 || hires_y != 0) {
		// slow down pointer, could be made user-adjustable
		int slowdown = 2048;
		*x += hires_x / slowdown;
		*y += hires_y / slowdown;
		hires_x %= slowdown;
		hires_y %= slowdown;
	}
}


void ImGui_ImplVitaGL_NewFrame()
{
	
	if (!g_FontTexture)
		ImGui_ImplVitaGL_CreateDeviceObjects();

	ImGuiIO& io = ImGui::GetIO();
	
	// Setup display size (every frame to accommodate for window resizing)
	int w, h;
	int display_w, display_h;
	GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
	w = display_w = viewport[2];
	h = display_h = viewport[3];
	io.DisplaySize = ImVec2((float)w, (float)h);
	io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

	static uint64_t frequency = 1000000;
	uint64_t current_time = sceKernelGetProcessTimeWide();
	io.DeltaTime = g_Time > 0 ? (float)((double)(current_time - g_Time) / frequency) : (float)(1.0f / 60.0f);
	g_Time = current_time;

	// Touch for mouse emulation
	if (touch_usage){
		double scale_x = 960.0 / io.DisplaySize.x;
		double scale_y = 544.0 / io.DisplaySize.y;
		double offset_x = 0;
		double offset_y = 0;
		ImGui_ImplVitaGL_PollTouch(offset_x, offset_y, scale_x, scale_y, &mx, &my, g_MousePressed);
	}
	
	// Keypad navigation
	if (gamepad_usage){
		SceCtrlData pad;
		int lstick_x, lstick_y = 0;
		ImGui_ImplVitaGL_PollLeftStick(&pad, &lstick_x, &lstick_y);
		io.NavInputs[ImGuiNavInput_Activate]  = (pad.buttons & SCE_CTRL_CROSS)    ? 1.0f : 0.0f;
		io.NavInputs[ImGuiNavInput_Cancel]    = (pad.buttons & SCE_CTRL_CIRCLE)   ? 1.0f : 0.0f;
		io.NavInputs[ImGuiNavInput_Input]     = (pad.buttons & SCE_CTRL_TRIANGLE) ? 1.0f : 0.0f;
		io.NavInputs[ImGuiNavInput_Menu]      = (pad.buttons & SCE_CTRL_SQUARE)   ? 1.0f : 0.0f;
		io.NavInputs[ImGuiNavInput_DpadLeft]  = (pad.buttons & SCE_CTRL_LEFT)     ? 1.0f : 0.0f;
		io.NavInputs[ImGuiNavInput_DpadRight] = (pad.buttons & SCE_CTRL_RIGHT)    ? 1.0f : 0.0f;
		io.NavInputs[ImGuiNavInput_DpadUp]    = (pad.buttons & SCE_CTRL_UP)       ? 1.0f : 0.0f;
		io.NavInputs[ImGuiNavInput_DpadDown]  = (pad.buttons & SCE_CTRL_DOWN)     ? 1.0f : 0.0f;

		if (io.NavInputs[ImGuiNavInput_Menu] == 1.0f) {
			io.NavInputs[ImGuiNavInput_FocusPrev] = (pad.buttons & SCE_CTRL_LTRIGGER) ? 1.0f : 0.0f;
			io.NavInputs[ImGuiNavInput_FocusNext] = (pad.buttons & SCE_CTRL_RTRIGGER) ? 1.0f : 0.0f;
			if (lstick_x < 0) io.NavInputs[ImGuiNavInput_LStickLeft] = (float)(-lstick_x/16);
			if (lstick_x > 0) io.NavInputs[ImGuiNavInput_LStickRight] = (float)(lstick_x/16);
			if (lstick_y < 0) io.NavInputs[ImGuiNavInput_LStickUp] = (float)(-lstick_y/16);
			if (lstick_y > 0) io.NavInputs[ImGuiNavInput_LStickDown] = (float)(lstick_y/16);
		}
	}
	
	// Keys for mouse emulation
	if (mousestick_usage && !(io.NavInputs[ImGuiNavInput_Menu] == 1.0f)){
		SceCtrlData pad;
		ImGui_ImplVitaGL_PollLeftStick(&pad, &mx, &my);
		if ((pad.buttons & SCE_CTRL_LTRIGGER) != (g_OldPad.buttons & SCE_CTRL_LTRIGGER))
			g_MousePressed[0] = pad.buttons & SCE_CTRL_LTRIGGER;
		if ((pad.buttons & SCE_CTRL_RTRIGGER) != (g_OldPad.buttons & SCE_CTRL_RTRIGGER))
			g_MousePressed[1] = pad.buttons & SCE_CTRL_RTRIGGER;
		g_OldPad = pad;
	}
	
	// Setup mouse inputs (we already got mouse wheel, keyboard keys & characters from our event handler)
	//Uint32 mouse_buttons = SDL_GetMouseState(&mx, &my);
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	io.MouseDown[0] = g_MousePressed[0];
	io.MouseDown[1] = g_MousePressed[1];
	io.MouseDown[2] = g_MousePressed[2];

	if (mx < 0) mx = 0;
	else if (mx > 960) mx = 960;
	if (my < 0) my = 0;
	else if (my > 544) my = 544;
	io.MousePos = ImVec2((float)mx, (float)my);

	// Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
	ImGui::NewFrame();
}

void ImGui_ImplVitaGL_TouchUsage(bool val){
	touch_usage = val;
}

void ImGui_ImplVitaGL_UseIndirectFrontTouch(bool val){
	ImGui_ImplVitaGL_PrivateSetIndirectFrontTouch(val);
}

void ImGui_ImplVitaGL_UseRearTouch(bool val){
	ImGui_ImplVitaGL_PrivateSetRearTouch(val);
}

void ImGui_ImplVitaGL_MouseStickUsage(bool val){
	mousestick_usage = val;
}

void ImGui_ImplVitaGL_GamepadUsage(bool val){
	gamepad_usage = val;
}

void ImGui_ImplVitaGL_UseCustomShader(bool val){
	shaders_usage = val;
}
