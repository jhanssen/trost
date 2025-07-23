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
#include <devices/gameport.h>
#include <devices/inputevent.h>
#include <cstdio>

int main(int /*argc*/, char** /*argv*/)
{
    Screen* myScreen;

    if ((myScreen = OpenScreenTags(NULL,
                                   SA_Width, 320,
                                   SA_Height, 256,
                                   SA_Depth, 5,
                                   SA_Type, CUSTOMSCREEN,
                                   SA_Title, "Full Screen Program",
                                   SA_ShowTitle, FALSE,
                                   SA_Quiet, TRUE,
                                   TAG_DONE))) {

        // Your drawing logic or other code here
        SetRGB4(&(myScreen->ViewPort), 0, 15, 0, 0);
        // set pen color to white
        SetRGB4(&(myScreen->ViewPort), 1, 15, 15, 15);

        auto rp = &myScreen->RastPort;
        SetAPen(rp, 1);
        SetBPen(rp, 0);

        Move(rp, 50, 50);

        Text(rp, "Hello, Amiga!", 13);

        Delay(300); // Wait ~6 seconds

        CloseScreen(myScreen);
    } else {
        printf("Failed to open screen!\n");
    }

    return 0;
}
