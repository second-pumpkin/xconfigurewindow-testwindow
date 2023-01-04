#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "config.h"

// node for a linked list containing all of the buttons
typedef struct Button Button;
struct Button {
	Window window; 
	char* text;
	int x, y, w, h;
	Button* next;
	void (*function)(int);
	int arg; // arg represents any arbitrary 32 bits of data to be passed into function()
};

// struct for information about the app
struct {
	Button* firstButton;
	int numButtons;
	int x, y, w, h;
	Window window;
	Button* currentlyPressedButton;
	bool fixedButtonPositions;
	bool knownPosition;
	char gay;
	GC gc;
	XFontStruct* font;
} app;

// functions: event functions, button press functions, and other functions
void onButtonRelease(XEvent* e);
void onButtonPress(XEvent* e);
void onMapNotify(XEvent* e);
void onConfigureNotify(XEvent* e);

void changeGeometry(int arg);
void toggleFixedButtonPositions(int arg);
void toggleGayMode(int arg);

void drawFrame();
void drawButton(Button* button, bool pressed);
void drawOctagon(Drawable drawable, GC gc, int x, int y, int w, int h, int cornerSize);
void createButton(int w, int h, char* text, void (*function)(int), int arg);
void repositionButtons();
Button* getButton(Window window);
void getRealAppPosition();
void addButtons();

// display varbale (global varable😱😱)
Display* dpy;

int main() {
	// open a connection to the display
	if (!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "Error: could not open display\n");
		return 1;
	}

	// set up graphics stuff
	XVisualInfo visualInfo;
	if (!XMatchVisualInfo(dpy, DefaultScreen(dpy), 24, TrueColor, &visualInfo)) {
		fprintf(stderr, "Error starting: could not find an appropriate visual info (\?\?)\n");
		return 1;
	}
	app.font = XLoadQueryFont(dpy, FONT_STRING);
	
	// create the window and its buttons and map them
	XSetWindowAttributes wattr;
	wattr.background_pixel = APP_BG_COLOR;
	wattr.event_mask = StructureNotifyMask | ButtonPressMask | ButtonReleaseMask;
	wattr.colormap = XCreateColormap(dpy, DefaultRootWindow(dpy), visualInfo.visual, AllocNone);
	unsigned long valueMask = CWBackPixel | CWEventMask | CWColormap;

	app.window = XCreateWindow(dpy, DefaultRootWindow(dpy), 100, 100, 800, 600, 0, visualInfo.depth, InputOutput, visualInfo.visual, valueMask, &wattr);
	addButtons();
	XMapWindow(dpy, app.window);
	
	// graphics stuff part two
	XGCValues gcValues;
	gcValues.font = app.font->fid;
	unsigned long gcValueMask = GCForeground | GCFont;
	app.gc = XCreateGC(dpy, app.window, gcValueMask, &gcValues);
	
	// seed the random number generator for gay mode
	srand(729);

	// main event loop
	XEvent e;
	while (1) {
		XNextEvent(dpy, &e);

		switch (e.type) {
			case ConfigureNotify:
				onConfigureNotify(&e);
				break;
			case ButtonRelease:
				onButtonRelease(&e);
				break;
			case ButtonPress:
				onButtonPress(&e);
				break;
			case MapNotify:
				onMapNotify(&e);
				break;
		}
	}
}

void onButtonRelease(XEvent* e) {
	// if a button is currently pressed, unpress it
	if (app.currentlyPressedButton) drawButton(app.currentlyPressedButton, false);
	
	Button* button;

	// do what the button does... if you can solve my riddles three
	if (!e->xbutton.subwindow || !(button = getButton(e->xbutton.subwindow)) || app.currentlyPressedButton != button) return;
	button->function(button->arg);
}

void onButtonPress(XEvent* e) {
	Button* button;
	if (!e->xbutton.subwindow || !(button = getButton(e->xbutton.subwindow))) return;
	app.currentlyPressedButton = button;

	drawButton(button, true);
}

void onMapNotify(XEvent* e) {
	if (e->xmap.window != app.window) return;

	// draw the buttons
	drawFrame();
	
}

void onConfigureNotify(XEvent* e) {
	// if there are more pending ConfigureNotify events, only process the newest one
	while (XCheckTypedEvent(dpy, ConfigureNotify, e));
		
	XConfigureEvent* xconfigure = &e->xconfigure;
	
	// determine if the size has changed
	bool sizeChanged = false;
	if (app.w - xconfigure->width || app.h - xconfigure->height) sizeChanged = true;

	// update the geometry
	app.x = xconfigure->x;
	app.y = xconfigure->y;
	app.w = xconfigure->width;
	app.h = xconfigure->height;
	
	// if the configure event is synthetic, the coordinates in that event are relative to the root.
	// real configure events are usually followed by synthetic ones, but not always. when they aren't, things get pretty silly.
	// there is no way of knowing what the real position of the window is if a synthetic configure event never comes,
	// so when the position is necessary and the position is not known, a special function will be used (see getRealAppPosition())
	if (xconfigure->send_event)
		app.knownPosition = true;
	else
		app.knownPosition = false;

	// if the size changed, reposition and redraw the buttons
	if (sizeChanged) {
		if (!app.fixedButtonPositions) repositionButtons();
		drawFrame();
	}
}

void drawFrame() {
	// clear the window by drawing the background
	XSetForeground(dpy, app.gc, APP_BG_COLOR);
	if (!(app.gay & 1)) XFillRectangle(dpy, app.window, app.gc, 0, 0, app.w, app.h);
	else if (app.gay & 4) {
		// draw rainbow flag
		XSetForeground(dpy, app.gc, 0xE40303);
		XFillRectangle(dpy, app.window, app.gc, 0, 0, app.w, app.h / 6);
		XSetForeground(dpy, app.gc, 0xFF8C00);
		XFillRectangle(dpy, app.window, app.gc, 0, app.h / 6, app.w, app.h / 6);
		XSetForeground(dpy, app.gc, 0xFFED00);
		XFillRectangle(dpy, app.window, app.gc, 0, app.h / 6 * 2, app.w, app.h / 6);
		XSetForeground(dpy, app.gc, 0x008026);
		XFillRectangle(dpy, app.window, app.gc, 0, app.h / 6 * 3, app.w, app.h / 6);
		XSetForeground(dpy, app.gc, 0x24408E);
		XFillRectangle(dpy, app.window, app.gc, 0, app.h / 6 * 4, app.w, app.h / 6);
		XSetForeground(dpy, app.gc, 0x732982);
		XFillRectangle(dpy, app.window, app.gc, 0, app.h / 6 * 5, app.w, app.h / 6);
	} else {
		// draw french flag
		XSetForeground(dpy, app.gc, 0x002654);
		XFillRectangle(dpy, app.window, app.gc, 0, 0, app.w / 3, app.h);
		XSetForeground(dpy, app.gc, 0xFFFFFF);
		XFillRectangle(dpy, app.window, app.gc, app.w / 3, 0, app.w / 3, app.h);
		XSetForeground(dpy, app.gc, 0xED2939);
		XFillRectangle(dpy, app.window, app.gc, app.w / 3 * 2, 0, app.w / 3, app.h);
	}

	// loop through the buttons, drawing each one
	Button* button = app.firstButton;
	while (button) {
		drawButton(button, false);
		button = button->next;
	}
}

void drawButton(Button* button, bool pressed) {
	// draw the background colored sections first
	XSetForeground(dpy, app.gc, pressed ? B_BG_COLOR_PRESSED : B_BG_COLOR);

	// first draw a square with the corners cut off (octagon)
	drawOctagon(app.window, app.gc, button->x, button->y, button->w, button->h, B_CORNER_RADIUS);

	// draw the corner circles. angle units is degrees * 64 for some reason (i am going insane)
	XFillArc(dpy, app.window, app.gc, button->x, button->y, 2*B_CORNER_RADIUS, 2*B_CORNER_RADIUS, 90*64, 90*64);
	XFillArc(dpy, app.window, app.gc, 0+button->x + button->w - 2*B_CORNER_RADIUS, button->y, 2*B_CORNER_RADIUS, 2*B_CORNER_RADIUS, 0*64, 90*64);
	XFillArc(dpy, app.window, app.gc, button->x + button->w - 2*B_CORNER_RADIUS, button->y + button->h - 2*B_CORNER_RADIUS, 2*B_CORNER_RADIUS, 2*B_CORNER_RADIUS, 270*64, 90*64);
	XFillArc(dpy, app.window, app.gc, button->x, button->y + button->h - 2*B_CORNER_RADIUS, 2*B_CORNER_RADIUS, 2*B_CORNER_RADIUS, 180*64, 90*64);
	
	// text (TODO: figure out how to vertically center the text)
	XSetForeground(dpy, app.gc, B_TEXT_COLOR);
	XDrawString(dpy, app.window, app.gc, button->x + ((button->w - XTextWidth(app.font, button->text, strlen(button->text))) >> 1), button->y + (button->h >> 1), button->text, strlen(button->text));
}

void drawOctagon(Drawable drawable, GC gc, int x, int y, int w, int h, int cornerSize) {
	// array of points for the octagon, last point is the same as the first point to close the shape
	XPoint points[9];
	
	// x and y are contiguous shorts so I can do stupid pointer stuff to set them both at the same time
	#define SETPOS(P,X,Y) *((int*)(&P.x))=Y<<16|(short)X

	SETPOS(points[0], x + cornerSize, y);
	SETPOS(points[1], x + w - cornerSize, y);
	SETPOS(points[2], x + w, y + cornerSize);
	SETPOS(points[3], x + w, y + h - cornerSize);
	SETPOS(points[4], x + w - cornerSize, y + h);
	SETPOS(points[5], x + cornerSize, y + h);
	SETPOS(points[6], x, y + h - cornerSize);
	SETPOS(points[7], x, y + cornerSize);
	SETPOS(points[8], x + cornerSize, y);

	XFillPolygon(dpy, drawable, gc, points, 9, Convex, CoordModeOrigin);
}

void addButtons() {
	// macro for use as the arg parameter in changeGeometry():
	// the 16 most significant bits are a short representing the relative change
	// the 16 least significant bits are a bitfield representing which attribute(s) of geometry to change
	// bits: 1-X 2-Y 4-W 8-H
	#define CG(n,m) ((short)n<<16)|m

	// create all the little buttons :3
	createButton(45, 20, "X+", changeGeometry, CG(50, 1));
	createButton(45, 20, "X-", changeGeometry, CG(-50, 1));
	createButton(45, 20, "Y+", changeGeometry, CG(50, 2));
	createButton(45, 20, "Y-", changeGeometry, CG(-50, 2));
	createButton(45, 20, "W+", changeGeometry, CG(50, 4));
	createButton(45, 20, "W-", changeGeometry, CG(-50, 4));
	createButton(45, 20, "H+", changeGeometry, CG(50, 8));
	createButton(45, 20, "H-", changeGeometry, CG(-50, 8));
	createButton(45, 20, "YH+", changeGeometry, CG(50, 10));
	createButton(45, 20, "YH-", changeGeometry, CG(-50, 10));
	createButton(45, 20, "XW+", changeGeometry, CG(50, 5));
	createButton(45, 20, "XW-", changeGeometry, CG(-50, 5));
	createButton(45, 20, "All+", changeGeometry, CG(50, 15));
	createButton(45, 20, "All-", changeGeometry, CG(-50, 15));
	createButton(150, 20, "Toggle Static Buttons", toggleFixedButtonPositions, 0);
	createButton(100, 20, "Gay Mode", toggleGayMode, 0);
	
	// position and map them
	repositionButtons();
	XMapSubwindows(dpy, app.window);
}

void getRealAppPosition() {
	int nx, ny;
	Window child;
	if (XTranslateCoordinates(dpy, app.window, DefaultRootWindow(dpy), 0, 0, &nx, &ny, &child)) {
		app.x = nx;
		app.y = ny;
	} else {
		fprintf(stderr, "Warning: could not find real position :(\n");
	}
}

void createButton(int w, int h, char* text, void (*function)(int), int arg) {
	// put it in the linked list
	Button* button = malloc(sizeof(Button));
	button->next = app.firstButton;
	app.firstButton = button;
	
	// set the button
	app.numButtons++;
	button->w = w;
	button->h = h;
	button->text = text;
	button->function = function;
	button->arg = arg;

	// create the button input window
	button->window = XCreateWindow(dpy, app.window, 0, 0, w, h, 0, 0, InputOnly, CopyFromParent, 0, 0);
}

void repositionButtons() {
	// set the buttons' positions within the app to look nice :)
	// algorithms??? in MY program????? it's more likely than you think
	
	int sideLength = ceil(sqrt(app.numButtons));
	int stepX = app.w / (sideLength + 1);
	int stepY = app.h / (sideLength + 1);
	
	Button* button = app.firstButton;
	for (int y = 1; y <= sideLength; y++) {
		for (int x = 1; x <= sideLength; x++) {
			button->x = (int) stepX * x - (button->w >> 1);
			button->y = (int) stepY * y - (button->h >> 1);
			XMoveWindow(dpy, button->window, button->x, button->y);

			if (button->next) button = button->next;
			else return;
		}
	}
}

Button* getButton(Window window) {
	// loop through the buttons linked list to find the button that window is
	Button* button = app.firstButton;
	while(button) {
		if (button->window == window) return button;
		button = button->next;
	}
	return 0;
}

void changeGeometry(int arg) {
	// if the position of the application is not known, get it
	if (!app.knownPosition) getRealAppPosition();

	// bit stuff, see the definition of CG() in addButtons()
	short num = arg >> 16;
	
	int nw = app.w, nh = app.h;
	if (arg & 1)
		// x
		app.x += num;
	if (arg & 2)
		// y
		app.y += num;
	if (arg & 4)
		// w
		nw += num;
	if (arg & 8)
		// h
		nh += num;
	
	XMoveResizeWindow(dpy, app.window, app.x, app.y, nw, nh);
}

void toggleFixedButtonPositions(int arg) {
	app.fixedButtonPositions = !app.fixedButtonPositions;
}

void toggleGayMode(int arg) {
	// bits of app.gay:
	//   the least significant bit (1) determines if gay mode is enabled
	//   the next bit (2) determines if gay mode has been previously enabled
	//   the next bit (4) determines whether a rainbow flag (0) or french flag (1) should be displayed as a background

	// toggle gay mode
	app.gay ^= 1;
	
	// redraw the frame as normal if gay mode is off
	if (!(app.gay & 1)) {
		drawFrame();
		return;
	}
	
	// if gay mode has previously been activated, the background has a 50% chance to be french
	// otherwise, the background will always be a rainbow flag
	if (app.gay & 2) {
		// set background bit to either 0 or 1
		app.gay += (rand() % 2) << 2;
		
		// redraw the frame
		drawFrame();
	} else {
		// enable bits 2 and 4, marking gay mode as previously enabled and setting the background to the rainbow flag
		app.gay |= 6;

		// redraw the frame
		drawFrame();
	}
}
