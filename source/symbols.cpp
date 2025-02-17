#include "symbols.h"

static Symbol NULL_SIGNATURE = Symbol::FromSignature("");

namespace Symbols
{
	const std::vector<Symbol> CClientState_FinishSignonState_NewSym = { // Unabled to verify map %s\n - sub_100C2E70
		Symbol::FromSignature("\x55\x8B\xEC\x81\xEC\xD0\x00\x00\x00\x56\x8B\xF1\x83\xBE\x30\x01\x00\x00\x03"), // 55 8B EC 81 EC D0 00 00 00 56 8B F1 83 BE 30 01 00 00 03
	};

	const std::vector<Symbol> CL_InstallAndInvokeClientStringTableCallbacksSym = { // PureServerWhitelist -> CL_CheckForPureServerWhitelist(sub_100AF1F0) -> Find sub_100AF1F0 in sub_100C2E70 -> get the next sub_ below it. 
		Symbol::FromSignature("\x55\x8B\xEC\x83\xEC\x10\x8B*****\x53\x8B\x01\xFF\x50\x14"), // 55 8B EC 83 EC 10 8B ?? ?? ?? ?? ?? 53 8B 01 FF 50 14
	};

	const std::vector<Symbol> CL_CheckCRCsSym = { // Map %s is missing - sub_100ADA30
		Symbol::FromSignature("\x55\x8B\xEC\x83\xEC\x20\x33\xC0******\x02\x56"), // 55 8B EC 83 EC 20 33 C0 ?? ?? ?? ?? ?? ?? 02 56
	};

	const std::vector<Symbol> CL_SetSteamCrashCommentSym = { // "-General-" - sub_100AFD80
		Symbol::FromSignature("\x55\x8B\xEC\xB8\x44\x15\x00\x00*****\x8B"), // 55 8B EC B8 44 15 00 00 ?? ?? ?? ?? ?? 8B
	};

	const std::vector<Symbol> CL_RegisterResourcesSym = { // CL_RegisterResources - sub_100AFC40
		Symbol::FromSignature("\x6A\x01\xB9*********\x50\xB9"), // 6A 01 B9 ?? ?? ?? ?? ?? ?? ?? ?? ?? 50 B9
	};

	const std::vector<Symbol> CL_CheckForPureServerWhitelistSym = { // PureServerWhitelist - sub_100ADD30 (check for a "cmp dword_xyz, 1" which is for this cl.m_nMaxClients <= 1)
		Symbol::FromSignature("\x55\x8B\xEC\x83\xEC\x34\x83*****\x01"), // 55 8B EC 83 EC 34 83 ?? ?? ?? ?? ?? 01
	};

	const std::vector<Symbol> CModelLoader_DebugPrintDynamicModelsSym = { // dynamic models - sub_1010F8B0
		Symbol::FromSignature("\x55\x8B\xEC\x83\xEC\x08\x53\x56\x57\x8B*****\x68"), // 55 8B EC 83 EC 08 53 56 57 8B ?? ?? ?? ?? ?? 68
	};

	const std::vector<Symbol> R_LevelInitSym = { // "Initializing renderer...\n" - sub_100F05D0
		Symbol::FromSignature("\x56\x57\x68****\xFF*****\x8B*****\x68"), // 56 57 68 ?? ?? ?? ?? FF ?? ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? 68
	};

	const std::vector<Symbol> CClientState_ConsistencyCheckSym = { // Server is enforcing model bounds - sub_100C2720
		Symbol::FromSignature("\x55\x8B\xEC\x81\xEC\x9C\x02\x00\x00\x53\x8B\xD9"), // 55 8B EC 81 EC 9C 02 00 00 53 8B D9
	};

	const std::vector<Symbol> SendTable_ComputeCRCSym = { // "SendTable_Init: called twice." - Idk, guess. Search for a function with loc_ & then search for -dti, before -dti two lines above there is a call to sub_, which is our target
		Symbol::FromSignature("\x55\x8B\xEC\x83\xEC\x2C\x8D\x45\xFC"), // 55 8B EC 83 EC 2C 8D 45 FC
	};

	const std::vector<Symbol> CClientState_AddCustomFileSym = { // CacheCustomFiles - sub_100C1EF0
		Symbol::FromSignature("\x55\x8B\xEC\x81\xEC\x3C\x03\x00\x00"), // 55 8B EC 81 EC 3C 03 00 00
	};

	/*
	 * reload
	 * SCR_BeginLoadingPlaque(sub_100FCBD0)(above push offset MultiByteStr | push 0)
	 * call sub_100FCD40 (called twive / only function that ocurrs multiple times in it)
	 * call sub_10137840 (idk, find it again. Have fun)
	 */
	const std::vector<Symbol> V_RenderVGuiOnlySym = {
		Symbol::FromSignature("\x8B*****\xD9*****\x51\xD9\x1C\x24\x8B\x01\xFF*****\xE8****\x8B\xC8\x8B\x10\xFF*****\x8B*****\x8B\x01\xFF\x10\x8B"), // 8B ?? ?? ?? ?? ?? D9 ?? ?? ?? ?? ?? 51 D9 1C 24 8B 01 FF ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B C8 8B 10 FF ?? ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? 8B 01 FF 10 8B
	};

	const std::vector<Symbol> ClientDLL_ProcessInputSym = { // "_Host_RunFrame_Input" -> Guess the function.
		Symbol::FromSignature("\x8B*****\x85\xC9**\x83*****\x02\x8B\x11"), //8B ?? ?? ?? ?? ?? 85 C9 ?? ?? 83 ?? ?? ?? ?? ?? 02 8B 11
	};
}