//Modfiied by: Douglas Rank
//Date: Spring 2022

//author: Gordon Griesel
//date: Feb 2018
//line-line intersection
//modified to a game intersection

//Swipe the mouse over a line (while holding left mouse button) and 
//indicate the interesection
//Draw an indication when and where the lines intersect.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/Xdbe.h>

//#define DEBUG_ON

typedef float Flt;
typedef Flt Vec[2];
#define PI 3.14159265358979
#define SWAP(x,y) (x)^=(y);(y)^=(x);(x)^=(y)
#define rnd() (Flt)rand() / (Flt)RAND_MAX
const int maxTrails = 20;
//

struct Point {
	Flt pos[2];
	Flt vel[2];
	Point() { }
};

struct Trail {
	Flt pos[2];
	float color[3];
	struct timespec timeStart;
	double age;
	Trail() { }
};

class Global {
public:
	int xres, yres;
	Flt fxres, fyres;
	int lbuttonDown;
	Point p[4];
	Point exp[32];
	Trail trail[maxTrails];
	int ntrails;
	int overlap;
	time_t timer;
	int score;
	int timeCounter;
	char scoreText[100];
	char timerText[100];
	bool waitForNewLine;
	bool explosionDrawn;
	bool drawingMainLine;
	bool drawingMouseTrail;
	bool endGame;
	bool gameStart;
	Global() {
		xres = 640, yres = 640;
		fxres=(Flt)xres, fyres=(Flt)yres;
		lbuttonDown = 0;
		#ifdef DEBUG_ON
		srand(7);
		#else
		srand((unsigned)time(NULL));
		#endif
		timeCounter = 0;
		overlap = 0;
		timer = time(NULL);
		score = 0;
		waitForNewLine = 0;
		explosionDrawn = 0;
		drawingMainLine = 1;
		drawingMouseTrail = 0;
		endGame = 0;
		gameStart = 0;
		ntrails = 0;
	}
} g;

class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GC gc;
	XdbeBackBuffer backBuffer;
	XdbeSwapInfo swapInfo;
public:
	~X11_wrapper() {
		//Deallocate back buffer
		if (!XdbeDeallocateBackBufferName(dpy, backBuffer)) {
			fprintf(stderr,"Error : unable to deallocate back buffer.\n");
		}
		XFreeGC(dpy, gc);
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
	X11_wrapper() {
		int major, minor;
		XSetWindowAttributes attributes;
		XdbeBackBufferAttributes *backAttr;
		dpy = XOpenDisplay(NULL);
	    //List of events we want to handle
		attributes.event_mask = ExposureMask | StructureNotifyMask |
			PointerMotionMask | ButtonPressMask |
			ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
		//Various window attributes
		attributes.backing_store = Always;
		attributes.save_under = True;
		attributes.override_redirect = False;
		attributes.background_pixel = 0x00000000;
		//Get default root window
		Window root = DefaultRootWindow(dpy);
		//Create a window
		win = XCreateWindow(dpy, root, 0, 0, g.xres, g.yres, 0,
		    CopyFromParent, InputOutput, CopyFromParent,
		    CWBackingStore | CWOverrideRedirect | CWEventMask |
			CWSaveUnder | CWBackPixel, &attributes);
		//Create gc
		gc = XCreateGC(dpy, win, 0, NULL);
		//Get DBE version
		if (!XdbeQueryExtension(dpy, &major, &minor)) {
			fprintf(stderr, "Error : unable to fetch Xdbe Version.\n");
			XFreeGC(dpy, gc);
			XDestroyWindow(dpy, win);
			XCloseDisplay(dpy);
		}
		printf("Xdbe version %d.%d\n",major,minor);
		//Get back buffer and attributes (used for swapping)
		backBuffer = XdbeAllocateBackBufferName(dpy, win, XdbeUndefined);
		backAttr = XdbeGetBackBufferAttributes(dpy, backBuffer);
	    swapInfo.swap_window = backAttr->window;
	    swapInfo.swap_action = XdbeUndefined;
		XFree(backAttr);
		//Map and raise window
		set_window_title();
		XMapWindow(dpy, win);
		XRaiseWindow(dpy, win);
	}
	void set_window_title() {
		char ts[256];
		sprintf(ts, "line/line intersection");
		XStoreName(dpy, win, ts);
	}
	void check_resize(XEvent *e) {
		//ConfigureNotify is sent when the window is resized.
		if (e->type != ConfigureNotify)
			return;
		XConfigureEvent xce = e->xconfigure;
		g.xres = xce.width;
		g.yres = xce.height;
		g.fxres = (Flt)g.xres;
		g.fyres = (Flt)g.yres;
		//init();
		set_window_title();
		//g.circle.pos[0] = 0.0;
		//g.circle.pos[1] = 0.0;
	}
	void clear_screen()	{
		XSetForeground(dpy, gc, 0x00050505);
		XFillRectangle(dpy, backBuffer, gc, 0, 0, g.xres, g.yres);
	}
	void set_color_3i(int r, int g, int b) {
		unsigned long cref = 0L;
		cref += r;
		cref <<= 8;
		cref += g;
		cref <<= 8;
		cref += b;
		XSetForeground(dpy, gc, cref);
	}
	bool getXPending() {
		return XPending(dpy);
	}
	XEvent getXNextEvent() {
		XEvent e;
		XNextEvent(dpy, &e);
		return e;
	}
	void drawPoint(int x, int y) {
		XDrawPoint(dpy, backBuffer, gc, x, y);
	}
	void drawText(int x, int y, char *text) {
		XDrawString(dpy, backBuffer, gc, x, y, text, strlen(text));
	}
	void swapBuffers() { XdbeSwapBuffers(dpy, &swapInfo, 1); }
} x11;

//prototypes
double timeDiff(struct timespec *start, struct timespec *end);
void timeCopy(struct timespec *dest, struct timespec *source);
void init();
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void drawExplosion(Point p1, Point p2);
bool checkOverlappingLines();
void resetpoints();
void physics();
void render();

int main()
{
	init();
	int done = 0;
	while (!done) {
		//Handle all events in queue...
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			x11.check_resize(&e);
			check_mouse(&e);
			done = check_keys(&e);
		}
		//Process physics and rendering every frame
		physics();
		render();
		x11.swapBuffers();
		usleep(512);
	}
	return 0;
}

void init()
{

}

void check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;
	//
	if (e->type != ButtonRelease &&
			e->type != ButtonPress &&
			e->type != MotionNotify)
		return;
	if (e->type == ButtonRelease) {
		g.lbuttonDown = 0;
		return;
	}
	if (e->type == ButtonPress) {
		//Log("ButtonPress %i %i\n", e->xbutton.x, e->xbutton.y);
		if (e->xbutton.button==1) {
			//left button is down
			g.lbuttonDown = 1;
		}
		if (e->xbutton.button==3) {
			//Adding a new control point...
		}
	}
	if (e->type == MotionNotify) {
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
		//mouse moved
		if (g.lbuttonDown) {
			//grab the x,y endpoints for the start of the line
			//endpoints of the mouse line are array index 2 and 3
			g.p[2].pos[0] = savex;
			g.p[2].pos[1] = savey;
			g.p[3].pos[0] = e->xbutton.x;
			g.p[3].pos[1] = e->xbutton.y;
			//save trail endpoints
				if (g.ntrails == maxTrails)	{	
					//remove first point and shift array to the left
					for(int i = 0; i < g.ntrails; i++) {
						g.trail[i] = g.trail[i+1];
						g.ntrails--;
					}
				} else {
					g.trail[g.ntrails].pos[0] = e->xbutton.x;
					g.trail[g.ntrails].pos[1] = e->xbutton.y;
					clock_gettime(CLOCK_REALTIME, &g.trail[g.ntrails].timeStart);
					g.trail[g.ntrails].color[0] = 1.0;
					g.trail[g.ntrails].color[1] = 1.0;
					g.trail[g.ntrails].color[2] = 0.0;
					g.ntrails++;
				}
			}

			savex = e->xbutton.x;
			savey = e->xbutton.y;
		}
	}
}

int check_keys(XEvent *e)
{
	static int shift = 0;
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyRelease) {
		if (key == XK_Shift_L || key == XK_Shift_R) {
			shift = 0;
			return 0;
		}
	}
	if (key == XK_Shift_L || key == XK_Shift_R) {
		shift = 1;
		return 0;
	}
	(void)shift;
	//a key was pressed
	switch (key) {
		case XK_space: {
			g.gameStart = 1;
			break;
		}
		case XK_s: break;
		case XK_Up: break;
		case XK_Down: break;
		case XK_Left: break;
		case XK_Right: break;
		case XK_Escape:
			return 1;
	}
	return 0;
}

void myBresenhamLine(int x0, int y0, int x1, int y1)
{
	//Bresenham line algorithm, integers only.
	int steep = (abs(y1-y0) > abs(x1-x0));
	if (steep) {
		SWAP(x0, y0);
		SWAP(x1, y1);
	}
	if (x0 > x1) {
		SWAP(x0, x1);
		SWAP(y0, y1);
	}
	int ystep = (y1>y0) ? 1 : -1;
	int yDiff = abs(y1-y0);
	int xDiff = x1 - x0;
	int err=xDiff>>1;
	int x, y=y0;
	for	(x=x0; x<=x1; x++) {
		if (steep)
			x11.drawPoint(y, x);
		else
			x11.drawPoint(x, y);
		err -= yDiff;
		if (err <= 0) {
			y += ystep;
			err += xDiff;
		}
	}
}

void show_anchor(int x, int y, int size)
{
	//draw a small rectangle...
	int i,j;
	for (i=-size; i<=size; i++) {
		for (j=-size; j<=size; j++) {
			x11.drawPoint(x+j, y+i);
		}
	}
}
void drawExplosion(Point p1, Point p2)
{
	int length = 30;
	int angle = 0;
	Flt rad;
	//get midpoint of intersecting line drawn by mouse
	//save midpoint for drawAnchor at intersection point
	g.exp[0].pos[0] = (p1.pos[0] + p2.pos[0])/2;
	g.exp[0].pos[1] = (p1.pos[1] + p2.pos[1])/2;
	//draw lines extending from anchor to simulate explosion
	//start from 0 degrees, and rotate Y point 15 degrees for each line
	for (int i = 1; i < 32; i ++) {
		rad = angle*PI/180;
		g.exp[i].pos[0] = g.exp[0].pos[0] + (length * cos(rad));
		g.exp[i].pos[1] = g.exp[0].pos[1] + (length * sin(rad));
		angle += 15;
	}
}

void show_explosion(Point p0, Point p1)
{
	//flags are set and reset in physics()
	x11.set_color_3i(204,85,0);
	myBresenhamLine( p0.pos[0], p0.pos[1], p1.pos[0], p1.pos[1]);
}

void show_line(Point p0, Point p1)
{
	//default line color
	x11.set_color_3i(100,255,100);
	//flag is set and reset in render()
	if (g.drawingMainLine) 
		x11.set_color_3i(100,255,100);
	//flags are set and reset in physics()
	if (g.overlap || g.explosionDrawn) 
		x11.set_color_3i(255,0,0);
	myBresenhamLine( p0.pos[0], p0.pos[1], p1.pos[0], p1.pos[1]);
}

void show_line(Trail t0, Trail t1)
{
	//get current time
	struct timespec current;
	clock_gettime(CLOCK_REALTIME, &current);
	//
	//get difference in time
	t0.age = abs(timeDiff(&current, &t0.timeStart));
	//
	//intialize rgb intensity from values in MotionNotify checkkeys
	float rgb[3];
	for (int i = 0; i < 2; i++) {
		rgb[i] = t0.color[i];
	}
	//fade color based on diff in time
	float intensity = 1 - t0.age;
	if (intensity < 0)
		intensity = 0;
	for(int i = 0; i < 2; i++) {
		rgb[i] = rgb[i]*(intensity);
	}
	//
	x11.set_color_3i((int)(rgb[0]*255.0),(int)(rgb[1]*255.0),(int)(rgb[2]*255.0));
	myBresenhamLine( t0.pos[0], t0.pos[1], t1.pos[0], t1.pos[1]);
}

Flt dotProduct(Vec v0, Vec v1) {
	return (v0[0]*v1[0] + v0[1]*v1[1]);
}

bool checkLineEndPoints(int a, int b, int c, int d)
{
	//make some vectors
	//get a perpendicular vector
	//are the dot products positive or negative?
	//different dot products means line end-points are on different sides
	//of the other line segment. If both line segments meet this condition,
	//the lines intersect.
	Vec v0 = {g.p[a].pos[0]-g.p[d].pos[0],g.p[a].pos[1]-g.p[d].pos[1]};
	Vec v1 = {g.p[b].pos[0]-g.p[d].pos[0],g.p[b].pos[1]-g.p[d].pos[1]};
	Vec v2 = {g.p[c].pos[0]-g.p[d].pos[0],g.p[c].pos[1]-g.p[d].pos[1]};
	Vec perp = { v0[1], -v0[0] };
	Flt dot1 = dotProduct(perp, v1);
	Flt dot2 = dotProduct(perp, v2);
	int sign1 = (dot1 < 0.0) ? -1 : 1;
	int sign2 = (dot2 < 0.0) ? -1 : 1;
	return (sign1 != sign2);
}
bool checkOverlappingLines()
{
	int over1 = checkLineEndPoints(1, 2, 3, 0);
	int over2 = checkLineEndPoints(2, 0, 1, 3);
	return (over1 && over2);
}

void physics()
{
	if (g.gameStart) {
		time_t currentSecs = time(NULL);
		time_t elapsedTime;
		if (currentSecs != g.timer) {
			//define endpoints for main line segment
			for (int i=0; i<2; i++) {
				g.p[i].pos[0] = rand() % g.xres;
				g.p[i].pos[1] = rand() % g.yres;
			}
			elapsedTime = currentSecs - g.timer;
			if (elapsedTime >= 1) {
				g.timeCounter++;
				//flags to reset game state
				g.waitForNewLine = 0;
				g.explosionDrawn = 0;
			}
			g.timer = currentSecs;
		}
		
		//check for lines overlapping...
		g.overlap = checkOverlappingLines();
		if (g.overlap && !g.waitForNewLine) {
			g.score++;
			drawExplosion(g.p[2], g.p[3]);	
			//flag explosion to avoid clearscreen call in render
			g.explosionDrawn = 1;
			//use flag to pause scoring until next line is drawn
			g.waitForNewLine = 1;
		}
		
	}
	if (g.score == 10 || g.timeCounter > 11)
		g.endGame = 1;
}

void render()
{
	x11.clear_screen();
	if (!g.gameStart) {
		//Gamestart is intialized to 1 to display on startup
		//Once player hits spacebar in the checkkeys function, the flag will set to false 
		//and move to the next game state
		x11.set_color_3i(255,255,  0);
		x11.drawText(g.xres/2-100, g.yres/2 - 30, (char*)" Drag mouse across lines to gain points");
		x11.drawText(g.xres/2-100, g.yres/2 - 15, (char*)"Score 10 points within time limit to win");
		x11.drawText(g.xres/2-100, g.yres/2,      (char*)"       <Press spacebar to start>");
	} else if (!g.endGame) {
		//Endgame flag is initialized to false, but will trigger in the physics funtions 
		//once the player reaches a certain score or the timer runs out
		//g.drawingMainLine and g.drawingMouseTrail flags are used to preserve color of main line segment 
		//and mouse line segment in the showline function
		g.drawingMainLine = 1;
		show_line(g.p[0], g.p[1]);
		g.drawingMainLine = 0;

		//show end-points
		x11.set_color_3i(255,255,255); show_anchor(g.p[0].pos[0],g.p[0].pos[1],2);
		x11.set_color_3i(255,  0,  0); show_anchor(g.p[1].pos[0],g.p[1].pos[1],2);
		
		//show mouse trail
		g.drawingMouseTrail = 1;
		if (g.ntrails > 1) {
			for(int i = 0; i < g.ntrails-1; i++) {
				show_line(g.trail[i], g.trail[i+1]);
			}	
		}
		g.drawingMouseTrail = 0;

		//draw explosion
		if (g.explosionDrawn && g.waitForNewLine) {
			x11.set_color_3i(255,0,0);
			show_anchor(g.exp[0].pos[0],g.exp[0].pos[1],2);
			for (int i = 1; i < 32; i ++)
				show_explosion(g.exp[0], g.exp[i]);
		}
	} else {
		//Endgame flag has triggered and the endgame screen will display until player hits escape
		x11.set_color_3i(255,255,  0);
		if (g.score == 10)
			x11.drawText(g.xres/2, g.yres/2, (char*)"You Win!");
		else
			x11.drawText(g.xres/2, g.yres/2, (char*)"Game over.");
	}
	//Constantly rendered at top of screen displaying current timer and player score
	sprintf(g.scoreText, "Score: %i", g.score);
	x11.set_color_3i(255,255,  0);
	x11.drawText(10, 20, g.scoreText);
	sprintf(g.timerText, "Timer: %i", (11-g.timeCounter) > 0 ? 11-g.timeCounter : 0);
	x11.set_color_3i(255,255,  0);
	x11.drawText(g.xres - 70, 20, g.timerText);

}
