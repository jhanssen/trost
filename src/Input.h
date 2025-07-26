#pragma once

#include "Graphics.h"
#include "Function.h"
#include "Rect.h"

namespace trost {

class Input
{
public:
    static void initialize(const Graphics* graphics);
    static void cleanup();

    static Input* instance();

    ULONG addKeyboard(Function<void(IntuiMessage*)>&& handler);
    ULONG addMouse(Function<void(IntuiMessage*)>&& handler);
    void removeHandler(ULONG id);

private:
    Input() = default;

    void initialize();
};

struct KeyInput
{
    char buffer[128];
    int length;
};

struct KeyInputOptions
{
    const Graphics* graphics;
    Rect rect;
    const char* message = nullptr;
    int messageLength = 0;
};

bool acquireKeyInput(const KeyInputOptions& options, KeyInput* input);

}
