#include <GarrysMod/InterfacePointers.hpp>
#include <GarrysMod/Lua/Interface.h>
#include "detours.h"
#include "symbols.h"
#include "client.h"
#include "networkstringtable.h"
#include "vstdlib/jobthread.h"
#include "vprof.h"

static IThreadPool* g_pWorkerThreads;
static CClientState* g_pClientState;
static Detouring::Hook detour_CClientState_FinishSignonState_New;
static void hook_CClientState_FinishSignonState_New()
{
	__asm {
		mov g_pClientState, ebx; // Loads the CClientState since it won't be passed as a argument(unlike linux) on windows.
	}

	VPROF_BUDGET("LoadingScreen - CClientState::FinishSignonState_New", VPROF_BUDGETGROUP_GAME);

	Msg("Called CClientState::FinishSignonState_New!\n"); // Time to figure out things from here

	// Map loading, precaching, basically everything we could safely touch is done here.
	detour_CClientState_FinishSignonState_New.GetTrampoline<Symbols::CClientState_FinishSignonState_New>()(g_pClientState);
}



static Detouring::Hook detour_CL_InstallAndInvokeClientStringTableCallbacks;
static void hook_CL_InstallAndInvokeClientStringTableCallbacks()
{
	VPROF_BUDGET("LoadingScreen - CL_InstallAndInvokeClientStringTableCallbacks", VPROF_BUDGETGROUP_GAME);

	Msg("Called CL_InstallAndInvokeClientStringTableCallbacks!\n");

	// install hooks
	int numTables = g_pClientState->m_StringTableContainer->GetNumTables();

	for (int i=0; i<numTables; ++i)
	{
		// iterate through server tables
		CNetworkStringTable *pTable = 
			(CNetworkStringTable*)g_pClientState->m_StringTableContainer->GetTable( i );

		if ( !pTable )
			continue;

		pfnStringChanged pOldFunction = pTable->GetCallback();

		const char* pTableName = pTable->GetTableName();
		cl.InstallStringTableCallback( pTableName );

		pfnStringChanged pNewFunction = pTable->GetCallback();
		if ( !pNewFunction )
			continue;

		// We already had it installed (e.g., from client .dll) so all of the callbacks have been called and don't need a second dose
		if ( pNewFunction == pOldFunction )
			continue;

		bool bModelPrecache = V_stricmp(pTableName, "modelprecache") == 0;

		for ( int j = 0; j < pTable->GetNumStrings(); ++j )
		{
			int userDataSize;
			const void *pUserData = pTable->GetStringUserData( j, &userDataSize );
			(*pNewFunction)( NULL, pTable, j, pTable->GetString( j ), pUserData );
		}
	}
}

#if ARCHITECTURE_IS_X86_64
#define V_CreateThreadPool CreateNewThreadPool
#define V_DestroyThreadPool DestroyThreadPool
#endif

GMOD_MODULE_OPEN()
{
	// We won't even use lua :p

	ThreadPoolStartParams_t startParams;
	startParams.nThreads = 4;
	startParams.nThreadsMax = startParams.nThreads;

	g_pWorkerThreads = V_CreateThreadPool();
	g_pWorkerThreads->Start(startParams);

	g_pFullFileSystem = InterfacePointers::FileSystemClient();

	SourceSDK::ModuleLoader engine_loader("engine");
	Detour::Create(
		&detour_CClientState_FinishSignonState_New, "CClientState::FinishSignonState_New",
		engine_loader.GetModule(), Symbols::CClientState_FinishSignonState_NewSym,
		(void*)hook_CClientState_FinishSignonState_New
	);

	Detour::Create(
		&detour_CL_InstallAndInvokeClientStringTableCallbacks, "CL_InstallAndInvokeClientStringTableCallbacks",
		engine_loader.GetModule(), Symbols::CL_InstallAndInvokeClientStringTableCallbacksSym,
		(void*)hook_CL_InstallAndInvokeClientStringTableCallbacks
	);

	return 0;
}

GMOD_MODULE_CLOSE()
{
	Detour::Remove(0);
	Detour::ReportLeak();

	g_pFullFileSystem = NULL;

	return 0;
}