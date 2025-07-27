#include "Input.h"
#include "App.h"
#include "Messages.h"
#include "Renderer.h"
#include <clib/alib_protos.h>
#include <clib/keymap_protos.h>
#include <clib/graphics_protos.h>
#include <clib/exec_protos.h>
#include <devices/gameport.h>
#include <cstring>
#include <cstdio>

extern "C" ExecBase *SysBase;

using namespace trost;

// x -1 is left, 1 is right
// y -1 is up,   1 is down

using JoyDirection = Input::JoyDirection;

static const JoyDirection joyDirectionLookup[3][3] = {
    // y = -1                                 0,                   1 (up)
    { JoyDirection::Left  | JoyDirection::Up, JoyDirection::Left,  JoyDirection::Left  | JoyDirection::Down },  // x = -1
    { JoyDirection::Down,                     JoyDirection::None,  JoyDirection::Up                         },  // x =  0
    { JoyDirection::Right | JoyDirection::Up, JoyDirection::Right, JoyDirection::Right | JoyDirection::Down }   // x = +1
};

// the following functions are lifted from https://wiki.amigaos.net/wiki/Gameport_Device
static BOOL set_controller_type(BYTE type, struct IOStdReq *game_io_msg)
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

static void set_trigger_conditions(struct GamePortTrigger *gpt,
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

static void flush_buffer(struct IOStdReq *game_io_msg)
{
    game_io_msg->io_Command = CMD_CLEAR;
    game_io_msg->io_Flags   = IOF_QUICK;
    game_io_msg->io_Data    = NULL;
    game_io_msg->io_Length  = 0;
    DoIO(reinterpret_cast<IORequest*>(game_io_msg));
}

static void free_gp_unit(struct IOStdReq *game_io_msg)
{
    BYTE type = GPCT_NOCONTROLLER;

    game_io_msg->io_Command = GPD_SETCTYPE;
    game_io_msg->io_Flags   = IOF_QUICK;
    game_io_msg->io_Data    = (APTR)&type;
    game_io_msg->io_Length  = 1;
    DoIO(reinterpret_cast<IORequest*>(game_io_msg));
}

static void send_read_request(struct InputEvent *game_event,
                       struct IOStdReq *game_io_msg)
{
    game_io_msg->io_Command = GPD_READEVENT;
    game_io_msg->io_Flags   = 0;
    game_io_msg->io_Data    = (APTR)game_event;
    game_io_msg->io_Length  = sizeof(struct InputEvent);
    SendIO(reinterpret_cast<IORequest*>(game_io_msg));  /* Asynchronous - message will return later */
}

namespace trost {
bool acquireKeyInput(const KeyInputOptions& options, KeyInput* input)
{
    InputEvent ie;

    char output[10];
    unsigned int pos = 0;
    long x = options.rect.x, y = options.rect.y;

    static auto keymap = AskKeyMapDefault();

    auto inputInstance = Input::instance();
    bool success = false;
    bool done = false;
    auto inputId = inputInstance->addKeyboard([&](IntuiMessage* msg) -> void {
        auto code = msg->Code;
        auto qualifier = msg->Qualifier;
        if ((code & 0x80) == 0) {
            // key press
            switch (code) {
            case 0x45: // Escape key
                done = true;
                break;
            case 0x44: // Enter key
                input->buffer[pos] = '\0';
                input->length = pos;
                success = done = true;
                break;
            case 0x41: // Backspace key
            case 0x46: // Delete key
                pos--;
                break;
            default:
                ie.ie_Class = IECLASS_RAWKEY;
                ie.ie_Code = code;
                ie.ie_Qualifier = qualifier;
                ie.ie_EventAddress = NULL;
                if (MapRawKey(&ie, (STRPTR)output, sizeof(output), keymap) > 0) {
                    char ch = output[0];
                    if (pos < sizeof(input->buffer) - 1 && ch >= 32 && ch <= 126) {
                        input->buffer[pos++] = ch;
                    }
                } else {
                    // map failed, bail out. should surface this somehow
                    //printf("Failed to map raw key: %d %d\n", code, qualifier);
                    //done = true;
                }
            }
        }
    }, Input::AddMode::Exclusive);

    auto app = App::instance();

    auto renderer = Renderer::instance();

    auto rendererId = renderer->addRenderer([&](Renderer::Context* ctx) -> void {
        auto rp = ctx->rastPort;

        long cx = x, cy = y;
        Move(rp, cx, cy);
        if (options.message) {
            auto messageLen = options.messageLength ? options.messageLength : strlen(options.message);
            Text(rp, options.message, messageLen);
            cy += 20;
            Move(rp, cx, cy);
        }
        if (pos > 0) {
            Text(rp, input->buffer, pos);
        }
    });

    while (!done) {
        app->iterateLoop();
    }

    renderer->removeRenderer(rendererId);
    inputInstance->removeKeyboard(inputId);
    return true;
}

Input* Input::sInstance = nullptr;

void Input::initialize(const Graphics* graphics)
{
    if (sInstance) {
        return;
    }
    sInstance = new Input();
    sInstance->mGraphics = graphics;

    auto inputPort = CreatePort("RKM_game_port", 0);

    GamePortTrigger joytrigger;
    auto inputRequest = CreateExtIO(inputPort, sizeof(IOStdReq));
    inputRequest->io_Message.mn_Node.ln_Type = NT_UNKNOWN;
    if (OpenDevice("gameport.device", 1, inputRequest, 0) != 0) {
        DeleteMsgPort(inputPort);
        inputPort = nullptr;
        printf("Failed to open gameport.device\n");
        return;
    }
    if (!set_controller_type(GPCT_ABSJOYSTICK, reinterpret_cast<IOStdReq*>(inputRequest))) {
        CloseDevice(reinterpret_cast<IORequest*>(inputRequest));
        DeleteMsgPort(inputPort);
        inputPort = nullptr;
        printf("Failed to acquire joystick\n");
        return;
    }
    set_trigger_conditions(&joytrigger, reinterpret_cast<IOStdReq*>(inputRequest));
    flush_buffer(reinterpret_cast<IOStdReq*>(inputRequest));

    sInstance->mInputPort = inputPort;
    sInstance->mInputRequest = inputRequest;
    send_read_request(&sInstance->mGameEvent, reinterpret_cast<IOStdReq*>(inputRequest));

    auto messages = Messages::instance();
    sInstance->mMessageId = messages->addHandler(IDCMP_RAWKEY, [that = sInstance](IntuiMessage* msg) -> void {
        if (that->mExclusiveKeyboard) {
            // find the exclusive handler
            const auto sz = that->mKeyboards.size();
            for (std::size_t i = 0; i < sz; ++i) {
                if (that->mKeyboards[i].id == that->mExclusiveKeyboard) {
                    that->mKeyboards[i].handler(msg);
                    break;
                }
            }
            return;
        }

        // call all handlers
        const auto sz = that->mKeyboards.size();
        for (std::size_t i = 0; i < sz; ++i) {
            that->mKeyboards[i].handler(msg);
        }
    });
}

void Input::cleanup()
{
    if (!sInstance) {
        return;
    }

    Messages::instance()->removeHandler(sInstance->mMessageId);

    auto that = sInstance;
    if (!CheckIO(reinterpret_cast<IORequest*>(that->mInputRequest))) {
        AbortIO(reinterpret_cast<IORequest*>(that->mInputRequest));
        WaitIO(reinterpret_cast<IORequest*>(that->mInputRequest));
    }

    free_gp_unit(reinterpret_cast<IOStdReq*>(that->mInputRequest));

    WaitIO(reinterpret_cast<IORequest*>(that->mInputRequest));

    CloseDevice(reinterpret_cast<IORequest*>(that->mInputRequest));
    DeleteExtIO(reinterpret_cast<IORequest*>(that->mInputRequest));
    DeletePort(that->mInputPort);

    delete sInstance;
    sInstance = nullptr;
}

Input* Input::instance()
{
    return sInstance;
}

ULONG Input::addKeyboard(Function<void(IntuiMessage*)>&& handler, AddMode mode)
{
    ULONG id = ++mNextKeyboardId;

    if (mode == AddMode::Exclusive) {
        if (mExclusiveKeyboard != 0) {
            return 0;
        }
        mExclusiveKeyboard = id;
    }

    mKeyboards.push_back({ id, std::move(handler) });
    return id;
}

ULONG Input::addJoystick(Function<void(JoystickEvent*)>&& handler, AddMode mode)
{
    ULONG id = ++mNextJoystickId;

    if (mode == AddMode::Exclusive) {
        if (mExclusiveJoystick != 0) {
            return 0;
        }
        mExclusiveJoystick = id;
    }

    mJoysticks.push_back({ id, std::move(handler) });
    return id;
}

void Input::removeKeyboard(ULONG id)
{
    const auto sz = mKeyboards.size();
    for (std::size_t i = 0; i < sz; ++i) {
        if (mKeyboards[i].id == id) {
            mKeyboards.remove_at(i);

            if (mExclusiveKeyboard == id) {
                mExclusiveKeyboard = 0;
            }

            return;
        }
    }
}

void Input::removeJoystick(ULONG id)
{
    const auto sz = mJoysticks.size();
    for (std::size_t i = 0; i < sz; ++i) {
        if (mJoysticks[i].id == id) {
            mJoysticks.remove_at(i);

            if (mExclusiveJoystick == id) {
                mExclusiveJoystick = 0;
            }

            return;
        }
    }
}

void Input::processInput()
{
    struct Message *imsg;
    if ((imsg = GetMsg(mInputPort))) {
        mJoystickEvent = { JoyDirection::None, JoyButton::None };

        bool send = true;
        switch (mGameEvent.ie_Code) {
        case IECODE_LBUTTON:
            // fire button pressed
            mJoystickEvent.buttons |= JoyButton::Button1Down;
            break;
        case IECODE_LBUTTON | IECODE_UP_PREFIX:
            // fire button released
            mJoystickEvent.buttons |= JoyButton::Button1Up;
            break;
        case IECODE_RBUTTON:
            // alt button pressed
            mJoystickEvent.buttons |= JoyButton::Button2Down;
            break;
        case IECODE_RBUTTON | IECODE_UP_PREFIX:
            // alt button released
            mJoystickEvent.buttons |= JoyButton::Button2Up;
            break;
        case IECODE_NOBUTTON: {
            // check for move
            const auto xmove = mGameEvent.ie_X;
            const auto ymove = mGameEvent.ie_Y;
            if (xmove == 0 && ymove == 0) {
                // this might be a timeout
                if (mGameEvent.ie_TimeStamp.tv_secs >= (UWORD)(SysBase->VBlankFrequency) * 10) {
                    send = false;
                    break; // timeout, ignore
                }
            }
            mJoystickEvent.directions = joyDirectionLookup[xmove + 1][ymove + 1];
            //printf("Joystick moved: x=%d, y=%d, direction=%d\n", xmove, ymove, static_cast<UBYTE>(dir));

            break; }
        }

        if (send) {
            if (mExclusiveJoystick) {
                // find the exclusive handler
                const auto sz = mJoysticks.size();
                for (std::size_t i = 0; i < sz; ++i) {
                    if (mJoysticks[i].id == mExclusiveJoystick) {
                        mJoysticks[i].handler(&mJoystickEvent);
                        break;
                    }
                }
            } else {
                // call all handlers
                const auto sz = mJoysticks.size();
                for (std::size_t i = 0; i < sz; ++i) {
                    mJoysticks[i].handler(&mJoystickEvent);
                }
            }
        }
    }

    send_read_request(&mGameEvent, reinterpret_cast<IOStdReq*>(mInputRequest));
}

UBYTE Input::sigBit() const
{
    return mInputPort->mp_SigBit;
}

}
