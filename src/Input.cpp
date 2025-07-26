#include "Input.h"
#include "Messages.h"
#include "Renderer.h"
#include <clib/keymap_protos.h>
#include <clib/graphics_protos.h>
#include <cstring>

namespace trost {
bool acquireInput(const InputOptions& options, Input* input)
{
    InputEvent ie;

    char output[10];
    unsigned int pos = 0;
    long x = options.rect.x, y = options.rect.y;

    static auto keymap = AskKeyMapDefault();

    auto messages = Messages::instance();
    bool success = false;
    bool done = false;
    auto messagesId = messages->addHandler(IDCMP_RAWKEY, [&](IntuiMessage* msg) -> void {
        auto code = msg->Code;
        auto qualifier = msg->Qualifier;
        if ((code & 0x80) == 0) {
            // key press
            switch (code) {
            case 0x45: // Escape key
                done = true;
                break;
            case 0x44: // Enter key
                input->buffer[pos] = '\0';
                input->length = pos;
                success = done = true;
                break;
            case 0x41: // Backspace key
            case 0x46: // Delete key
                pos--;
                break;
            default:
                ie.ie_Class = IECLASS_RAWKEY;
                ie.ie_Code = code;
                ie.ie_Qualifier = qualifier;
                ie.ie_EventAddress = NULL;
                if (MapRawKey(&ie, (STRPTR)output, sizeof(output), keymap) > 0) {
                    char ch = output[0];
                    if (pos < sizeof(input->buffer) - 1 && ch >= 32 && ch <= 126) {
                        input->buffer[pos++] = ch;
                    }
                } else {
                    // map failed, bail out. should surface this somehow
                    done = true;
                }
            }
        }
    });

    auto renderer = Renderer::instance();

    auto rendererId = renderer->addRenderer([&](Renderer::Context* ctx) -> void {
        auto rp = ctx->rastPort;

        long cx = x, cy = y;
        Move(rp, cx, cy);
        if (options.message) {
            auto messageLen = options.messageLength ? options.messageLength : strlen(options.message);
            Text(rp, options.message, messageLen);
            cy += 20;
            Move(rp, cx, cy);
        }
        if (pos > 0) {
            Text(rp, input->buffer, pos);
        }
    });

    while (!done) {
        renderer->render();
    }

    renderer->removeRenderer(rendererId);
    messages->removeHandler(messagesId);
    return success;
}
}
