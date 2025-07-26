#pragma once

#include "Graphics.h"
#include "Function.h"
#include "Vector.h"
#include <clib/intuition_protos.h>

namespace trost {

class Messages
{
public:
    static Messages* instance();

    ULONG addHandler(ULONG clazz, trost::Function<void(IntuiMessage*)>&& handler);
    void removeHandler(ULONG id);

    void processMessages(const Graphics* graphics);
    void processOneMessage(ULONG clazz, const Graphics* graphics);

private:
    Messages() = default;

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
