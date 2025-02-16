#include <GarrysMod/Symbol.hpp>
#include "vector"

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
	typedef void (GMCOMMON_CALLING_CONVENTION *CClientState_FinishSignonState_New)(void* cclientState);
	extern const std::vector<Symbol> CClientState_FinishSignonState_NewSym;

	typedef void (GMCOMMON_CALLING_CONVENTION *CL_InstallAndInvokeClientStringTableCallbacks)();
	extern const std::vector<Symbol> CL_InstallAndInvokeClientStringTableCallbacksSym;
}