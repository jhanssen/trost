#pragma once

#include "Graphics.h"
#include "Function.h"
#include "Vector.h"
#include <clib/intuition_protos.h>
#include <devices/inputevent.h>

namespace trost {

class Renderer
{
public:
    static bool initialize(const Graphics* graphics);
    static Renderer* instance();

    void render();
    void cleanup();

    struct Context
    {
        RastPort* rastPort;
    };
    ULONG addRenderer(trost::Function<void(Context*)>&& handler);
    void removeRenderer(ULONG id);

private:
    Renderer() = default;

    bool initialize();

private:
    Graphics mGraphics;
    ScreenBuffer* mBuffers[2] = { nullptr, nullptr };
    RastPort mRastPorts[2];
    MsgPort* mDbufPort = nullptr;
    MsgPort* mUserPort = nullptr;
    MsgPort* mInputPort = nullptr;
    IORequest* mInputRequest = nullptr;
    InputEvent mGameEvent;

    enum class RedrawStatus { Redraw, Swapin, Wait };
    RedrawStatus mStatus[2] = { RedrawStatus::Redraw, RedrawStatus::Redraw };

    UWORD mDraw = 0;
    UWORD mSwap = 0;

    struct Entry
    {
        ULONG id;
        trost::Function<void(Context*)> handler;
    };
    Vector<Entry> mHandlers;
    ULONG mNextId = 0;

    static Renderer* sInstance;
};

} // namespace trost
