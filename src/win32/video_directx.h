#pragma once

// Our video front-end
#include "video.h"

// Initialize the directx back-end
BOOL dx9vid_init();
BOOL dx9vid_render();
BOOL dx9vid_resize(int);
BOOL dx9vid_update();
void dx9vid_shutdown();
