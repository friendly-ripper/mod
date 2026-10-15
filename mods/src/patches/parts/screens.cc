// Shortcuts that open game screens, press the buttons on them, and step through them: the show_* navigation
// shortcuts, Ship Swap and the officer swap and preset buttons, the arrow keys on docks, artifacts and selectors, and
// focusing the search box.

#include "config.h"
#include "patches/patches.h"
#include "patches/shortcuts.h"

#include "prime/Hub.h"
#include "prime/fleet.h"
#include "prime/inventory.h"
#include "prime/navigation.h"
#include "patches/object_tracker.h"
#include "prime/officers.h"
#include "prime/ui.h"

#include <il2cpp/il2cpp.h>
#include <spdlog/spdlog.h>

namespace
{
// Ship Swap and Officer Swap live on the Manage Ships screen, and Officer Presets on the officer assignment screen
// that Officer Swap opens. Their shortcuts open those screens first, then press the button once it exists. 300
// frames is a few seconds at the game's frame rate.
constexpr int kButtonWaitFrames = 300;

struct Waits {
  int  ship_swap         = 0;
  int  officer_swap      = 0;
  int  officer_presets   = 0;
  bool assignment_opened = false;
} waits;

// Runs one of the game's own shortcut handlers. A handler missing from this client is reported once by the binding
// layer and does nothing.
template <il2cpp::Name Action> void Native()
{
  if (auto* manager = ShortcutsManager::Instance())
    manager->Run<Action>();
  else
    spdlog::warn("[Hotkeys] native {} shortcut is unavailable", Action.View());
}

template <SectionID Section> void Goto()
{ Hub::GotoSection(Section); }

// Galaxy and system views keep their state in section storage; reuse it so the camera returns where it was.
template <SectionID Section> void GotoNavigation()
{
  auto* manager = Hub::Sections();
  auto* storage = manager ? manager->sectionStorage() : nullptr;
  if (auto* state = storage ? storage->GetState(Section) : nullptr)
    Hub::GotoSection(Section, state);
  else
    NavigationSectionManager::ChangeNavigationSection(Section);
}

void ShowBookmarks()
{
  if (auto* bookmarks = BookmarksManager::Instance())
    bookmarks->ViewBookmarks();
  else
    Hub::GotoSection(SectionID::Bookmarks_Main);
}

void ShowCoordinateSearch()
{
  if (auto* bookmarks = BookmarksManager::Instance())
    return bookmarks->ViewCoordinateSearch();
  spdlog::warn("[ShowLookup] BookmarksManager instance not available, falling back to main bookmarks");
  Hub::GotoSection(SectionID::Bookmarks_Main);
}

void ShowHaven()
{ PlanetaryBaseManager::ViewOwnHaven(); }

// Presses the button `pick` returns on the first visible, active Controller.
template <typename Controller> bool PressButton(Button* (*pick)(Controller*))
{
  for (auto* controller : ObjectFinder<Controller>::GetAll()) {
    auto* button =
        controller && CanvasController::Shows(controller) && controller->isActiveAndEnabled() ? pick(controller) : nullptr;
    if (button) {
      button->Press();
      return true;
    }
  }
  return false;
}

bool PressShipSwapButton()
{
  return PressButton<ShipManagementScreenStateViewController>(
      [](ShipManagementScreenStateViewController* controller) { return controller->swapShipButton(); });
}

bool PressOfficerSwapButton()
{
  return PressButton<ShipManagementScreenStateViewController>(
      [](ShipManagementScreenStateViewController* controller) { return controller->officerIconButton(); });
}

bool PressOfficerPresetsButton()
{
  return PressButton<OfficerAssignmentViewController>([](OfficerAssignmentViewController* controller) -> Button* {
    auto* widget = controller->officerPresetsButton();
    return widget ? widget->TheButton() : nullptr;
  });
}

void OpenManageShips()
{
  auto* fleet_bar = ObjectFinder<FleetBarViewController>::Get();
  if (auto* fleet = fleet_bar ? fleet_bar->SelectedFleet() : nullptr)
    fleet_bar->fleetPanelController()->RequestAction(fleet, ActionType::Manage, 0, ActionBehaviour::Default);
}

// Opens Manage Ships for the selected fleet, for a button that is only there. False, with a warning, without one.
bool OpenManageShipsFor(const char* what)
{
  auto* fleet_bar = ObjectFinder<FleetBarViewController>::Get();
  if (!fleet_bar || !fleet_bar->SelectedFleet()) {
    spdlog::warn("[Hotkeys] unable to open Manage Ships for {}", what);
    return false;
  }
  OpenManageShips();
  return true;
}

void ShowShipSwap()
{
  if (!PressShipSwapButton() && OpenManageShipsFor("Ship Swap"))
    waits.ship_swap = kButtonWaitFrames;
}

void ShowOfficerSwap()
{
  if (!PressOfficerSwapButton() && OpenManageShipsFor("Officer Swap"))
    waits.officer_swap = kButtonWaitFrames;
}

void ShowOfficerPresets()
{
  if (PressOfficerPresetsButton())
    return;
  waits.assignment_opened = PressOfficerSwapButton();
  if (waits.assignment_opened || OpenManageShipsFor("Officer Presets"))
    waits.officer_presets = kButtonWaitFrames;
}

struct ScreenShortcut {
  Action shortcut;
  void (*run)();
  // Opening Manage Ships lets the game see the key too.
  bool consumes = true;
};

// Checked in order; the first pressed one runs.
constexpr ScreenShortcut kScreens[] = {
    {Action::ShowQTrials, Goto<SectionID::ChallengeSelection>},
    {Action::ShowBookmarks, ShowBookmarks},
    {Action::ShowLookup, ShowCoordinateSearch},
    {Action::ShowRefinery, Goto<SectionID::Shop_Refining_List>},
    {Action::ShowFactions, Goto<SectionID::Shop_MainFactions>},
    {Action::ShowStationExterior, Goto<SectionID::Starbase_Exterior>},
    {Action::NativeShortcutGalaxy, Native<"OnGalaxyAction">},
    {Action::ShowGalaxy, GotoNavigation<SectionID::Navigation_Galaxy>},
    {Action::ShowStationInterior, Goto<SectionID::Starbase_Interior>},
    {Action::ShowHaven, ShowHaven},
    {Action::ShowSystem, GotoNavigation<SectionID::Navigation_System>},
    {Action::ShowArtifacts, Goto<SectionID::ArtifactHall_Inventory>},
    {Action::ShowShipConstruction, Native<"OnShipsAction">},
    {Action::ShowOfficerSwap, ShowOfficerSwap},
    {Action::ShowShipSwap, ShowShipSwap},
    {Action::ShowShields, Native<"OnPeaceShieldAction">},
    {Action::ShowBattlelogs, Native<"OnInboxAction">},
    {Action::ShowInventory, Goto<SectionID::InventoryList>},
    {Action::ShowMissions, Goto<SectionID::Missions_AcceptedList>},
    {Action::ShowResearch, Goto<SectionID::Research_LandingPage>},
    {Action::ShowScrapYard, Goto<SectionID::ShipScrapping_List>},
    {Action::ShowOfficers, Goto<SectionID::OfficerInventory>},
    {Action::ShowOfficerPresets, ShowOfficerPresets},
    // TODO: opens the first fleet commander (Spock), not the selected fleet's.
    {Action::ShowCommander, Goto<SectionID::FleetCommander_Management>},
    {Action::ShowAwayTeam, Goto<SectionID::Missions_AwayTeamsList>},
    {Action::NativeShortcutEvents, Native<"OnEventsAction">},
    {Action::ShowEvents, Goto<SectionID::Tournament_Group_Selection>},
    {Action::ShowExoComp, Goto<SectionID::Consumables>},
    {Action::ShowDaily, Goto<SectionID::Missions_DailyGoals>},
    {Action::ShowGifts, Goto<SectionID::Shop_List>},
    {Action::ShowAlliance, Goto<SectionID::Alliance_Main>},
    {Action::ShowAllianceHelp, Goto<SectionID::Alliance_Help>},
    {Action::ShowAllianceArmada, Goto<SectionID::Alliance_Armadas>},
    {Action::ShowSettings, Goto<SectionID::GameSettings>},
    {Action::ShowShips, OpenManageShips, false},
};

// Left and right step through docks, ship lists, artifacts and officer pages on the screens that have them,
// before they pan the camera. Returns true when something moved.
bool StepScreen(bool left)
{
  const auto& config = Config::Get();

  if (config.ui.arrow_keys_to_select_ship && Hub::CurrentSection() == SectionID::ShipManagement_Details) {
    bool moved = false;
    for (auto* controller : ObjectFinder<ShipManagementViewController>::GetAll()) {
      auto* context = controller && controller->isActiveAndEnabled() && CanvasController::Shows(controller)
                          ? controller->Context()
                          : nullptr;
      if (!context)
        continue;
      const int32_t count = context->ListCount();
      const int32_t next  = context->CurrentIndex() + (left ? -1 : 1);
      if (count <= 1 || context->CurrentIndex() < 0 || next < 0 || next >= count)
        continue;
      context->SetLastMovementWasToIncrement(!left);
      context->SetCurrentIndex(next);
      moved = true;
    }
    if (moved)
      return true;
  }

  if (config.ui.arrow_keys_to_select_ship) {
    bool moved = false;
    for (auto* widget : ObjectFinder<AssignShipsWidget>::GetAll()) {
      auto* list = widget && CanvasController::Shows(widget) && widget->isActiveAndEnabled() ? widget->selectableList()
                                                                                              : nullptr;
      if (!list || list->Count() <= 0)
        continue;
      // Skip empty dock slots.
      const int32_t count = list->Count();
      int32_t       index = list->SelectedIndex();
      do
        index += left ? -1 : 1;
      while (index >= 0 && index < count
             && !(list->DataItem(index) && static_cast<FleetPlayerData*>(list->DataItem(index))->HasShip()));
      if (index < 0 || index >= count)
        continue;
      list->RequestSelect(index);
      moved = true;
    }
    if (moved)
      return true;
  }

  bool moved = false;
  for (auto* controller : ObjectFinder<ArtifactHallDetailsViewController>::GetAll()) {
    if (!controller || !controller->isActiveAndEnabled())
      continue;
    left ? controller->PressLeftArrow() : controller->PressRightArrow();
    moved = true;
  }
  if (moved)
    return true;

  for (auto* selector : ObjectFinder<ElementSelectorViewController>::GetAll()) {
    if (!selector || !selector->isActiveAndEnabled())
      continue;
    left ? selector->PressDecrement() : selector->PressIncrement();
    moved = true;
  }
  return moved;
}

// Focuses the search box of whichever inventory, officer or ship list is on screen.
bool FocusSearchBox()
{
  const auto focus = [](auto* owner) {
    auto* field = owner ? owner->inputField() : nullptr;
    if (!field || !CanvasController::Shows(owner) || !owner->isActiveAndEnabled() || !field->isActiveAndEnabled())
      return false;
    field->Focus();
    return true;
  };
  // InventoryListViewController also covers the officer roster and the other inventory screens.
  for (auto* controller : ObjectFinder<InventoryListViewController>::GetAll())
    if (focus(controller))
      return true;
  for (auto* controller : ObjectFinder<OfficerAssignmentViewController>::GetAll())
    if (focus(controller))
      return true;
  for (auto* widget : ObjectFinder<AssignShipsWidget>::GetAll())
    if (focus(widget))
      return true;
  return false;
}
} // namespace

bool PressAwaitedButton()
{
  if (waits.officer_presets > 0) {
    if (PressOfficerPresetsButton()) {
      waits.officer_presets   = 0;
      waits.assignment_opened = false;
      return true;
    }
    // Manage Ships is up but officer assignment not yet: open it on the way.
    if (!waits.assignment_opened && PressOfficerSwapButton())
      waits.assignment_opened = true;
    if (--waits.officer_presets == 0) {
      waits.assignment_opened = false;
      spdlog::warn("[Hotkeys] timed out waiting for the Officer Presets button");
    }
  }

  if (waits.officer_swap > 0) {
    if (PressOfficerSwapButton()) {
      waits.officer_swap = 0;
      return true;
    }
    if (--waits.officer_swap == 0)
      spdlog::warn("[Hotkeys] timed out waiting for the Officer Swap button");
  }

  if (waits.ship_swap > 0) {
    if (PressShipSwapButton()) {
      waits.ship_swap = 0;
      return true;
    }
    if (--waits.ship_swap == 0)
      spdlog::warn("[Hotkeys] timed out waiting for the Ship Swap button");
  }
  return false;
}

bool ScreenShortcuts()
{
  if (Shortcut::IsDown(Action::MoveLeft) && StepScreen(true))
    return true;
  if (Shortcut::IsDown(Action::MoveRight) && StepScreen(false))
    return true;
  if (Config::Get().ui.focus_search && Shortcut::IsDown(Action::FocusSearch) && FocusSearchBox())
    return true;
  for (const auto& screen : kScreens) {
    if (Shortcut::IsDown(screen.shortcut)) {
      screen.run();
      return screen.consumes;
    }
  }
  return false;
}
