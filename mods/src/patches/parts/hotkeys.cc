// The mod's frame tick and the order its keyboard shortcuts are handled in. ScreenManager.Update runs once per
// frame: it runs the features that poll, then the shortcut that was pressed. Returning without calling the game's
// Update swallows the key, so the game's own handling of it does not run as well.
//
// The shortcuts themselves live with their features: screens.cc, fleet_actions.cc, chat.cc, camera.cc. This file
// keeps the mode switches, restart and quit, Escape, and the shortcuts that change a setting.

#include "config.h"
#include "patches/patches.h"
#include "patches/input.h"
#include "patches/shortcuts.h"

#include "prime/Hub.h"
#include "prime/ui.h"
#include "prime/fleet.h"
#include "patches/object_tracker.h"

#include <il2cpp/il2cpp.h>
#include <spdlog/spdlog.h>
#include <spud/detour.h>

#include <chrono>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace
{
using Clock = std::chrono::steady_clock;

bool hotkeys_installed = false;

template <auto Section, auto Setting> void Toggle()
{
  auto& section    = Config::Get().*Section;
  section.*Setting = !(section.*Setting);
}

template <spdlog::level::level_enum Level> void SetLogLevel()
{
  spdlog::set_level(Level);
  spdlog::flush_on(Level);
}

template <void (*Adjust)(bool), bool Up> void Scale()
{ Adjust(Up); }

void ReloadSettings()
{ Config::Reload(); }

struct SettingShortcut {
  Action shortcut;
  void (*run)();
  // Held shortcuts repeat every frame while the keys are down; the rest fire once per press.
  bool held = false;
};

// Shortcuts that change a setting. The game sees their keys too. The first pressed one runs.
constexpr SettingShortcut kSettings[] = {
    {Action::UiScaleUp, Scale<AdjustUiScale, true>, true},
    {Action::UiScaleDown, Scale<AdjustUiScale, false>, true},
    {Action::UiShipScaleUp, Scale<AdjustUiShipScale, true>, true},
    {Action::UiShipScaleDown, Scale<AdjustUiShipScale, false>, true},
    {Action::UiViewerScaleUp, Scale<AdjustUiViewerScale, true>, true},
    {Action::UiViewerScaleDown, Scale<AdjustUiViewerScale, false>, true},
    {Action::ToggleAutoConfirmInstantWarp, CycleInstantWarpConfirmation},
    {Action::TogglePreviewLocate, Toggle<&Config::ui, &Config::Ui::disable_preview_locate>},
    {Action::TogglePreviewRecall, Toggle<&Config::ui, &Config::Ui::disable_preview_recall>},
    {Action::ToggleCargoDefault, Toggle<&Config::ui, &Config::Ui::show_cargo_default>},
    {Action::ToggleCargoPlayer, Toggle<&Config::ui, &Config::Ui::show_player_cargo>},
    {Action::ToggleCargoStation, Toggle<&Config::ui, &Config::Ui::show_station_cargo>},
    {Action::ToggleCargoHostile, Toggle<&Config::ui, &Config::Ui::show_hostile_cargo>},
    {Action::ToggleCargoArmada, Toggle<&Config::ui, &Config::Ui::show_armada_cargo>},
    {Action::LogLevelOff, SetLogLevel<spdlog::level::off>},
    {Action::LogLevelError, SetLogLevel<spdlog::level::err>},
    {Action::LogLevelWarn, SetLogLevel<spdlog::level::warn>},
    {Action::LogLevelInfo, SetLogLevel<spdlog::level::info>},
    {Action::LogLevelDebug, SetLogLevel<spdlog::level::debug>},
    {Action::LogLevelTrace, SetLogLevel<spdlog::level::trace>},
    {Action::ReloadSettings, ReloadSettings},
};

void SettingShortcuts()
{
  for (const auto& setting : kSettings) {
    if (setting.held ? Shortcut::IsPressed(setting.shortcut) : Shortcut::IsDown(setting.shortcut)) {
      setting.run();
      return;
    }
  }
}

// With disable_escape_exit, Escape no longer reaches the game's exit prompt. disable_escape_exit_timer > 0 lets
// a double tap within that many milliseconds through. Returns true when Escape is swallowed.
bool SwallowEscape()
{
  const auto& config = Config::Get();
  if (!config.ui.disable_escape_exit || !Key::Pressed(KeyCode::Escape))
    return false;
  // A held key stays swallowed; only distinct presses count towards a double tap.
  if (config.ui.disable_escape_exit_timer <= 0 || !Key::Down(KeyCode::Escape))
    return true;

  static Clock::time_point first_press{};
  const auto               now = Clock::now();
  if (first_press == Clock::time_point{}
      || now - first_press > std::chrono::milliseconds(config.ui.disable_escape_exit_timer)) {
    first_press = now;
    return true;
  }
  // Consume the pair so a quick third press starts a new one.
  first_press = {};
  return false;
}

void RunFrameUpdates()
{
  // Each returns at once when its feature is off. They run in every hotkey mode, including Scopely's and while
  // hotkeys are toggled off. An exception must not unwind into the game's native frame.
  for (auto update : {Config::ReloadIfChanged, DockUpdate, MissionsUpdate, AnomalyTimerUpdate, InstantWarpUpdate}) {
    try {
      update();
    } catch (const std::exception& error) {
      spdlog::warn("[FrameUpdate] update failed: {}", error.what());
    } catch (...) {
      spdlog::warn("[FrameUpdate] update failed with an unknown exception");
    }
  }
}

// The shortcut dispatch for one frame. Returns true when the game's Update must not run, so it does not also
// handle the key.
bool HandleShortcuts()
{
  auto& config = Config::Get();

  if (Shortcut::IsDown(Action::DisableHotKeys)) {
    HideKeybindBadges();
    config.control.hotkeys_enabled = false;
    spdlog::warn("Setting hotkeys to DISABLED");
    return true;
  }
  if (Shortcut::IsDown(Action::EnableHotKeys)) {
    config.control.hotkeys_enabled = true;
    spdlog::warn("Setting hotkeys to ENABLED");
    return true;
  }

  if (PressAwaitedButton())
    return true;

  // Switched off, or with Scopely's (the game's own) hotkeys, the mod leaves the keys to the game. Returning true
  // here would skip the game's ScreenManager.Update, which also runs the back button, orientation and the rest of
  // the UI's per-frame work.
  if (!config.control.hotkeys_enabled || config.control.use_scopely_hotkeys)
    return false;

  // Keys typed into a text field (chat, search) are text, not shortcuts.
  const bool typing = Key::IsInputFocused();
  if (!typing && Shortcut::IsDown(Action::ToggleShortcutHints) && ToggleKeybindBadges())
    return true;

  if (!typing && Shortcut::IsDown(Action::Restart)) {
    spdlog::info("Clearing localisation cache and restarting");
    LanguageManager::ClearCache();
    Hub::App()->Reload();
    return true;
  }
  if (!typing && Shortcut::IsDown(Action::Quit)) {
#ifdef _WIN32
    TerminateProcess(GetCurrentProcess(), 1);
#else
    Hub::App()->Quit();
    return true;
#endif
  }

  if (SelectShipShortcuts())
    return true;

  // The game only clears a focused text field from its back button; Escape should too.
  const bool in_chat = Hub::IsInChat();
  if (Key::Pressed(KeyCode::Escape) && (typing || in_chat)) {
    Key::ClearInputFocus();
    return true;
  }

  if (in_chat) {
    if (ChatChannelShortcuts())
      return true;
  } else if (!typing) {
    if (FleetShortcuts())
      return true;
    OpenChatShortcut();
    if (ScreenShortcuts())
      return true;
    SettingShortcuts();
  }

  return FleetActionShortcuts() || SwallowEscape();
}

void UpdateManageShipDockSelection()
{
  static uint64_t last_fleet_id = 0;
  static auto next_update = std::chrono::steady_clock::time_point{};
  const auto now = std::chrono::steady_clock::now();
  if (now < next_update)
    return;
  next_update = now + std::chrono::milliseconds(100);
  auto* sections = Hub::Sections();
  if (!Config::Get().ui.dock_syncmanagement || !sections
      || sections->CurrentSection() != SectionID::ShipManagement_Details) {
    last_fleet_id = 0;
    return;
  }
  for (auto* controller : ObjectFinder<ShipManagementViewController>::GetAll()) {
    if (!controller || !controller->isActiveAndEnabled())
      continue;
    auto* canvas = GetCanvasControllerFromComponent(controller);
    if (!canvas || !canvas->Visible())
      continue;
    auto* context = controller->Context();
    auto* fleet = context ? context->CurrentFleet() : nullptr;
    auto* manager = FleetsManager::Instance();
    if (!fleet || !fleet->HasShip() || !manager || !manager->HasFleetService())
      continue;
    const auto index = fleet->Index();
    if (index < 0)
      continue;
    // Exclude unassigned and synthetic fleets; management list order is not dock order.
    auto* dock_fleet = manager->GetFleetPlayerData(index);
    if (!dock_fleet || dock_fleet->Id() != fleet->Id())
      continue;
    if (last_fleet_id == fleet->Id())
      return;
    auto* bar = ObjectFinder<FleetBarViewController>::Get();
    if (!bar)
      return;
    if (!bar->IsIndexSelected(index)) {
      // Use the bar's selection flow to update both the highlight and fleet panel.
      bar->RequestSelect(index);
      if (!bar->IsIndexSelected(index))
        return; // Retry next update if native selection was blocked.
      spdlog::debug("[ManageShips] selected dock {} for fleet {}", index + 1, fleet->Id());
    }
    last_fleet_id = fleet->Id();
    return;
  }
  last_fleet_id = 0;
}

void UpdateExpiredAbilityButtons()
{
  static auto next_update = std::chrono::steady_clock::time_point{};
  const auto now = std::chrono::steady_clock::now();
  if (now < next_update)
    return;
  next_update = now + std::chrono::milliseconds(250);
  auto* bar = ObjectFinder<FleetBarViewController>::Get();
  auto* controller = bar ? bar->fleetPanelController() : nullptr;
  auto* fleet = controller ? controller->fleet() : nullptr;
  if (!fleet || (static_cast<int>(fleet->CurrentState()) & static_cast<int>(FleetState::CanActivateAbility)) == 0)
    return;
  auto* manager = ActivatedAbilityManager::Instance();
  auto* panel = controller->shipActionPanelLocalViewController();
  auto* canvas = panel ? GetCanvasControllerFromComponent(panel) : nullptr;
  if (!manager || !canvas || !canvas->Visible() || panel->HasShipAnActiveAbilityWithActiveEffect())
    return;
  auto* ids = panel->abilityIDs();
  auto* buttons = panel->buttons();
  if (!ids || !buttons)
    return;
  for (size_t index = 0; index < ids->size() && index < buttons->size(); ++index) {
    auto* button = (*buttons)[index];
    auto* context = button ? button->Context() : nullptr;
    if (!button || !button->isActiveAndEnabled() || !context || context->Interactable())
      continue;
    const auto ability_id = (*ids)[index];
    auto* timer = panel->GetRemainingCooldown(ability_id);
    // TimeSpan is an eight-byte value in 100 ns ticks. Repair at -1 second or below.
    if (timer && timer->RemainingTime().ticks <= -10000000 && manager->CanUseAbility(fleet, ability_id))
      context->SetInteractable(true);
  }
}

void ScreenManager_Update_Hook(auto original, ScreenManager* _this)
{
  Key::Poll();
  RunFrameUpdates();
  if (hotkeys_installed) {
    UpdateManageShipDockSelection();
    UpdateExpiredAbilityButtons();
  }
  if (hotkeys_installed && HandleShortcuts())
    return;
  original(_this);
}

// With Scopely hotkeys off, the game never registers its own input actions, so its shortcuts cannot fire.
void ShortcutsManager_InitializeActions_Hook(auto original, ShortcutsManager* _this)
{
  if (Config::Get().control.use_scopely_hotkeys)
    original(_this);
}
} // namespace

bool InstallFrameTick()
{
  static bool installed = false;
  if (!installed) {
    if (auto m = ScreenManager::Class::Method("Update")) {
      SPUD_STATIC_DETOUR(m.Pointer(), ScreenManager_Update_Hook);
      installed = true;
    }
  }
  return installed;
}

void InstallHotkeys()
{
  // The badges only make sense when the game's own input actions are under the mod's control.
  if (auto m = ShortcutsManager::Class::Method("InitializeActions")) {
    SPUD_STATIC_DETOUR(m.Pointer(), ShortcutsManager_InitializeActions_Hook);
    InstallKeybindBadges();
  }
  hotkeys_installed = InstallFrameTick();
  InstallFleetActions();
}
