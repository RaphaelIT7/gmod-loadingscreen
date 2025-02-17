#include <GarrysMod/Symbol.hpp>
#include "vector"
#include <filesystem.h>

#if defined SYSTEM_WINDOWS
#if defined ARCHITECTURE_X86_64
#define GMCOMMON_CALLING_CONVENTION __fastcall
#else
#define GMCOMMON_CALLING_CONVENTION __thiscall
#endif
#else
#define GMCOMMON_CALLING_CONVENTION
#endif

/*
 * The symbols will have this order:
 * 0 - Windows 32x
 * 1 - Windows 64x
 * 2 - Linux 32x
 * 3 - Linux 64x
 */

namespace Symbols
{
	typedef void (GMCOMMON_CALLING_CONVENTION *CClientState_FinishSignonState_New)(void* clientState);
	extern const std::vector<Symbol> CClientState_FinishSignonState_NewSym;

	typedef void (*CL_InstallAndInvokeClientStringTableCallbacks)();
	extern const std::vector<Symbol> CL_InstallAndInvokeClientStringTableCallbacksSym;

	typedef bool (*CL_CheckCRCs)(const char*);
	extern const std::vector<Symbol> CL_CheckCRCsSym;

	typedef void (*CL_SetSteamCrashComment)();
	extern const std::vector<Symbol> CL_SetSteamCrashCommentSym;

	typedef void (*CL_RegisterResources)();
	extern const std::vector<Symbol> CL_RegisterResourcesSym;

	typedef void (*CL_CheckForPureServerWhitelist)(IFileList *&pFilesToReload);
	extern const std::vector<Symbol> CL_CheckForPureServerWhitelistSym;

	typedef void (GMCOMMON_CALLING_CONVENTION *CModelLoader_DebugPrintDynamicModels)(void* modelloader);
	extern const std::vector<Symbol> CModelLoader_DebugPrintDynamicModelsSym;

	typedef void (*R_LevelInit)();
	extern const std::vector<Symbol> R_LevelInitSym;

	typedef void (GMCOMMON_CALLING_CONVENTION *CClientState_ConsistencyCheck)(void* clientState, bool bChanged);
	extern const std::vector<Symbol> CClientState_ConsistencyCheckSym;

	typedef CRC32_t (*SendTable_ComputeCRC)();
	extern const std::vector<Symbol> SendTable_ComputeCRCSym;

	typedef void (GMCOMMON_CALLING_CONVENTION *CClientState_AddCustomFile)(void* clientState, int slot, const char *resourceFile);
	extern const std::vector<Symbol> CClientState_AddCustomFileSym;

	typedef void (GMCOMMON_CALLING_CONVENTION *CClientState_SetModel)(void* clientState, int tableIndex);
	extern const std::vector<Symbol> CClientState_SetModelSym;

	typedef void (*V_RenderVGuiOnly)();
	extern const std::vector<Symbol> V_RenderVGuiOnlySym;

	typedef void (*ClientDLL_ProcessInput)();
	extern const std::vector<Symbol> ClientDLL_ProcessInputSym;
}