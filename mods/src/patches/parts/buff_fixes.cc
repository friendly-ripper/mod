// Buffs: use_out_of_dock_power, so the ship power shown in the dock matches the power out in space, and a guard
// against a buff list the game cannot handle.

#include "config.h"
#include "patches/patches.h"

#include "prime/server.h"

#include <il2cpp/il2cpp.h>
#include <spud/detour.h>

namespace
{
// Leaves out the buffs that only apply while docked at the station.
bool BuffService_IsBuffConditionMet_Hook(auto original, void* _this, BuffCondition currentCondition,
                                         IBuffComparer* buffComparer, IBuffData* buffToCompare,
                                         bool excludeFactionBuffs, bool isAllianceLoyalty)
{
  if (currentCondition == BuffCondition::CondSelfAtStation && Config::Get().buffs.use_out_of_dock_power)
    return false;
  return original(_this, currentCondition, buffComparer, buffToCompare, excludeFactionBuffs, isAllianceLoyalty);
}

// List<IBuffData> in and out, read untyped so a drifted IBuffData binding cannot make every buff look null. A list
// with a null buff crashes the game, so it is answered with no buffs instead.
il2cpp::List<il2cpp::Object*>* BuffService_ExtractBuffsOfType_Hook(auto original, ClientModifierType modifier,
                                                                   il2cpp::List<il2cpp::Object*>* list)
{
  if (list)
    for (auto* buff : *list)
      if (!buff)
        return nullptr;
  return original(modifier, list);
}
} // namespace

void InstallOutOfDockPower()
{
  if (auto method = BuffService::Method("IsBuffConditionMet"))
    SPUD_STATIC_DETOUR(method.Pointer(), BuffService_IsBuffConditionMet_Hook);
}

void InstallBuffCrashGuard()
{
  if (auto method = BuffService::Method("ExtractBuffsOfType"))
    SPUD_STATIC_DETOUR(method.Pointer(), BuffService_ExtractBuffsOfType_Hook);
}
