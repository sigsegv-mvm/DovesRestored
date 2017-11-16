/**
 * SM Extension: DovesRestored
 * Written by sigsegv
 *
 * Distributed under the terms of the Simplified BSD License
 * (see LICENSE file for details)
 */

#include "extension.h"
#include <ISDKHooks.h>


DovesRestored g_Extension;
SMEXT_LINK(&g_Extension);

/* Source interface ptrs */
ICvar *icvar = nullptr;

/* SM interface ptrs */
ISDKHooks *g_pSDKHooks = nullptr;

/* gamedata */
IGameConfig *g_pGameConf = nullptr;
void *fptr_CEntityBird_SpawnRandomBirds = nullptr;
void *vtptr_CTFGameRules = nullptr;
int vtidx_CTFGameRules_SetupOnRoundStart = -1;

/* hooks */
SH_DECL_MANUALHOOK0_void(MHook_SetupOnRoundStart, 0, 0, 0);
int hookid_SetupOnRoundStart = 0;

/* convars */
ConVar *mp_tournament = nullptr;


static void Hook_SetupOnRoundStart()
{
	/* we would ideally call TFGameRules()->IsInTournamentMode() here, but it's easier to just check the convar directly */
	if (mp_tournament == nullptr || !mp_tournament->GetBool()) {
		g_pSM->LogMessage(myself, "Spawning doves.");
		(*reinterpret_cast<void (*)()>(fptr_CEntityBird_SpawnRandomBirds))();
	}
}


bool DovesRestored::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "sdkhooks.ext", true, true);
	
	if (!gameconfs->LoadGameConfigFile("DovesRestored", &g_pGameConf, error, maxlength)) {
		return false;
	}
	
	if (!g_pGameConf->GetMemSig("CEntityBird::SpawnRandomBirds", &fptr_CEntityBird_SpawnRandomBirds)) {
		snprintf(error, maxlength, "Failed to find function CEntityBird::SpawnRandomBirds");
		return false;
	}
	
	if (!g_pGameConf->GetMemSig("vtable for CTFGameRules", &vtptr_CTFGameRules)) {
		snprintf(error, maxlength, "Failed to find vtable of CTFGameRules");
		return false;
	} else {
		*reinterpret_cast<uintptr_t *>(&vtptr_CTFGameRules) += 0x8;
	}
	
	if (!g_pGameConf->GetOffset("CTFGameRules::SetupOnRoundStart", &vtidx_CTFGameRules_SetupOnRoundStart) || vtidx_CTFGameRules_SetupOnRoundStart < 0) {
		snprintf(error, maxlength, "Failed to find VT offset of CTFGameRules::SetupOnRoundStart");
		return false;
	} else {
		SH_MANUALHOOK_RECONFIGURE(MHook_SetupOnRoundStart, vtidx_CTFGameRules_SetupOnRoundStart, 0, 0);
	}
	
	hookid_SetupOnRoundStart = SH_ADD_MANUALDVPHOOK(MHook_SetupOnRoundStart, vtptr_CTFGameRules, SH_STATIC(Hook_SetupOnRoundStart), true);
	if (hookid_SetupOnRoundStart == 0) {
		snprintf(error, maxlength, "Failed to set up global SourceHook post-hook for CTFGameRules::SetupOnRoundStart");
		return false;
	}
	
	return true;
}

void DovesRestored::SDK_OnUnload()
{
	if (hookid_SetupOnRoundStart != 0) {
		SH_REMOVE_HOOK_ID(hookid_SetupOnRoundStart);
	}
	
	gameconfs->CloseGameConfigFile(g_pGameConf);
}

void DovesRestored::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(SDKHOOKS, g_pSDKHooks);
}

bool DovesRestored::QueryRunning(char *error, size_t maxlength)
{
	SM_CHECK_IFACE(SDKHOOKS, g_pSDKHooks);
	
	return true;
}

bool DovesRestored::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	
	mp_tournament = icvar->FindVar("mp_tournament");
	
	return true;
}
