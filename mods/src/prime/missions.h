#pragma once

// The missions screen and its daily goals.

#include "prime/server.h"
#include "prime/unity.h"

#include <il2cpp/il2cpp.h>

// An ITournament from the missions screen's claimable dailies; today an EventModel. Read on its runtime class.
struct DailyGoal : il2cpp::Object {
  il2cpp::List<InventoryItem*>* factionPointRewards()
  { return Field<"_factionPointRewards", il2cpp::List<InventoryItem*>*>(); }
};

struct MissionViewController : il2cpp::Object {
  using Class = il2cpp::Class<"Assembly-CSharp", "Digit.Prime.Missions", "MissionViewController">;

  Toggle* claimAllToggle()
  { return Field<"_claimAllToggle", Toggle*>(); }
};
