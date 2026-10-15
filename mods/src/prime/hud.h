#pragma once

// The HUD around the space views: the mission buttons, the faction and system panel, the alliance drawer, and the
// galactic anomaly the viewed system may have.

#include "prime/ui.h"
#include "prime/unity.h"

#include <il2cpp/il2cpp.h>

#include <cstdint>

// The Q-Trials, field training, outposts, daily goals and missions buttons.
struct MissionsHudViewController : il2cpp::Object {
  using Class = il2cpp::Class<"Assembly-CSharp", "Digit.Prime.HUD", "MissionsHudViewController">;

  // The buttons are a mix of Button and GenericButtonWidget fields; only their GameObject matters to the mod.
  template <il2cpp::Name Field> Component* ButtonField()
  { return this->template Field<Field, Component*>(); }
};

struct FactionInformation : il2cpp::Object {
  using Class = il2cpp::Class<"Assembly-CSharp", "Digit.Prime.HUD", "FactionInformation">;

  // TextLocalizer components, one per system name label.
  il2cpp::List<Component*>* systemName()
  { return Field<"_systemName", il2cpp::List<Component*>*>(); }
};

// Timer data contexts, galactic anomalies and navigation view data are read on their runtime classes: the mod only
// needs a few members, and these classes have moved between client builds.
struct TimerDataContext : il2cpp::Object {
  TimeSpan RemainingTime()
  { return Get<"RemainingTime", TimeSpan>(); }
};

// Digit.PrimeServer.Models.GalacticAnomaly
struct GalacticAnomaly : il2cpp::Object {
  bool IsActive()
  { return Get<"IsActive", bool>(); }
  TimerDataContext* EndTimeTimerDataContext()
  { return Get<"EndTimeTimerDataContext", TimerDataContext*>(); }
};

struct NodeAddress : il2cpp::Object {
  int64_t System()
  { return Get<"System", int64_t>(); }
};

struct NavigationViewData : il2cpp::Object {
  NodeAddress* ViewingAddress()
  { return Get<"ViewingAddress", NodeAddress*>(); }
};

struct HudNavigationViewController : Component {
  using Class = il2cpp::Class<"Assembly-CSharp", "Digit.Prime.Navigation.UI", "HudNavigationViewController">;

  Component* galacticAnomaliesButtonWidget()
  { return Field<"_galacticAnomaliesButtonWidget", Component*>(); }
  FactionInformation* factionInformationHelper()
  { return Field<"_factionInformationHelper", FactionInformation*>(); }
  bool isActiveAndEnabled()
  { return Get<"isActiveAndEnabled", bool>(); }
  bool CanShowGalacticAnomaly()
  { return Call<"CanShowGalacticAnomaly", bool>(); }
  NavigationViewData* CanvasContext()
  { return Get<"CanvasContext", NavigationViewData*>(); }
};

// The upper-right drawer with Help All, Engage and Claim.
struct HudAllianceAndNewsViewController : il2cpp::Object {
  using Class = il2cpp::Class<"Assembly-CSharp", "Digit.Prime.HUD", "HudAllianceAndNewsViewController">;

  Component* chestPromotion()
  { return Field<"_chestPromotion", Component*>(); }
};

struct GalacticAnomaliesManager : MonoSingleton<GalacticAnomaliesManager> {
  using Class = il2cpp::Class<"Assembly-CSharp", "Digit.Prime.GalacticAnomalies", "GalacticAnomaliesManager">;

  // Instance is only read once the singleton is up, so a missing manager never triggers the game's own search.
  static GalacticAnomaliesManager* Current()
  { return Class::Get<"Initialized", bool>() ? Instance() : nullptr; }

  GalacticAnomaly* GetSystemGalacticAnomalies(int64_t system)
  { return Call<"GetSystemGalacticAnomalies", GalacticAnomaly*>(system); }
};
