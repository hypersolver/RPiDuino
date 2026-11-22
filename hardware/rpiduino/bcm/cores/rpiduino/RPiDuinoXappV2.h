#pragma once

#if !defined(RPIDUINO_XDOS_XAPP_V2)
#error "This API requires the XDOS XAPP V2 build target"
#endif

// Route a diagnostic message through the XDOS host. Core 0 only.
extern "C" void rpiduino_xdos_log(const char *message);
