#pragma once

// #define VIRTUAL_ACTIVE

#ifdef VIRTUAL_ACTIVE
#define VVIRTUAL virtual
#define VOVERRIDE override
#define VSELECT(x) x
#define VINTERFACE(x) I##x
#else
#define VVIRTUAL
#define VOVERRIDE
#define VSELECT(x)
#define VINTERFACE(x) x
#endif