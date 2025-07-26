#include "Graphics.h"
#include "App.h"
#include "Input.h"
#include "Renderer.h"
#include <clib/graphics_protos.h>
#include <cstdio>

#define MAX_INPUT 128

int main(int /*argc*/, char** /*argv*/)
{
    trost::App::initialize();

    auto app = trost::App::instance();
    auto renderer = trost::Renderer::instance();

    unsigned int idx = 0;
    auto helloId = renderer->addRenderer([&idx](trost::Renderer::Context* ctx) {
        const auto rp = ctx->rastPort;

        char buf[128];
        int len = sprintf(buf, "Hello World! %u", idx++);

        Move(rp, 10, 10);
        Text(rp, buf, len);
    });

    for (int n = 0; n < 5; ++n) {
        app->iterateLoop();
    }

    renderer->removeRenderer(helloId);

    trost::KeyInput input;
    if (trost::acquireKeyInput({ renderer->graphics(), { 10, 50, 0, 0 }, "Type something" }, &input)) {
        printf("Input received: %s\n", input.buffer);
    }

    trost::App::cleanup();

    return 0;
}
