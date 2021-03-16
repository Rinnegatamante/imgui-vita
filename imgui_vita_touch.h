//
// Created by rsn8887 on 02/05/18.

#ifndef IMGUI_VITA_TOUCH_H
#define IMGUI_VITA_TOUCH_H

#include <psp2/appmgr.h>
#include <psp2/common_dialog.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/processmgr.h> 
#include <psp2/kernel/sysmem.h>
#include <psp2/sharedfb.h>
#include <psp2/touch.h>

/* Touch functions */
void ImGui_ImplVitaGL_InitTouch(void);
void ImGui_ImplVitaGL_PollTouch(double x0, double y0, double sx, double sy, int *mx, int *my, bool *mbuttons);
void ImGui_ImplVitaGL_PrivateSetIndirectFrontTouch(bool enable);
void ImGui_ImplVitaGL_PrivateSetRearTouch(bool enable);

#endif
