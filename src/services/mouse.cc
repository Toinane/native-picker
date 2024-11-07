#include "services.h"

MMSignedPoint GetMousePos() {
#if defined(IS_MACOSX)
	CGEventRef event = CGEventCreate(NULL);
	CGPoint point = CGEventGetLocation(event);
	CFRelease(event);

	return MMSignedPointFromCGPoint(point);
#elif defined(USE_X11)
	int x, y;
	Window garb1, garb2;
	int garb_x, garb_y;
	unsigned int more_garbage;

	Display *display = XGetMainDisplay();
	XQueryPointer(display, XDefaultRootWindow(display), &garb1, &garb2,
	              &x, &y, &garb_x, &garb_y, &more_garbage);

	return MMSignedPointMake(x, y);
#elif defined(IS_WINDOWS)
	POINT point;
	GetCursorPos(&point);

	MMSignedPoint p;
  p.x = (int32_t)point.x;
  p.y = (int32_t)point.y;

  return p;
#endif
}