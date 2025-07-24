#pragma once

#include "Graphics.h"
#include <clib/intuition_protos.h>
#include <tuple>
#include <functional>
#include <vector>

class Messages
{
public:
    static Messages* instance();

    ULONG addHandler(ULONG clazz, std::function<void(IntuiMessage*)>&& handler);
    void removeHandler(ULONG id);

    void processMessage(const Graphics* graphics);

private:
    Messages() = default;

    std::vector<std::tuple<ULONG, ULONG, std::function<void(IntuiMessage*)>>> mHandlers;
    ULONG mNextId = 0;

    static Messages* sInstance;
};
