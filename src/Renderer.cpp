#include "Renderer.h"
#include "Messages.h"
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>

using namespace trost;

Renderer* Renderer::sInstance = nullptr;

void Renderer::initialize()
{
    mBuffers[0] = AllocScreenBuffer(mGraphics.screen, nullptr, SB_SCREEN_BITMAP);
    mBuffers[1] = AllocScreenBuffer(mGraphics.screen, nullptr, SB_COPY_BITMAP);
    mBuffers[0]->sb_DBufInfo->dbi_UserData1 = reinterpret_cast<APTR>(0);
    mBuffers[1]->sb_DBufInfo->dbi_UserData1 = reinterpret_cast<APTR>(1);

    InitRastPort(&mRastPorts[0]);
    mRastPorts[0].BitMap = mBuffers[0]->sb_BitMap;

    InitRastPort(&mRastPorts[1]);
    mRastPorts[1].BitMap = mBuffers[1]->sb_BitMap;

    mDbufPort = CreateMsgPort();
    mUserPort = CreateMsgPort();

    mGraphics.window->UserPort = mUserPort;
}

void Renderer::initialize(const Graphics* graphics)
{
    sInstance = new Renderer();
    sInstance->mGraphics.screen = graphics->screen;
    sInstance->mGraphics.window = graphics->window;
    sInstance->initialize();
}

Renderer* Renderer::instance()
{
    return sInstance;
}

void Renderer::render()
{
    // if there are no handlers, just process messages at refresh rate
    if (mHandlers.size() == 0) {
        Messages::instance()->processMessage(&mGraphics);
        WaitTOF();
        return;
    }

    if (mStatus[mDraw] == RedrawStatus::Wait) {
        const auto sigs = Wait((1 << mDbufPort->mp_SigBit) | (1 << mUserPort->mp_SigBit));
        if (sigs & (1 << mDbufPort->mp_SigBit)) {
            struct Message *dbmsg;
            while ((dbmsg = GetMsg(mDbufPort))) {
                ULONG buffer = reinterpret_cast<ULONG>(*(reinterpret_cast<APTR**>(dbmsg + 1)));
                mStatus[buffer ^ 1] = RedrawStatus::Redraw;
            }
        }
        if (sigs & (1 << mUserPort->mp_SigBit)) {
            Messages::instance()->processMessage(&mGraphics);
        }
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
    FreeScreenBuffer(mGraphics.screen, mBuffers[0]);
    FreeScreenBuffer(mGraphics.screen, mBuffers[1]);

    Forbid();

    StripIntuiMessages(mGraphics.window);
    mGraphics.window->UserPort = nullptr;
    ModifyIDCMP(mGraphics.window, 0);

    Permit();

    DeleteMsgPort(mDbufPort);
    DeleteMsgPort(mUserPort);
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
