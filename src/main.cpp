#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/exec.h>
#include <clib/alib_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/keymap_protos.h>
#include <devices/gameport.h>
#include <devices/inputevent.h>
#include <cstdio>
#include "Graphics.h"
#include "Renderer.h"
#include "Input.h"

#define MAX_INPUT 128

int main(int /*argc*/, char** /*argv*/)
{
    trost::Graphics graphics;

    graphics.screen = OpenScreenTags(NULL,
                                    SA_Width, 320,
                                    SA_Height, 256,
                                    SA_Depth, 5,
                                    SA_Type, CUSTOMSCREEN,
                                    SA_Title, "Full Screen Program",
                                    SA_ShowTitle, FALSE,
                                    SA_Quiet, TRUE,
                                    TAG_DONE);
    if (!graphics.screen) {
        printf("Failed to open screen\n");
        return 1;
    }

    graphics.window = OpenWindowTags(NULL,
                                     WA_Left,        0,
                                     WA_Top,         0,
                                     WA_Width,       320,
                                     WA_Height,      256,
                                     WA_IDCMP,       IDCMP_RAWKEY,
                                     WA_Flags,       WFLG_SIMPLE_REFRESH |
                                                     WFLG_BACKDROP |
                                                     WFLG_BORDERLESS |
                                                     WFLG_ACTIVATE,
                                     WA_CustomScreen,(ULONG)graphics.screen,
                                     TAG_DONE);
    if (!graphics.window) {
        printf("Failed to open window\n");
        CloseScreen(graphics.screen);
        return 1;
    }

    if (!trost::Renderer::initialize(&graphics)) {
        printf("Failed to initialize renderer\n");
        CloseWindow(graphics.window);
        CloseScreen(graphics.screen);
        return 1;
    }

    auto renderer = trost::Renderer::instance();

    // Your drawing logic or other code here
    SetRGB4(&(graphics.screen->ViewPort), 0, 15, 0, 0);
    // set pen color to white
    SetRGB4(&(graphics.screen->ViewPort), 1, 15, 15, 15);

    auto helloId = renderer->addRenderer([](trost::Renderer::Context* ctx) {
        const auto rp = ctx->rastPort;

        Move(rp, 10, 10);
        Text(rp, "Hello, World!", 13);
    });

    for (int n = 0; n < 200; ++n) {
        renderer->render();
    }

    renderer->removeRenderer(helloId);

    trost::KeyInput input;
    if (trost::acquireKeyInput({ &graphics, { 10, 50, 0, 0 }, "Type something" }, &input)) {
        printf("Input received: %s\n", input.buffer);
    }

    renderer->cleanup();

    CloseWindow(graphics.window);
    CloseScreen(graphics.screen);

    return 0;
}
