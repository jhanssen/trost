#pragma once

#include "Graphics.h"
#include "Function.h"
#include "Rect.h"
#include <clib/intuition_protos.h>
#include <devices/inputevent.h>

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

    void processInput();

    UBYTE sigBit() const;

private:
    Input() = default;

    void initialize();

private:
    const Graphics* mGraphics = nullptr;

    MsgPort* mInputPort = nullptr;
    IORequest* mInputRequest = nullptr;
    InputEvent mGameEvent;

    static Input* sInstance;
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
