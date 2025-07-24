#pragma once

#include "Graphics.h"
#include "Rect.h"

namespace trost {

struct Input
{
    char buffer[128];
    int length;
};

struct InputOptions
{
    const Graphics* graphics;
    Rect rect;
    const char* message = nullptr;
    int messageLength = 0;
};

bool acquireInput(const InputOptions& options, Input* input);

}
