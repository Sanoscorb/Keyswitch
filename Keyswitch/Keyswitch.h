#pragma once

#include "resource.h"

#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 160

#define CTRL_PADDING 7
#define CTRL_HEIGHT 24
#define CTRL_GAP CTRL_PADDING

#define ESC_NO_ACT 0
#define ESC_EXIT 1
#define ESC_MINIMIZE 2

typedef struct tagKS_USERDATA
{
    BOOL isToggle;
    UINT uEscAction;
    UINT uLastCurKl;
    UINT uLastAltKl;
} KS_USERDATA;