#include "Messages.h"
#include <clib/exec_protos.h>

using namespace trost;

Messages* Messages::sInstance = nullptr;

void Messages::initialize(const Graphics* graphics)
{
    if (sInstance) {
        return;
    }

    sInstance = new Messages();
    sInstance->mGraphics = graphics;
    sInstance->mUserPort = graphics->window->UserPort;
}

void Messages::cleanup()
{
    if (!sInstance) {
        return;
    }

    delete sInstance;
    sInstance = nullptr;
}

Messages* Messages::instance()
{
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

UBYTE Messages::sigBit() const
{
    return mUserPort->mp_SigBit;
}

void Messages::processMessages()
{
    IntuiMessage* msg;
    while ((msg = reinterpret_cast<IntuiMessage*>(GetMsg(mGraphics->window->UserPort))) != nullptr) {
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

void Messages::processOneMessage(ULONG clazz)
{
    IntuiMessage* msg;
    WaitPort(mGraphics->window->UserPort);
    while ((msg = reinterpret_cast<IntuiMessage*>(GetMsg(mGraphics->window->UserPort))) != nullptr) {
        const auto sz = mHandlers.size();
        for (std::size_t i = 0; i < sz; ++i) {
            auto& entry = mHandlers[i];
            if (entry.clazz == msg->Class) {
                entry.handler(msg);
                if (entry.clazz == clazz) {
                    ReplyMsg((struct Message*)msg);
                    return;
                }
            }
        }
        ReplyMsg((struct Message*)msg);
    }
}
