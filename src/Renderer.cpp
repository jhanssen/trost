#include "Renderer.h"
#include "Messages.h"
#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <devices/gameport.h>
#include <cstdio>

extern "C" ExecBase *SysBase;

using namespace trost;

Renderer* Renderer::sInstance = nullptr;

enum JoyDirection {
    Joy_None,
    Joy_Left = 0x1,
    Joy_Right = 0x2,
    Joy_Up = 0x4,
    Joy_Down = 0x8,
};

// x -1 is left, 1 is right
// y -1 is up,   1 is down
static const BYTE joyDirectionLookup[3][3] = {
    // y = -1     0,                 1 (up)
    { Joy_Left  | Joy_Up, Joy_Left,  Joy_Left  | Joy_Down },  // x = -1
    { Joy_Down,           Joy_None,  Joy_Up               },  // x =  0
    { Joy_Right | Joy_Up, Joy_Right, Joy_Right | Joy_Down }   // x = +1
};

// the following functions are lifted from https://wiki.amigaos.net/wiki/Gameport_Device
BOOL set_controller_type(BYTE type, struct IOStdReq *game_io_msg)
{
    BOOL success = FALSE;
    BYTE controller_type = 0;

    /* begin critical section
    ** we need to be sure that between the time we check that the controller
    ** is available and the time we allocate it, no one else steals it.
    */
    Forbid();

    game_io_msg->io_Command = GPD_ASKCTYPE;    /* inquire current status */
    game_io_msg->io_Flags   = IOF_QUICK;
    game_io_msg->io_Data    = (APTR)&controller_type; /* put answer in here */
    game_io_msg->io_Length  = 1;
    DoIO(reinterpret_cast<IORequest*>(game_io_msg));

    /* No one is using this device unit, let's claim it */
    if (controller_type == GPCT_NOCONTROLLER)
    {
        game_io_msg->io_Command = GPD_SETCTYPE;
        game_io_msg->io_Flags   = IOF_QUICK;
        game_io_msg->io_Data    = (APTR)&type;
        game_io_msg->io_Length  = 1;
        DoIO(reinterpret_cast<IORequest*>(game_io_msg));
        success = TRUE;
    }

    Permit(); /* critical section end */
    return(success);
}

void set_trigger_conditions(struct GamePortTrigger *gpt,
                            struct IOStdReq *game_io_msg)
{
    /* trigger on all joystick key transitions */
    gpt->gpt_Keys   = GPTF_UPKEYS | GPTF_DOWNKEYS;
    gpt->gpt_XDelta = 1;
    gpt->gpt_YDelta = 1;
    /* timeout trigger every TIMEOUT_SECONDS second(s) */
    gpt->gpt_Timeout = (UWORD)(SysBase->VBlankFrequency) * 10;

    game_io_msg->io_Command = GPD_SETTRIGGER;
    game_io_msg->io_Flags   = IOF_QUICK;
    game_io_msg->io_Data    = (APTR)gpt;
    game_io_msg->io_Length  = (LONG)sizeof(struct GamePortTrigger);
    DoIO(reinterpret_cast<IORequest*>(game_io_msg));
}

void flush_buffer(struct IOStdReq *game_io_msg)
{
    game_io_msg->io_Command = CMD_CLEAR;
    game_io_msg->io_Flags   = IOF_QUICK;
    game_io_msg->io_Data    = NULL;
    game_io_msg->io_Length  = 0;
    DoIO(reinterpret_cast<IORequest*>(game_io_msg));
}

void free_gp_unit(struct IOStdReq *game_io_msg)
{
    BYTE type = GPCT_NOCONTROLLER;

    game_io_msg->io_Command = GPD_SETCTYPE;
    game_io_msg->io_Flags   = IOF_QUICK;
    game_io_msg->io_Data    = (APTR)&type;
    game_io_msg->io_Length  = 1;
    DoIO(reinterpret_cast<IORequest*>(game_io_msg));
}

void send_read_request(struct InputEvent *game_event,
                       struct IOStdReq *game_io_msg)
{
    game_io_msg->io_Command = GPD_READEVENT;
    game_io_msg->io_Flags   = 0;
    game_io_msg->io_Data    = (APTR)game_event;
    game_io_msg->io_Length  = sizeof(struct InputEvent);
    SendIO(reinterpret_cast<IORequest*>(game_io_msg));  /* Asynchronous - message will return later */
}

bool Renderer::initialize()
{
    mInputPort = CreatePort("RKM_game_port", 0);

    GamePortTrigger joytrigger;
    mInputRequest = CreateExtIO(mInputPort, sizeof(IOStdReq));
    mInputRequest->io_Message.mn_Node.ln_Type = NT_UNKNOWN;
    if (OpenDevice("gameport.device", 1, mInputRequest, 0) != 0) {
        DeleteMsgPort(mInputPort);
        mInputPort = nullptr;
        printf("Failed to open gameport.device\n");
        return false;
    }
    if (!set_controller_type(GPCT_ABSJOYSTICK, reinterpret_cast<IOStdReq*>(mInputRequest))) {
        CloseDevice(reinterpret_cast<IORequest*>(mInputRequest));
        DeleteMsgPort(mInputPort);
        mInputPort = nullptr;
        printf("Failed to acquire joystick\n");
        return false;
    }
    set_trigger_conditions(&joytrigger, reinterpret_cast<IOStdReq*>(mInputRequest));
    flush_buffer(reinterpret_cast<IOStdReq*>(mInputRequest));

    send_read_request(&mGameEvent, reinterpret_cast<IOStdReq*>(mInputRequest));

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
    return true;
}

bool Renderer::initialize(const Graphics* graphics)
{
    sInstance = new Renderer();
    sInstance->mGraphics.screen = graphics->screen;
    sInstance->mGraphics.window = graphics->window;
    return sInstance->initialize();
}

Renderer* Renderer::instance()
{
    return sInstance;
}

void Renderer::render()
{
    // if there are no handlers, just process messages at refresh rate
    if (mHandlers.size() == 0) {
        Messages::instance()->processMessages(&mGraphics);
        WaitTOF();
        return;
    }

    if (mStatus[mDraw] == RedrawStatus::Wait) {
        const auto sigs = Wait((1 << mDbufPort->mp_SigBit) | (1 << mUserPort->mp_SigBit) | (1 << mInputPort->mp_SigBit));
        if (sigs & (1 << mDbufPort->mp_SigBit)) {
            struct Message *dbmsg;
            while ((dbmsg = GetMsg(mDbufPort))) {
                ULONG buffer = reinterpret_cast<ULONG>(*(reinterpret_cast<APTR**>(dbmsg + 1)));
                mStatus[buffer ^ 1] = RedrawStatus::Redraw;
            }
        }
        if (sigs & (1 << mUserPort->mp_SigBit)) {
            Messages::instance()->processMessages(&mGraphics);
        }
        if (sigs & (1 << mInputPort->mp_SigBit)) {
            printf("input\n");
            struct Message *imsg;
            if ((imsg = GetMsg(mInputPort))) {
                switch (mGameEvent.ie_Code) {
                case IECODE_LBUTTON:
                    // fire button pressed
                    printf("Left button pressed\n");
                    break;
                case IECODE_LBUTTON | IECODE_UP_PREFIX:
                    // fire button released
                    break;
                case IECODE_RBUTTON:
                    // alt button pressed
                    break;
                case IECODE_RBUTTON | IECODE_UP_PREFIX:
                    // alt button released
                    break;
                case IECODE_NOBUTTON: {
                    // check for move
                    const auto xmove = mGameEvent.ie_X;
                    const auto ymove = mGameEvent.ie_Y;
                    if (xmove == 0 && ymove == 0) {
                        // this might be a timeout
                        if (mGameEvent.ie_TimeStamp.tv_secs >= (UWORD)(SysBase->VBlankFrequency) * 10) {
                            break; // timeout, ignore
                        }
                    }
                    auto dir = joyDirectionLookup[xmove + 1][ymove + 1];
                    printf("Joystick moved: x=%d, y=%d, direction=%d\n", xmove, ymove, dir);

                    break; }
                }
            }

            send_read_request(&mGameEvent, reinterpret_cast<IOStdReq*>(mInputRequest));
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
    //FreeScreenBuffer(mGraphics.screen, mBuffers[0]);
    FreeScreenBuffer(mGraphics.screen, mBuffers[1]);

    Forbid();

    StripIntuiMessages(mGraphics.window);
    mGraphics.window->UserPort = nullptr;
    ModifyIDCMP(mGraphics.window, 0);

    Permit();

    printf("out.\n");

    if (!CheckIO(reinterpret_cast<IORequest*>(mInputRequest))) {
        AbortIO(reinterpret_cast<IORequest*>(mInputRequest));
        WaitIO(reinterpret_cast<IORequest*>(mInputRequest));
    }

    free_gp_unit(reinterpret_cast<IOStdReq*>(mInputRequest));

    WaitIO(reinterpret_cast<IORequest*>(mInputRequest));

    CloseDevice(reinterpret_cast<IORequest*>(mInputRequest));
    DeleteExtIO(reinterpret_cast<IORequest*>(mInputRequest));
    DeletePort(mInputPort);
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
