#include "GarrysMod/IMenuSystem.h"
#include <GarrysMod/InterfacePointers.hpp>
#include <GarrysMod/Lua/Interface.h>
#include "detours.h"
#include "symbols.h"
#include "client.h"
#include "baseserver.h"
#include "networkstringtable.h"
#include "vstdlib/jobthread.h"
#include "vprof.h"
#include "netmessages.h"
#include "vgui_baseui_interface.h"
#include "modelloader.h"
#include <isteamuser.h>
#include <icommandline.h>

static CClientState* g_pClientState;
static CBaseServer* g_pBaseServer;
static IThreadPool* g_pWorkerThreads;
static IThreadPool* g_pMainThreads;
IEngineVGuiInternal* enginevgui_internal;
IModelLoader* modelloader;
IVEngineClient* engineclient;
IMDLCache* mdlcache;
static IMenuSystem* g_pMenuSystem;

inline IEngineVGuiInternal* EngineVGui()
{
	return enginevgui_internal;
}

static CRC32_t g_pSendTableCRC;
static Symbols::CClientState_AddCustomFile func_CClientState_AddCustomFile;
static Symbols::SendTable_ComputeCRC func_SendTable_ComputeCRC;
void CClientState::SendClientInfo( void )
{
	CLC_ClientInfo info;
	
	info.m_nSendTableCRC = g_pSendTableCRC;
	info.m_nServerCount = m_nServerCount;
	info.m_bIsHLTV = false;
#if !defined( NO_STEAM )
	info.m_nFriendsID = SteamUser() ? SteamUser()->GetSteamID().GetAccountID() : 0;
#else
	info.m_nFriendsID = 0;
#endif
	Q_strncpy( info.m_FriendsName, m_FriendsName, sizeof(info.m_FriendsName) );

	ConVarRef cl_logofile("cl_logofile");
	func_CClientState_AddCustomFile( this, 0, cl_logofile.GetString() );

	for ( int i=0; i< MAX_CUSTOM_FILES; i++ )
		info.m_nCustomFiles[i] = m_nCustomFiles[i].crc;

	m_NetChannel->SendNetMsg( info );
}

void CClientState::SetModel( int tableIndex )
{
	if ( !m_pModelPrecacheTable )
		return;

	// Bogus index
	if ( tableIndex < 0 || tableIndex >= m_pModelPrecacheTable->GetNumStrings() )
		return;

	char const *name = m_pModelPrecacheTable->GetString( tableIndex );
	if ( tableIndex == 1 )
	{
		// The world model must match the LevelFileName -- it is the path we just checked the CRC for, and paths may differ
		// from what the server is using based on what the gameDLL override does
		name = m_szLevelFileName;
	}

	CPrecacheItem *p = &model_precache[ tableIndex ];
	bool bLoadNow = true;
	if ( CommandLine()->FindParm( "-nopreload" ) ||	CommandLine()->FindParm( "-nopreloadmodels" ))
		bLoadNow = false;
	else if ( CommandLine()->FindParm( "-preload" ) )
		bLoadNow = true;

	if ( bLoadNow )
		p->SetModel( modelloader->GetModelForName( name, IModelLoader::FMODELLOADER_CLIENT ) );
	else
		p->SetModel( NULL );
}


static void Threaded_CallStringTableCallback(MDLHandle_t handle)
{
	if (handle && handle != MDLHANDLE_INVALID)
	{
		mdlcache->TouchAllData(handle);
	}
}

pfnStringChanged CNetworkStringTable::GetCallback()
{ 
	return m_changeFunc; 
}

// Why, because else we get the funny crashes since rendering VGUI while loading breaks stuff
static bool g_bWaitingForRender = false;
static void WaitForVGUIRender()
{
	g_bWaitingForRender = true;
	while(g_bWaitingForRender)
		ThreadSleep(0);
}

/*
 * Renders the loading screen & processes all inputs later
 */
static Symbols::V_RenderVGuiOnly func_V_RenderVGuiOnly;
static Symbols::ClientDLL_ProcessInput func_ClientDLL_ProcessInput;
inline void RunVGUIFrame()
{
	if (ThreadInMainThread())
	{
		g_pMenuSystem->Think();
		func_V_RenderVGuiOnly();
	} else {
		WaitForVGUIRender();
	}
}

inline bool ShouldCancelLoading()
{
	return g_pClientState->m_nSignonState == SIGNONSTATE_NONE;
}

struct StringTableCallback {
	pfnStringChanged callbackFunc;
	void *object;
	INetworkStringTable *stringTable;
	int stringNumber;
	char const *newString;
	void const *newData;
	const char* tableName;
};

static void Threaded_StringTableCallback(StringTableCallback*& data)
{
	if (ShouldCancelLoading())
		return;

	data->callbackFunc(data->object, data->stringTable, data->stringNumber, data->newString, data->newData);

	RunVGUIFrame();

	delete data;
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
		g_pClientState->InstallStringTableCallback( pTableName );

		pfnStringChanged pNewFunction = pTable->GetCallback();
		if ( !pNewFunction )
			continue;

		// We already had it installed (e.g., from client .dll) so all of the callbacks have been called and don't need a second dose
		if ( pNewFunction == pOldFunction )
			continue;

		//bool bModelPrecache = V_stricmp(pTableName, "modelprecache") == 0;
		CUtlVector<StringTableCallback*> entries;
		for ( int j = 0; j < pTable->GetNumStrings(); ++j )
		{
			int userDataSize;
			const void *pUserData = pTable->GetStringUserData( j, &userDataSize );

			std::string_view strFile = pTable->GetString(j);
			bool bIsModel = strFile.find(".mdl") != std::string_view::npos;

			if (bIsModel)
			{
				StringTableCallback* data = new StringTableCallback;
				data->callbackFunc = pNewFunction;
				data->object = NULL;
				data->stringTable = pTable;
				data->stringNumber = j;
				data->newString = pTable->GetString( j );
				data->newData = pUserData;
				data->tableName = pTableName;

				entries.AddToTail(data);
			} else {
				(*pNewFunction)( NULL, pTable, j, pTable->GetString( j ), pUserData );
			}
		}

		ParallelProcess("Threaded_StringTableCallback", entries.Base(), entries.Count(), Threaded_StringTableCallback);
	}
}

static bool g_bIsFinishedLoading = false;
static Symbols::CL_CheckForPureServerWhitelist func_CL_CheckForPureServerWhitelist;
static Symbols::CL_RegisterResources func_CL_RegisterResources;
static Symbols::R_LevelInit func_R_LevelInit;
static Symbols::CClientState_ConsistencyCheck func_CClientState_ConsistencyCheck;
static void Threaded_ClientLoading()
{
	hook_CL_InstallAndInvokeClientStringTableCallbacks();

	if (ShouldCancelLoading())
		return;

	materials->CacheUsedMaterials();
	RunVGUIFrame();

	if (ShouldCancelLoading())
		return;

	func_CClientState_ConsistencyCheck(g_pClientState, true);
	RunVGUIFrame();

	if (ShouldCancelLoading())
		return;

	func_CL_RegisterResources();
	RunVGUIFrame();

	if (ShouldCancelLoading())
		return;

	g_bIsFinishedLoading = true;
}

static void WaitForFinish()
{
	while (true)
	{
		if (g_bIsFinishedLoading)
			break;

		while(!g_bWaitingForRender && !g_bIsFinishedLoading)
			ThreadSleep(0);

		RunVGUIFrame();
		g_bWaitingForRender = false;
	}

	g_bIsFinishedLoading = false;
}

static Detouring::Hook detour_CClientState_FinishSignonState_New;
static Symbols::CL_CheckCRCs func_CL_CheckCRCs;
static Symbols::CL_SetSteamCrashComment func_CL_SetSteamCrashComment;
static void hook_CClientState_FinishSignonState_New()
{
	__asm {
		mov g_pClientState, ebx; // Loads the CClientState since it won't be passed as a argument(unlike linux) on windows.
	}

	VPROF_BUDGET("LoadingScreen - CClientState::FinishSignonState_New", VPROF_BUDGETGROUP_GAME);

	Msg("Called CClientState::FinishSignonState_New!\n");

	if (g_pClientState->m_nSignonState != SIGNONSTATE_NEW)
		return;

	if ( !g_pClientState->m_bMarkedCRCsUnverified )
	{
		// Mark all file CRCs unverified once per server. We may have verified CRCs for certain files on
		// the previous server, but we need to reverify them on the new server.
		g_pClientState->m_bMarkedCRCsUnverified = true;
		g_pFullFileSystem->MarkAllCRCsUnverified();
	}

	if ( !func_CL_CheckCRCs( g_pClientState->m_szLevelFileName ) )
	{
		Error( "Unable to verify map %s", g_pClientState->m_szLevelFileName[0] ? g_pClientState->m_szLevelFileName : "unknown" );
		return;
	}

	if ( g_pBaseServer->m_State < ss_loading )
	{
		// Reset the last used count on all models before beginning the new load -- The nServerCount value on models should
		// always resolve as different from values from previous servers.
		modelloader->ResetModelServerCounts();
	}

	ConVarRef cl_always_flush_models("cl_always_flush_models");
	if ( cl_always_flush_models.GetBool() )
		modelloader->PurgeUnusedModels();

	g_pClientState->SetModel(1);

	g_pClientState->m_bCheckCRCsWithServer = false;

	func_CL_CheckForPureServerWhitelist( g_pClientState->m_pPendingPureFileReloads );

	if (true) // true = threaded
	{
		g_pMainThreads->QueueCall(Threaded_ClientLoading);
		WaitForFinish();
	} else
		Threaded_ClientLoading();

	if (ShouldCancelLoading())
		return;

	func_R_LevelInit(); // Can't move this since else we crash randomly

	EngineVGui()->UpdateProgressBar(PROGRESS_SENDCLIENTINFO); // <--- Liar, we didn't send it yet XD
	if ( !g_pClientState->m_NetChannel )
		return;

	g_pClientState->SendClientInfo();

	func_CL_SetSteamCrashComment();

	// tell server that we entered now that state
	auto msg = NET_SignonState( g_pClientState->m_nSignonState, g_pClientState->m_nServerCount );
	g_pClientState->m_NetChannel->SendNetMsg( msg );
}

static bool g_bLoadingModelloader = false;
static Detouring::Hook detour_CModelLoader_DebugPrintDynamicModels;
/*
 * __fastcall took me an hour to figure out since the compiler did funnies.
 * it nuked the ecx register making me believe I was always wrong,
 * but in the end I figured it out when I looked at the compiled asm.
 * asm is fun.
 */
static void __fastcall hook_CModelLoader_DebugPrintDynamicModels()
{
	__asm {
		mov modelloader, ecx; // Loads the modelloader since i'm too dumb to make a signature to load it with. (Tried 5 different ones, all failed)
	}

	if (!g_bLoadingModelloader)
		detour_CModelLoader_DebugPrintDynamicModels.GetTrampoline<Symbols::CModelLoader_DebugPrintDynamicModels>()(modelloader);
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

	startParams.nThreads = 1;

	g_pMainThreads = V_CreateThreadPool();
	g_pMainThreads->Start(startParams);

	g_pFullFileSystem = InterfacePointers::FileSystemClient();
	g_pBaseServer = (CBaseServer*)InterfacePointers::Server();
	g_pCVar = InterfacePointers::Cvar();
	engineclient = InterfacePointers::VEngineClient();

	SourceSDK::FactoryLoader engine_loader("engine");
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

	Detour::Create(
		&detour_CModelLoader_DebugPrintDynamicModels, "CL_CModelLoader_DebugPrintDynamicModels",
		engine_loader.GetModule(), Symbols::CModelLoader_DebugPrintDynamicModelsSym,
		(void*)hook_CModelLoader_DebugPrintDynamicModels
	);

	func_CL_CheckCRCs = (Symbols::CL_CheckCRCs)Detour::GetFunction(engine_loader.GetModule(), Symbols::CL_CheckCRCsSym);
	Detour::CheckFunction((void*)func_CL_CheckCRCs, "CL_CheckCRCs");

	func_CL_CheckForPureServerWhitelist = (Symbols::CL_CheckForPureServerWhitelist)Detour::GetFunction(engine_loader.GetModule(), Symbols::CL_CheckForPureServerWhitelistSym);
	Detour::CheckFunction((void*)func_CL_CheckForPureServerWhitelist, "CL_CheckForPureServerWhitelist");

	func_CL_RegisterResources = (Symbols::CL_RegisterResources)Detour::GetFunction(engine_loader.GetModule(), Symbols::CL_RegisterResourcesSym);
	Detour::CheckFunction((void*)func_CL_RegisterResources, "CL_RegisterResources");

	func_CL_SetSteamCrashComment = (Symbols::CL_SetSteamCrashComment)Detour::GetFunction(engine_loader.GetModule(), Symbols::CL_SetSteamCrashCommentSym);
	Detour::CheckFunction((void*)func_CL_SetSteamCrashComment, "CL_SetSteamCrashComment");

	func_R_LevelInit = (Symbols::R_LevelInit)Detour::GetFunction(engine_loader.GetModule(), Symbols::R_LevelInitSym);
	Detour::CheckFunction((void*)func_R_LevelInit, "R_LevelInit");

	func_CClientState_ConsistencyCheck = (Symbols::CClientState_ConsistencyCheck)Detour::GetFunction(engine_loader.GetModule(), Symbols::CClientState_ConsistencyCheckSym);
	Detour::CheckFunction((void*)func_CClientState_ConsistencyCheck, "CClientState::ConsistencyCheck");

	func_SendTable_ComputeCRC = (Symbols::SendTable_ComputeCRC)Detour::GetFunction(engine_loader.GetModule(), Symbols::SendTable_ComputeCRCSym);
	Detour::CheckFunction((void*)func_SendTable_ComputeCRC, "SendTable_ComputeCRC");

	func_CClientState_AddCustomFile = (Symbols::CClientState_AddCustomFile)Detour::GetFunction(engine_loader.GetModule(), Symbols::CClientState_AddCustomFileSym);
	Detour::CheckFunction((void*)func_CClientState_AddCustomFile, "CClientState::AddCustomFile");

	func_V_RenderVGuiOnly = (Symbols::V_RenderVGuiOnly)Detour::GetFunction(engine_loader.GetModule(), Symbols::V_RenderVGuiOnlySym);
	Detour::CheckFunction((void*)func_V_RenderVGuiOnly, "V_RenderVGuiOnly");

	func_ClientDLL_ProcessInput = (Symbols::ClientDLL_ProcessInput)Detour::GetFunction(engine_loader.GetModule(), Symbols::ClientDLL_ProcessInputSym);
	Detour::CheckFunction((void*)func_ClientDLL_ProcessInput, "ClientDLL_ProcessInput");

	enginevgui_internal = engine_loader.GetInterface<IEngineVGuiInternal>(VENGINE_VGUI_VERSION);
	Detour::CheckValue("get IEngineVGuiInternal", enginevgui_internal);

	SourceSDK::FactoryLoader material_loader("materialsystem");
	materials = material_loader.GetInterface<IMaterialSystem>(MATERIAL_SYSTEM_INTERFACE_VERSION);
	Detour::CheckValue("get IMaterialSystem", materials);

	SourceSDK::FactoryLoader datacache_loader("datacache");
	mdlcache = datacache_loader.GetInterface<IMDLCache>(MDLCACHE_INTERFACE_VERSION);
	Detour::CheckValue("get IMDLCache", mdlcache);

	SourceSDK::FactoryLoader menusystem_loader("menusystem");
	g_pMenuSystem = menusystem_loader.GetInterface<IMenuSystem>(INTERFACEVERSION_MENUSYSTEM);
	Detour::CheckValue("get IMenuSystem", g_pMenuSystem);

	if (func_SendTable_ComputeCRC)
		g_pSendTableCRC = func_SendTable_ComputeCRC();

	ConCommand* cmd = g_pCVar->FindCommand("mod_dynamicmodeldebug");
	if (cmd)
	{
		g_bLoadingModelloader = true;
		engineclient->ExecuteClientCmd("mod_dynamicmodeldebug");
		g_bLoadingModelloader = false;
	} else {
		Detour::g_bDetourError = true;
	}

	if (Detour::g_bDetourError)
	{
		Detour::Remove(0);
		Detour::ReportLeak();
		Warning("loadingscreen: Dll failed initialization, removing all detours to prevent unexpected crashes.\n");
	}

	return 0;
}

GMOD_MODULE_CLOSE()
{
	Detour::Remove(0);
	Detour::ReportLeak();

	g_pFullFileSystem = NULL;

	g_pWorkerThreads->Stop();
	V_DestroyThreadPool(g_pWorkerThreads);

	g_pMainThreads->Stop();
	V_DestroyThreadPool(g_pMainThreads);

	return 0;
}