#pragma once

#include "Graphics.h"
#include "util/Function.h"
#include "util/Vector.h"
#include <clib/intuition_protos.h>

namespace trost {

class Messages
{
public:
    static void initialize(const Graphics* graphics);
    static void cleanup();

    static Messages* instance();

    ULONG addHandler(ULONG clazz, trost::Function<void(IntuiMessage*)>&& handler);
    void removeHandler(ULONG id);

    void processMessages();
    void processOneMessage(ULONG clazz);

    UBYTE sigBit() const;

private:
    Messages() = default;

    const Graphics* mGraphics = nullptr;
    MsgPort* mUserPort = nullptr;

    struct Entry
    {
        ULONG id;
        ULONG clazz;
        trost::Function<void(IntuiMessage*)> handler;
    };

    Vector<Entry> mHandlers;
    ULONG mNextId = 0;

    static Messages* sInstance;
};

} // namespace trost
