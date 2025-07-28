#pragma once

#include "Graphics.h"
#include "util/Function.h"
#include "util/Vector.h"
#include "Rect.h"
#include "util/Flags.h"
#include <clib/intuition_protos.h>
#include <devices/inputevent.h>

namespace trost {

class Input
{
public:
    static void initialize(const Graphics* graphics);
    static void cleanup();

    static Input* instance();

    enum class AddMode {
        Normal,
        Exclusive,
    };

    enum class JoyDirection {
        None  = 0x0,
        Left  = 0x1,
        Right = 0x2,
        Up    = 0x4,
        Down  = 0x8,
    };

    enum class JoyButton {
        None         = 0x0,
        Button1Down  = 0x1,
        Button1Up    = 0x2,
        Button2Down  = 0x4,
        Button2Up    = 0x8,
    };

    struct JoystickEvent
    {
        JoyDirection directions;
        JoyButton buttons;
    };

    ULONG addKeyboard(Function<void(IntuiMessage*)>&& handler, AddMode mode = AddMode::Normal);
    ULONG addJoystick(Function<void(JoystickEvent*)>&& handler, AddMode mode = AddMode::Normal);

    void removeKeyboard(ULONG id);
    void removeJoystick(ULONG id);

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
    JoystickEvent mJoystickEvent;

    struct KeyboardEntry
    {
        ULONG id;
        trost::Function<void(IntuiMessage*)> handler;
    };
    Vector<KeyboardEntry> mKeyboards;
    ULONG mExclusiveKeyboard = 0;

    struct JoystickEntry
    {
        ULONG id;
        trost::Function<void(JoystickEvent*)> handler;
    };
    Vector<JoystickEntry> mJoysticks;
    ULONG mExclusiveJoystick = 0;

    ULONG mNextKeyboardId = 0, mNextJoystickId = 0;
    ULONG mMessageId = 0;

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

template<>
struct EnableBitMaskOperators<Input::JoyDirection>
{
    static constexpr bool enable = true;
};

template<>
struct EnableBitMaskOperators<Input::JoyButton>
{
    static constexpr bool enable = true;
};

} // namespace trost
