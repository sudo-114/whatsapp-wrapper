#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "window.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        // Older Windows versions may not support this; continue with defaults.
    }
    return RunApplication(instance, showCommand);
}
