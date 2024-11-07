#ifndef SERVICES_H
#define SERVICES_H

#include "../types.h"

#if defined(IS_MACOSX)
  #include <ApplicationServices/ApplicationServices.h>
#elif defined(USE_X11)
  #include <X11/Xlib.h>
  #include <X11/extensions/XTest.h>
  #include <stdlib.h>
  #include "xdisplay.h"
#elif defined(IS_WINDOWS)
  #include <windows.h>
#endif


MMSignedPoint GetMousePos();

#endif