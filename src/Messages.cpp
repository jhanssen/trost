#include "Messages.h"
#include <clib/exec_protos.h>

using namespace trost;

Messages* Messages::sInstance = nullptr;

Messages* Messages::instance()
{
    if (!sInstance) {
        sInstance = new Messages();
    }
    return sInstance;
}

ULONG Messages::addHandler(ULONG clazz, trost::Function<void(IntuiMessage*)>&& handler)
{
    ULONG id = mNextId++;
    mHandlers.push_back({ id, clazz, std::move(handler) });
    return id;
}

void Messages::removeHandler(ULONG id)
{
    const auto sz = mHandlers.size();
    for (std::size_t i = 0; i < sz; ++i) {
        if (mHandlers[i].id == id) {
            mHandlers.remove_at(i);
            return;
        }
    }
}

void Messages::processMessage(const Graphics* graphics)
{
    IntuiMessage* msg;
    //WaitPort(graphics->window->UserPort);
    while ((msg = reinterpret_cast<IntuiMessage*>(GetMsg(graphics->window->UserPort))) != nullptr) {
        const auto sz = mHandlers.size();
        for (std::size_t i = 0; i < sz; ++i) {
            auto& entry = mHandlers[i];
            if (entry.clazz == msg->Class) {
                entry.handler(msg);
            }
        }
        ReplyMsg((struct Message*)msg);
    }
}
