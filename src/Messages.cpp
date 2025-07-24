#include "Messages.h"
#include <clib/exec_protos.h>

Messages* Messages::sInstance = nullptr;

Messages* Messages::instance()
{
    if (!sInstance) {
        sInstance = new Messages();
    }
    return sInstance;
}

ULONG Messages::addHandler(ULONG clazz, std::function<void(IntuiMessage*)>&& handler)
{
    ULONG id = mNextId++;
    mHandlers.emplace_back(id, clazz, std::move(handler));
    return id;
}

void Messages::removeHandler(ULONG id)
{
    std::remove_if(mHandlers.begin(), mHandlers.end(),
                   [id](const auto& tup) { return std::get<0>(tup) == id; });
}

void Messages::processMessage(const Graphics* graphics)
{
    IntuiMessage* msg;
    bool running = true;
    while (running) {
        WaitPort(graphics->window->UserPort);
        while ((msg = reinterpret_cast<IntuiMessage*>(GetMsg(graphics->window->UserPort))) != nullptr) {
            auto it = mHandlers.begin();
            const auto end = mHandlers.end();
            for (; it != end; ++it) {
                if (std::get<1>(*it) == msg->Class) {
                    std::get<2>(*it)(msg);
                    running = false;
                }
            }
            ReplyMsg((struct Message*)msg);
        }
    }
}
