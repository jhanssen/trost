#pragma once

#include "Graphics.h"

namespace trost {

class Renderer;
class Messages;
class Input;

class App
{
public:
    static bool initialize();
    static void cleanup();

    static App* instance();

    void iterateLoop();

private:
    App() = default;

    const Graphics* mGraphics = nullptr;

    UBYTE mInputSig, mRenderSig, mMessageSig;
    Input* mInput = nullptr;
    Renderer* mRenderer = nullptr;
    Messages* mMessages = nullptr;

    static App* sInstance;
};

} // namespace trost
