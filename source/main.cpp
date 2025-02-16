#include <GarrysMod/Lua/Interface.h>
#include "detours.h"
#include <tier2/tier2.h>
#include <tier1.h>

GMOD_MODULE_OPEN()
{
	// We won't even use lua :p
	return 0;
}

GMOD_MODULE_CLOSE()
{
	Detour::Remove(0);
	Detour::ReportLeak();

	return 0;
}