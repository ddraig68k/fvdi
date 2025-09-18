#include <stdint.h>
#include "driver.h"
#include "gfxvga.h"

long CDECL 
c_mouse_draw(Workstation *UNUSED(wk), long UNUSED(x), long UNUSED(y), Mouse *UNUSED(mouse))
{
    return 1;
    //return 0;
}
