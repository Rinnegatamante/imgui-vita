//
// Created by rsn8887 on 02/05/18.

#include <psp2/kernel/processmgr.h>
#include <psp2/touch.h>
#include <psp2/display.h>
#include "imgui_vita_touch.h"
#include <string.h>
//#include "math.h"

#define SDL_BUTTON_LEFT 0
#define SDL_BUTTON_RIGHT 1
typedef int Uint8;


static int *touchmouse_x;
static int *touchmouse_y;
static bool *touchmouse_button;

static SceTouchData touch_old[SCE_TOUCH_PORT_MAX_NUM];
static SceTouchData touch[SCE_TOUCH_PORT_MAX_NUM];

static SceTouchPanelInfo panelinfo[SCE_TOUCH_PORT_MAX_NUM];

static float aAWidth[SCE_TOUCH_PORT_MAX_NUM];
static float aAHeight[SCE_TOUCH_PORT_MAX_NUM];
static float dispWidth[SCE_TOUCH_PORT_MAX_NUM];
static float dispHeight[SCE_TOUCH_PORT_MAX_NUM];
static float forcerange[SCE_TOUCH_PORT_MAX_NUM];

static bool rear_touch = false; // rear panel on or off
static bool indirect_front_touch = false; // front panel indirect mode (drag pointer) or direct mode (pointer jumps to finger)

static float pointer_speed_factor = 1.0; // pointer speed factor for indirect touch
static double offset_x = 0.0;
static double offset_y = 0.0;
static double scale_x = 1.0;
static double scale_y = 1.0;
static int hires_dx;
static int hires_dy;

typedef enum TouchEventType {
	FINGERDOWN,
	FINGERUP,
	FINGERMOTION,
} TouchEventType;

typedef struct {
	TouchEventType type;
	SceUInt64 timestamp;
	int touchId;
	int fingerId;
	float x;
	float y;
	float dx;
	float dy;
} FingerType;

typedef struct {
	TouchEventType type;
	FingerType tfinger;
} TouchEvent;

enum {
	MAX_NUM_FINGERS = 3, // number of fingers to track per panel
	MAX_TAP_TIME = 250, // taps longer than this will not result in mouse click events
	MAX_TAP_MOTION_DISTANCE = 10, // max distance finger motion in Vita screen pixels to be considered a tap
	SIMULATED_CLICK_DURATION = 50, // time in ms how long simulated mouse clicks should be
}; // track three fingers per panel

typedef struct {
	int id; // -1: no touch
	int timeLastDown;
	float lastDownX;
	float lastDownY;
} Touch;

static Touch _finger[SCE_TOUCH_PORT_MAX_NUM][MAX_NUM_FINGERS]; // keep track of finger status

typedef enum DraggingType {
	DRAG_NONE = 0,
	DRAG_TWO_FINGER,
	DRAG_THREE_FINGER,
} DraggingType;

static DraggingType _multiFingerDragging[SCE_TOUCH_PORT_MAX_NUM]; // keep track whether we are currently drag-and-dropping

static int _simulatedClickStartTime[SCE_TOUCH_PORT_MAX_NUM][2]; // initiation time of last simulated left or right click (zero if no click)

static void psp2ProcessTouchEvent(TouchEvent *event);
static void psp2ProcessFingerDown(TouchEvent *event);
static void psp2ProcessFingerUp(TouchEvent *event);
static void psp2ProcessFingerMotion(TouchEvent *event);
static void psp2ConvertTouchXYToSDLXY(float *sdl_x, float *sdl_y, int vita_x, int vita_y, int port);
static void psp2ConvertTouchXYToGameXY(float touchX, float touchY, int *gameX, int *gameY); 
static void psp2FinishSimulatedMouseClicks(int port, SceUInt64 currentTime);

void ImGui_ImplVitaGL_InitTouch() {
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);
	sceTouchEnableTouchForce(SCE_TOUCH_PORT_BACK);

	SceTouchPanelInfo panelinfo[SCE_TOUCH_PORT_MAX_NUM];
	for(int port = 0; port < SCE_TOUCH_PORT_MAX_NUM; port++) {
		sceTouchGetPanelInfo(port, &panelinfo[port]);
		aAWidth[port] = (float)(panelinfo[port].maxAaX - panelinfo[port].minAaX);
		aAHeight[port] = (float)(panelinfo[port].maxAaY - panelinfo[port].minAaY);
		dispWidth[port] = (float)(panelinfo[port].maxDispX - panelinfo[port].minDispX);
		dispHeight[port] = (float)(panelinfo[port].maxDispY - panelinfo[port].minDispY);
		forcerange[port] = (float)(panelinfo[port].maxForce - panelinfo[port].minForce);
	}
	
	for (int port = 0; port < SCE_TOUCH_PORT_MAX_NUM; port++) {
		for (int i = 0; i < MAX_NUM_FINGERS; i++) {
			_finger[port][i].id = -1;
		}
		_multiFingerDragging[port] = DRAG_NONE;
	}
	
	for (int port = 0; port < SCE_TOUCH_PORT_MAX_NUM; port++) {
		for (int i = 0; i < 2; i++) {
			_simulatedClickStartTime[port][i] = 0;
		}
	}
}

void ImGui_ImplVitaGL_PollTouch(double x0, double y0, double sx, double sy, int *mx, int *my, bool *mbuttons) {
	offset_x = x0;
	offset_y = y0;
	scale_x = sx;
	scale_y = sy;

	touchmouse_x = mx;
	touchmouse_y = my;
	touchmouse_button = mbuttons;

	int finger_id = 0;
	memcpy(touch_old, touch, sizeof(touch_old));

	for(int port = 0; port < SCE_TOUCH_PORT_MAX_NUM; port++) {
		if ((port == SCE_TOUCH_PORT_FRONT) || (rear_touch && port == SCE_TOUCH_PORT_BACK)) {
			sceTouchPeek(port, &touch[port], 1);
			psp2FinishSimulatedMouseClicks(port, touch[port].timeStamp / 1000);
			if (touch[port].reportNum > 0) {
				for (int i = 0; i < touch[port].reportNum; i++) {
					// adjust coordinates and forces to return normalized values
					// for the front, screen area is used as a reference (for direct touch)
					// e.g. touch_x = 1.0 corresponds to screen_x = 960
					// for the back panel, the active touch area is used as reference
					float x = 0;
					float y = 0;
					psp2ConvertTouchXYToSDLXY(&x, &y, touch[port].report[i].x, touch[port].report[i].y, port);
					finger_id = touch[port].report[i].id;

					// Send an initial touch if finger hasn't been down
					bool hasBeenDown = false;
					int j = 0;
					if (touch_old[port].reportNum > 0) {
						for (j = 0; j < touch_old[port].reportNum; j++) {
							if (touch_old[port].report[j].id == touch[port].report[i].id ) {
								hasBeenDown = true;
								break;
							}
						}
					}
					if (!hasBeenDown) {
						TouchEvent ev;
						ev.type = FINGERDOWN;
						ev.tfinger.timestamp = touch[port].timeStamp / 1000;
						ev.tfinger.touchId = port;
						ev.tfinger.fingerId = finger_id;
						ev.tfinger.x = x;
						ev.tfinger.y = y;
						psp2ProcessTouchEvent(&ev);
					} else {
						// If finger moved, send motion event instead
						if (touch_old[port].report[j].x != touch[port].report[i].x ||
							touch_old[port].report[j].y != touch[port].report[i].y) {
							float oldx = 0;
							float oldy = 0;
							psp2ConvertTouchXYToSDLXY(&oldx, &oldy, touch_old[port].report[j].x, touch_old[port].report[j].y, port);
							TouchEvent ev;
							ev.type = FINGERMOTION;
							ev.tfinger.timestamp = touch[port].timeStamp / 1000;
							ev.tfinger.touchId = port;
							ev.tfinger.fingerId = finger_id;
							ev.tfinger.x = x;
							ev.tfinger.y = y;
							ev.tfinger.dx = x - oldx;
							ev.tfinger.dy = y - oldy;
							psp2ProcessTouchEvent(&ev);
						}
					}
				}
			}
			// some fingers might have been let go
			if (touch_old[port].reportNum > 0) {
				for (int i = 0; i < touch_old[port].reportNum; i++) {
					int finger_up = 1;
					if (touch[port].reportNum > 0) {
						for (int j = 0; j < touch[port].reportNum; j++) {
							if (touch[port].report[j].id == touch_old[port].report[i].id ) {
								finger_up = 0;
							}
						}
					}
					if (finger_up == 1) {
						float x = 0;
						float y = 0;
						psp2ConvertTouchXYToSDLXY(&x, &y, touch_old[port].report[i].x, touch_old[port].report[i].y, port);
						finger_id = touch_old[port].report[i].id;
						// Finger released from screen
						TouchEvent ev;
						ev.type = FINGERUP;
						ev.tfinger.timestamp = touch[port].timeStamp / 1000;
						ev.tfinger.touchId = port;
						ev.tfinger.fingerId = finger_id;
						ev.tfinger.x = x;
						ev.tfinger.y = y;
						psp2ProcessTouchEvent(&ev);
					}
				}
			}
		}
	}
}

static void psp2ConvertTouchXYToSDLXY(float *sdl_x, float *sdl_y, int vita_x, int vita_y, int port) {
	float x = 0;
	float y = 0;
	if (port == SCE_TOUCH_PORT_FRONT) {
		x = (vita_x - panelinfo[port].minDispX) / dispWidth[port];
		y = (vita_y - panelinfo[port].minDispY) / dispHeight[port];
	} else {
		x = (vita_x - panelinfo[port].minAaX) / aAWidth[port];
		y = (vita_y - panelinfo[port].minAaY) / aAHeight[port];				
	}
	if (x < 0.0) {
		x = 0.0;
	} else if (x > 1.0) {
		x = 1.0;
	}
	if (y < 0.0) {
		y = 0.0;
	} else if (y > 1.0) {
		y = 1.0;
	}
	*sdl_x = x;
	*sdl_y = y;
}

static void psp2ProcessTouchEvent(TouchEvent *event) {
	// Supported touch gestures:
	// left mouse click: single finger short tap
	// right mouse click: second finger short tap while first finger is still down
	// pointer motion: single finger drag
	// drag and drop: dual finger drag
	if (event->type == FINGERDOWN || event->type == FINGERUP || event->type == FINGERMOTION) {
		// front (0) or back (1) panel
		int port = event->tfinger.touchId;
		if (port >= 0 && port < SCE_TOUCH_PORT_MAX_NUM) {
			switch (event->type) {
			case FINGERDOWN:
				psp2ProcessFingerDown(event);
				break;
			case FINGERUP:
				psp2ProcessFingerUp(event);
				break;
			case FINGERMOTION:
				psp2ProcessFingerMotion(event);
				break;
			}
		}
	}
}

static void psp2ProcessFingerDown(TouchEvent *event) {
	// front (0) or back (1) panel
	int port = event->tfinger.touchId;
	// id (for multitouch)
	int id = event->tfinger.fingerId;

	// make sure each finger is not reported down multiple times
	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (_finger[port][i].id == id) {
			_finger[port][i].id = -1;
		}
	}

	// we need the timestamps to decide later if the user performed a short tap (click)
	// or a long tap (drag)
	// we also need the last coordinates for each finger to keep track of dragging
	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (_finger[port][i].id == -1) {
			_finger[port][i].id = id;
			_finger[port][i].timeLastDown = event->tfinger.timestamp;
			_finger[port][i].lastDownX = event->tfinger.x;
			_finger[port][i].lastDownY = event->tfinger.y;
			break;
		}
	}
}

static void psp2ProcessFingerUp(TouchEvent *event) {
	// front (0) or back (1) panel
	int port = event->tfinger.touchId;
	// id (for multitouch)
	int id = event->tfinger.fingerId;

	// find out how many fingers were down before this event
	int numFingersDown = 0;
	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (_finger[port][i].id >= 0) {
			numFingersDown++;
		}
	}

	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (_finger[port][i].id == id) {
			_finger[port][i].id = -1;
			if (!_multiFingerDragging[port]) {
				if ((event->tfinger.timestamp - _finger[port][i].timeLastDown) <= MAX_TAP_TIME) {
					// short (<MAX_TAP_TIME ms) tap is interpreted as right/left mouse click depending on # fingers already down
					// but only if the finger hasn't moved since it was pressed down by more than MAX_TAP_MOTION_DISTANCE pixels
					float xrel = ((event->tfinger.x * 960.0) - (_finger[port][i].lastDownX * 960.0));
					float yrel = ((event->tfinger.y * 544.0) - (_finger[port][i].lastDownY * 544.0));
					float maxRSquared = (float) (MAX_TAP_MOTION_DISTANCE * MAX_TAP_MOTION_DISTANCE);
					if ((xrel * xrel + yrel * yrel) < maxRSquared) {
						if (numFingersDown == 2 || numFingersDown == 1) {
							Uint8 simulatedButton = 0;
							if (numFingersDown == 2) {
								simulatedButton = SDL_BUTTON_RIGHT;
								// need to raise the button later
								_simulatedClickStartTime[port][1] = event->tfinger.timestamp;
							} else if (numFingersDown == 1) {
								if (port == 0 && !indirect_front_touch) {
									int x, y;
									psp2ConvertTouchXYToGameXY(event->tfinger.x, event->tfinger.y, &x, &y);
									*touchmouse_x = x;
									*touchmouse_y = y;
								}
								simulatedButton = SDL_BUTTON_LEFT;
								// need to raise the button later
								_simulatedClickStartTime[port][0] = event->tfinger.timestamp;
							}
							touchmouse_button[simulatedButton] = 1;
						}
					}
				}
			} else if (numFingersDown == 1) {
				// when dragging, and the last finger is lifted, the drag is over
				Uint8 simulatedButton = 0;
				if (_multiFingerDragging[port] == DRAG_THREE_FINGER) {
					simulatedButton = SDL_BUTTON_RIGHT;
				}
				else {
					simulatedButton = SDL_BUTTON_LEFT;
				}
				touchmouse_button[simulatedButton] = 0;
				_multiFingerDragging[port] = DRAG_NONE;
			}
		}
	}
}

static void psp2ProcessFingerMotion(TouchEvent *event) {
	// front (0) or back (1) panel
	int port = event->tfinger.touchId;
	// id (for multitouch)
	int id = event->tfinger.fingerId;

	// find out how many fingers were down before this event
	int numFingersDown = 0;
	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (_finger[port][i].id >= 0) {
			numFingersDown++;
		}
	}

	if (numFingersDown >= 1) {
		// If we are starting a multi-finger drag, start holding down the mouse button
		if (numFingersDown >= 2) {
			if (!_multiFingerDragging[port]) {
				// only start a multi-finger drag if at least two fingers have been down long enough
				int numFingersDownLong = 0;
				for (int i = 0; i < MAX_NUM_FINGERS; i++) {
					if (_finger[port][i].id >= 0) {
						if (event->tfinger.timestamp - _finger[port][i].timeLastDown > MAX_TAP_TIME) {
							numFingersDownLong++;
						}
					}
				}
				if (numFingersDownLong >= 2) {
					Uint8 simulatedButton = 0;
					if (numFingersDownLong == 2) {
						simulatedButton = SDL_BUTTON_LEFT;
						_multiFingerDragging[port] = DRAG_TWO_FINGER;
					} else {
						simulatedButton = SDL_BUTTON_RIGHT;
						_multiFingerDragging[port] = DRAG_THREE_FINGER;						
					}

					touchmouse_button[simulatedButton] = 1;
				}
			}
		}

		//check if this is the "oldest" finger down (or the only finger down), otherwise it will not affect mouse motion
		bool updatePointer = true;
		if (numFingersDown > 1) {
			for (int i = 0; i < MAX_NUM_FINGERS; i++) {
				if (_finger[port][i].id == id) {
					for (int j = 0; j < MAX_NUM_FINGERS; j++) {
						if (_finger[port][j].id >= 0 && (i != j) ) {
							if (_finger[port][j].timeLastDown < _finger[port][i].timeLastDown) {
								updatePointer = false;
							}
						}
					}
				}
			}
		}
		if (updatePointer) {
			if (port == 0 && !indirect_front_touch) {
				int x, y;
				psp2ConvertTouchXYToGameXY(event->tfinger.x, event->tfinger.y, &x, &y);
				*touchmouse_x = x;
				*touchmouse_y = y;
			} else {
				// for relative mode, use the pointer speed setting
				int dx = event->tfinger.dx * 960.0 * 256 * pointer_speed_factor / scale_x;
				int dy = event->tfinger.dy * 544.0 * 256 * pointer_speed_factor / scale_y;
				hires_dx += dx;
				hires_dy += dy;
				*touchmouse_x += hires_dx / 256;
				*touchmouse_y += hires_dy / 256;
				hires_dx %= 256;
				hires_dy %= 256;
			}
		}
	}
}


static void psp2ConvertTouchXYToGameXY(float touchX, float touchY, int *gameX, int *gameY) {
	//map to display
	float x = touchX * 960.0;
	float y = touchY * 544.0;
	*gameX = (x - offset_x) / scale_x;
	*gameY = (y - offset_y) / scale_y;
}

static void psp2FinishSimulatedMouseClicks(int port, SceUInt64 currentTime) {
	for (int i = 0; i < 2; i++) {
		if (_simulatedClickStartTime[port][i] != 0) {
			if (currentTime - _simulatedClickStartTime[port][i] >= SIMULATED_CLICK_DURATION) {
				int simulatedButton;
				if (i == 0) {
					simulatedButton = SDL_BUTTON_LEFT;
				} else {
					simulatedButton = SDL_BUTTON_RIGHT;
				}
				touchmouse_button[simulatedButton] = 0;

				_simulatedClickStartTime[port][i] = 0;
			}
		}
	}
}

void ImGui_ImplVitaGL_PrivateSetIndirectFrontTouch(bool enable) {
	indirect_front_touch = enable;
}

void ImGui_ImplVitaGL_PrivateSetRearTouch(bool enable) {
	rear_touch = enable;
}

