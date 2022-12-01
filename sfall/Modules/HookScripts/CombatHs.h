#pragma once

// Combat-related hook scripts

namespace sfall
{

void InitCombatHookScripts();

void Inject_ToHitHook();
void Inject_AfterHitRollHook();
void Inject_CalcApCostHook();
void Inject_CombatDamageHook();
void Inject_FindTargetHook();
void Inject_ItemDamageHook();
void Inject_AmmoCostHook();
void Inject_OnExplosionHook();
void Inject_TargetObjectHook();

int __fastcall AmmoCostHook_Script(DWORD hookType, fo::GameObject* weapon, DWORD &rounds);

long CalcApCostHook_Invoke(fo::GameObject* source, long hitMode, long isCalled, long cost, fo::GameObject* weapon);

}
