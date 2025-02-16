#include "symbols.h"

static Symbol NULL_SIGNATURE = Symbol::FromSignature("");

namespace Symbols
{
	const std::vector<Symbol> CClientState_FinishSignonState_NewSym = { // Unabled to verify map %s\n - sub_100C2E70
		Symbol::FromSignature("\x55\x8B\xEC\x81\xEC\xD0\x00\x00\x00\x56\x8B\xF1\x83\xBE\x30\x01\x00\x00\x03"), // 55 8B EC 81 EC D0 00 00 00 56 8B F1 83 BE 30 01 00 00 03
	};

	const std::vector<Symbol> CL_InstallAndInvokeClientStringTableCallbacksSym = { // PureServerWhitelist -> CL_CheckForPureServerWhitelist(sub_100AF1F0) -> Find sub_100AF1F0 in sub_100C2E70 -> get the next sub_ below it. 
		Symbol::FromSignature("\x55\x8B\xEC\x83\xEC\x10\x8B*****\x53\x8B\x01\xFF\x50\x14"), //  55 8B EC 83 EC 10 8B ?? ?? ?? ?? ?? 53 8B 01 FF 50 14
	};
}