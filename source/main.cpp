#include <GarrysMod/Lua/Interface.h>
#include "detours.h"
#include <tier2/tier2.h>
#include <tier1.h>
#include "symbols.h"
#include "client.h"

static CClientState* clientState;
static Detouring::Hook detour_CClientState_FinishSignonState_New;
static void hook_CClientState_FinishSignonState_New()
{
	__asm {
		mov clientState, ebx; // Loads the CClientState since it won't be passed as a argument(unlike linux) on windows.
	}


	Msg("Called!\n"); // Time to figure out things from here

	detour_CClientState_FinishSignonState_New.GetTrampoline<Symbols::CClientState_FinishSignonState_New>()(clientState);
}

GMOD_MODULE_OPEN()
{
	// We won't even use lua :p

	SourceSDK::ModuleLoader engine_loader("engine");
	Detour::Create(
		&detour_CClientState_FinishSignonState_New, "CClientState::FinishSignonState_New",
		engine_loader.GetModule(), Symbols::CClientState_FinishSignonState_NewSym,
		(void*)hook_CClientState_FinishSignonState_New
	);

	return 0;
}

GMOD_MODULE_CLOSE()
{
	Detour::Remove(0);
	Detour::ReportLeak();

	return 0;
}