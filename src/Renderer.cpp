#include "Renderer.h"
#include "Messages.h"
#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <cstdio>

using namespace trost;

Renderer* Renderer::sInstance = nullptr;

bool Renderer::initialize()
{
    if (sInstance) {
        return true;
    }

    sInstance = new Renderer();
    auto graphics = &sInstance->mGraphics;

    graphics->screen = OpenScreenTags(NULL,
                                     SA_Width, 320,
                                     SA_Height, 256,
                                     SA_Depth, 5,
                                     SA_Type, CUSTOMSCREEN,
                                     SA_Title, "Full Screen Program",
                                     SA_ShowTitle, FALSE,
                                     SA_Quiet, TRUE,
                                     TAG_DONE);
    if (!graphics->screen) {
        printf("Failed to open screen\n");
        return 1;
    }

    graphics->window = OpenWindowTags(NULL,
                                     WA_Left,        0,
                                     WA_Top,         0,
                                     WA_Width,       320,
                                     WA_Height,      256,
                                     WA_IDCMP,       IDCMP_RAWKEY,
                                     WA_Flags,       WFLG_SIMPLE_REFRESH |
                                     WFLG_BACKDROP |
                                     WFLG_BORDERLESS |
                                     WFLG_ACTIVATE,
                                     WA_CustomScreen,(ULONG)graphics->screen,
                                     TAG_DONE);
    if (!graphics->window) {
        printf("Failed to open window\n");
        CloseScreen(graphics->screen);
        return 1;
    }

    //sInstance->mUserPort = CreateMsgPort();
    //graphics->window->UserPort = sInstance->mUserPort;
    sInstance->mUserPort = graphics->window->UserPort;

    // Your drawing logic or other code here
    SetRGB4(&(graphics->screen->ViewPort), 0, 15, 0, 0);
    // set pen color to white
    SetRGB4(&(graphics->screen->ViewPort), 1, 15, 15, 15);

    sInstance->mBuffers[0] = AllocScreenBuffer(graphics->screen, nullptr, SB_SCREEN_BITMAP);
    sInstance->mBuffers[1] = AllocScreenBuffer(graphics->screen, nullptr, SB_COPY_BITMAP);
    sInstance->mBuffers[0]->sb_DBufInfo->dbi_UserData1 = reinterpret_cast<APTR>(0);
    sInstance->mBuffers[1]->sb_DBufInfo->dbi_UserData1 = reinterpret_cast<APTR>(1);

    InitRastPort(&sInstance->mRastPorts[0]);
    sInstance->mRastPorts[0].BitMap = sInstance->mBuffers[0]->sb_BitMap;

    InitRastPort(&sInstance->mRastPorts[1]);
    sInstance->mRastPorts[1].BitMap = sInstance->mBuffers[1]->sb_BitMap;

    sInstance->mDbufPort = CreateMsgPort();

    return true;
}

Renderer* Renderer::instance()
{
    return sInstance;
}

const Graphics* Renderer::graphics() const
{
    return &mGraphics;
}

bool Renderer::isWaiting() const
{
    return mStatus[mDraw] == RedrawStatus::Wait;
}

void Renderer::processDbuf()
{
    struct Message *dbmsg;
    while ((dbmsg = GetMsg(mDbufPort))) {
        ULONG buffer = reinterpret_cast<ULONG>(*(reinterpret_cast<APTR**>(dbmsg + 1)));
        mStatus[buffer ^ 1] = RedrawStatus::Redraw;
    }
}

UBYTE Renderer::sigBit() const
{
    return mDbufPort->mp_SigBit;
}

void Renderer::render()
{
    // if there are no handlers or if there's nothing to do, just wait for a refresh
    if (mHandlers.size() == 0 || mStatus[mDraw] == RedrawStatus::Wait) {
        WaitTOF();
        return;
    }

    if (mStatus[mDraw] == RedrawStatus::Redraw) {
        // draw into the buffer

        Context context{ &mRastPorts[mDraw] };

        SetAPen(context.rastPort, 0);
        RectFill(context.rastPort, 0, 0, mGraphics.screen->Width - 1, mGraphics.screen->Height - 1);
        SetAPen(context.rastPort, 1);
        SetBPen(context.rastPort, 0);

        const auto sz = mHandlers.size();
        for (std::size_t i = 0; i < sz; ++i) {
            auto& entry = mHandlers[i];
            entry.handler(&context);
        }

        mStatus[mDraw] = RedrawStatus::Swapin;
        mDraw ^= 1;
    }

    if (mStatus[mSwap] == RedrawStatus::Swapin) {
        for (;;) {
            mBuffers[mSwap]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort = mDbufPort;
            if (ChangeScreenBuffer(mGraphics.screen, mBuffers[mSwap]) != 0) {
                mStatus[mSwap] = RedrawStatus::Wait;
                mSwap ^= 1;
                break;
            } else {
                WaitTOF();
            }
        }
    }
}

static void StripIntuiMessages(Window* win)
{
    IntuiMessage* msg;
    while ((msg = reinterpret_cast<IntuiMessage*>(GetMsg(win->UserPort))) != nullptr) {
        ReplyMsg((struct Message*)msg);
    }
}

void Renderer::cleanup()
{
    auto that = sInstance;
    if (!that) {
        return;
    }

    // wait for all screen buffers to become available
    while (that->mStatus[0] == RedrawStatus::Wait || that->mStatus[1] == RedrawStatus::Wait) {
        Wait(1 << that->mDbufPort->mp_SigBit);
        that->processDbuf();
    }

    // change screen to show buffer 0
    while (ChangeScreenBuffer(that->mGraphics.screen, that->mBuffers[0]) == 0) {
        WaitTOF();
    }

    // clear any dbuf messages
    while (GetMsg(that->mDbufPort)) {
        // just clear the messages
    }

    Forbid();

    ModifyIDCMP(that->mGraphics.window, 0);
    StripIntuiMessages(that->mGraphics.window);

    Permit();

    CloseWindow(that->mGraphics.window);

    FreeScreenBuffer(that->mGraphics.screen, that->mBuffers[1]);
    FreeScreenBuffer(that->mGraphics.screen, that->mBuffers[0]);

    CloseScreen(that->mGraphics.screen);

    DeleteMsgPort(that->mDbufPort);
    //DeleteMsgPort(that->mUserPort);

    delete sInstance;
    sInstance = nullptr;
}

ULONG Renderer::addRenderer(trost::Function<void(Context*)>&& handler)
{
    ULONG id = mNextId++;
    mHandlers.push_back({ id, std::move(handler) });
    return id;
}

void Renderer::removeRenderer(ULONG id)
{
    const auto sz = mHandlers.size();
    for (std::size_t i = 0; i < sz; ++i) {
        if (mHandlers[i].id == id) {
            mHandlers.remove_at(i);
            return;
        }
    }
}
