#include "App.h"
#include "Input.h"
#include "Renderer.h"
#include "Messages.h"
#include <clib/exec_protos.h>
#include <cstdio>

using namespace trost;

App* App::sInstance = nullptr;

bool App::initialize()
{
    if (sInstance) {
        return true;
    }

    sInstance = new App();

    if (!Renderer::initialize()) {
        delete sInstance->mRenderer;
        sInstance->mRenderer = nullptr;
        return false;
    }

    sInstance->mRenderer = Renderer::instance();
    sInstance->mRenderSig = sInstance->mRenderer->sigBit();
    sInstance->mGraphics = sInstance->mRenderer->graphics();

    Input::initialize(sInstance->mGraphics);
    sInstance->mInput = Input::instance();
    sInstance->mInputSig = sInstance->mInput->sigBit();

    Messages::initialize(sInstance->mGraphics);
    sInstance->mMessages = Messages::instance();
    sInstance->mMessageSig = sInstance->mMessages->sigBit();

    return true;
}

void App::cleanup()
{
    if (!sInstance) {
        return;
    }

    Input::cleanup();
    Messages::cleanup();
    Renderer::cleanup();

    delete sInstance;
    sInstance = nullptr;
}

App* App::instance()
{
    return sInstance;
}

void App::iterateLoop()
{
    // if the renderer is ready, just render
    if (!mRenderer->isWaiting()) {
        mRenderer->render();
        return;
    }

    const auto sigs = Wait((1 << mInputSig) | (1 << mRenderSig) | (1 << mMessageSig));
    if (sigs &(1 << mInputSig)) {
        mInput->processInput();
    }
    if (sigs & (1 << mRenderSig)) {
        mRenderer->processDbuf();
    }
    if (sigs & (1 << mMessageSig)) {
        mMessages->processMessages();
    }

    mRenderer->render();
}
