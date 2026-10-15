// Toast banners: hides the types listed in disabled_banner_types and mirrors the ones in notify_banner_types to
// Windows notifications, with a readable body for battle results.

#include "config.h"
#include "patches/parts/toast_text.h"
#include "patches/patches.h"
#include "str_utils.h"

#include "prime/fleet.h"
#include "prime/server.h"
#include "prime/ui.h"

#include <il2cpp/il2cpp.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <spud/detour.h>

#include <cstring>
#include <string>
#include <vector>

#if _WIN32
#include <windows.h>
#include <winrt/windows.data.xml.dom.h>
#include <winrt/windows.ui.notifications.h>
#endif

namespace
{
using toast_text::FormatPlaceholders;

constexpr std::pair<ToastState, const char*> kTitles[] = {
    {ToastState::Standard, "Notification"},
    {ToastState::FactionWarning, "Faction Warning"},
    {ToastState::FactionLevelUp, "Faction Level Up"},
    {ToastState::FactionLevelDown, "Faction Level Down"},
    {ToastState::FactionDiscovered, "Faction Discovered"},
    {ToastState::IncomingAttack, "Incoming Attack!"},
    {ToastState::IncomingAttackFaction, "Incoming Faction Attack!"},
    {ToastState::FleetBattle, "Fleet Battle"},
    {ToastState::StationBattle, "Station Under Attack!"},
    {ToastState::StationVictory, "Station Victory!"},
    {ToastState::Victory, "Victory!"},
    {ToastState::Defeat, "Defeat"},
    {ToastState::StationDefeat, "Station Defeat"},
    {ToastState::Tournament, "Event Progress"},
    {ToastState::ArmadaCreated, "Armada Created"},
    {ToastState::ArmadaCanceled, "Armada Canceled"},
    {ToastState::ArmadaIncomingAttack, "Armada Under Attack!"},
    {ToastState::ArmadaBattleWon, "Armada Victory!"},
    {ToastState::ArmadaBattleLost, "Armada Defeated"},
    {ToastState::DiplomacyUpdated, "Diplomacy Updated"},
    {ToastState::JoinedTakeover, "Territory Capture Joined"},
    {ToastState::CompetitorJoinedTakeover, "Competitor Joined Territory"},
    {ToastState::AbandonedTerritory, "Territory Abandoned"},
    {ToastState::TakeoverVictory, "Takeover Victory!"},
    {ToastState::TakeoverDefeat, "Takeover Defeat"},
    {ToastState::TreasuryProgress, "Treasury Progress"},
    {ToastState::TreasuryFull, "Treasury Full"},
    {ToastState::Achievement, "Achievement"},
    {ToastState::AssaultVictory, "Assault Victory!"},
    {ToastState::AssaultDefeat, "Assault Defeat"},
    {ToastState::ChallengeComplete, "Challenge Complete"},
    {ToastState::ChallengeFailed, "Challenge Failed"},
    {ToastState::StrikeHit, "Strike Hit"},
    {ToastState::StrikeDefeat, "Strike Defeat"},
    {ToastState::WarchestProgress, "Warchest Progress"},
    {ToastState::WarchestFull, "Warchest Full"},
    {ToastState::PartialVictory, "Partial Victory"},
    {ToastState::ArenaTimeLeft, "Arena Time Warning"},
    {ToastState::ChainedEventScored, "Event Progress"},
    {ToastState::FleetPresetApplied, "Fleet Preset Applied"},
    {ToastState::SurgeWarmUpEnded, "Surge Started"},
    {ToastState::SurgeHostileGroupDefeated, "Surge Hostiles Defeated"},
    {ToastState::SurgeTimeLeft, "Surge Time Warning"},
    {ToastState::QueueForLeaseActivated, "Queue Activated"},
    {ToastState::QueueForLeaseExpired, "Queue Expired"},
    {ToastState::PermanentQueuePurchased, "Permanent Queue Purchased"},
    {ToastState::OutpostStartedOrEnded, "Outpost Update"},
    {ToastState::CrossAllianceArmadaVictory, "Cross-Armada Victory!"},
    {ToastState::CrossAllianceArmadaDefeat, "Cross-Armada Defeated"},
    {ToastState::CrossAllianceArmadaPartialVictory, "Cross-Armada Partial Victory"},
    {ToastState::FactionWeeklyEventsProgress, "Faction Weekly Event Progress"},
    {ToastState::FactionWeeklyEventsComplete, "Faction Weekly Event Complete"},
    {ToastState::ArmadaPlayerBlocked, "Armada Player Blocked"},
    {ToastState::ArmadaPlayerUnblocked, "Armada Player Unblocked"},
    {ToastState::DynamicCrisisUpdate, "Dynamic Crisis Update"},
    {ToastState::DynamicCrisisFailed, "Dynamic Crisis Failed"},
    {ToastState::DynamicCrisisCompleted, "Dynamic Crisis Completed"},
    {ToastState::GalacticAnomalySystemEntered, "Galactic Anomaly Entered"},
};

// Indexed by Digit.PrimePlatform.Models.EventCategories.
constexpr const char* kEventCategories[] = {
    "Standard",           "Daily Goals",          "Daily Milestone",    "Leaderboard",           "Stat",
    "Battle Pass Season", "Battle Pass Event",    "Treasury Progress",  "Treasury Reward",       "Server Clash",
    "Webstore Event",     "Player Lifecycle",     "Field Training",     "FT Category",           "Cutscenes",
    "Minigame",           "Minigame Stage",       "Warchest",           "Alliance Game",         "Alliance Game Task",
    "Meta Event",         "Meta Event Objective", "Invasion",           "Loop Museum",           "Loop Museum Task",
    "PLC BP Season",      "PLC BP Event",         "Progression Reward", "Faction Weekly Events",
};

// Toasts whose Data is an EventModel; their body is prefixed with the event's category.
constexpr ToastState kEventToasts[] = {
    ToastState::Achievement,      ToastState::Tournament,   ToastState::ChainedEventScored,          ToastState::TreasuryProgress,           ToastState::TreasuryFull,
    ToastState::WarchestProgress, ToastState::WarchestFull, ToastState::FactionWeeklyEventsProgress, ToastState::FactionWeeklyEventsComplete};

// Toasts whose Data is a BattleResultHeader.
constexpr ToastState kBattleToasts[] = {ToastState::Victory,         ToastState::Defeat,           ToastState::PartialVictory, ToastState::StationVictory,
                                 ToastState::StationDefeat,   ToastState::StationBattle,    ToastState::IncomingAttack, ToastState::FleetBattle,
                                 ToastState::ArmadaBattleWon, ToastState::ArmadaBattleLost, ToastState::AssaultVictory, ToastState::AssaultDefeat};

constexpr BattleType kPvpBattles[] = {BattleType::Fleet,       BattleType::Base,      BattleType::ArmadaBase,
                                      BattleType::ArmadaAsb,   BattleType::ArmadaMta, BattleType::PvpCuttingBeam,
                                      BattleType::PvpChainShot};

template <typename T, size_t N> bool Contains(const T (&list)[N], T value)
{ return std::ranges::find(list, value) != std::end(list); }

// Toast data comes from a dozen game types whose layouts drift between builds. On Windows, an access violation
// while reading one is contained and logged here instead of taking the game down.
template <typename Fn> void Guarded(const char* what, Fn fn)
{
#if _WIN32
  __try {
    fn();
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    spdlog::warn("[Notify] access violation while reading {}", what);
  }
#else
  (void)what;
  fn();
#endif
}

// Localizes identifier (such as "ship_name_{0}") in category with one parameter. Empty when that fails.
std::string Localize(const char* identifier, const char* category, Il2CppObject* parameter)
{
  auto* object_class = SystemObject::Raw();
  if (!object_class || !parameter)
    return {};

  std::string result;
  Guarded(identifier, [&] {
    auto* parameters = il2cpp::Array<Il2CppObject*>::New(object_class, 1);
    auto* context    = il2cpp::New<LocaleTextContext>(il2cpp_string_new(identifier), il2cpp_string_new(category));
    if (!parameters || !context)
      return;
    parameters->Set(0, parameter);
    context->ApplyIdentifierParameters(parameters);
    if (auto* text = LocaleUtilities::Localize(context, false, false))
      result = to_string(text);
  });
  return result;
}

Il2CppObject* Box(int64_t value)
{
  auto* int64_class = SystemInt64::Raw();
  return int64_class ? il2cpp_value_box(int64_class, &value) : nullptr;
}

// NPC and marauder profiles have no name of their own; the game names them by their localization id.
std::string EnemyName(int64_t loca_id)
{ return loca_id ? Localize("marauder_name_only_{0}", "navigation", Box(loca_id)) : std::string{}; }

// A hull's display name: localized by numeric id, then by string id, then derived from its internal key.
std::string HullName(BattleResultHeader* header, int64_t hull_id)
{
  if (hull_id == 0)
    return "";
  auto* specs = header->specService();
  auto* hull  = specs ? specs->GetHull(hull_id) : nullptr;
  if (!hull)
    return fmt::format("Hull#{}", hull_id);

  // An unknown id localizes to the identifier itself.
  if (auto* ids = hull->idRefs()) {
    if (const auto loca_id = ids->locaId()) {
      auto name = Localize("ship_name_{0}", "ships", Box(loca_id));
      if (!name.empty() && name != fmt::format("ship_name_{}", loca_id))
        return name;
    }
    if (auto* loca_string = ids->locaStringId(); loca_string && loca_string->length) {
      const auto id = to_string(loca_string);
      auto name = Localize("ship_name_{0}", "ships", reinterpret_cast<Il2CppObject*>(il2cpp_string_new(id.c_str())));
      if (!name.empty() && name != "ship_name_" + id)
        return name;
    }
  }
  auto* key = hull->name();
  return key ? toast_text::HullKeyName(to_string(key)) : fmt::format("Hull#{}", hull_id);
}

// "Player (Ship) vs Enemy (Ship)". Outside PvP a named enemy is enough; its hull class is noise.
std::string BattleBody(BattleResultHeader* header)
{
  std::string player, enemy, player_ship, enemy_ship;
  bool        pvp = false;

  Guarded("the player profile", [&] {
    if (auto* profile = header->PlayerUserProfile())
      player = to_string(profile->name());
  });
  Guarded("the enemy profile", [&] {
    auto* profile = header->EnemyUserProfile();
    if (!profile)
      return;
    enemy = to_string(profile->name());
    if (enemy.empty()) {
      auto name = EnemyName(profile->locaId());
      if (name != "Unknown")
        enemy = std::move(name);
    }
  });
  Guarded("the battle type", [&] { pvp = Contains(kPvpBattles, header->BattleType()); });
  Guarded("the player hull",
          [&] { player_ship = toast_text::TitleCase(HullName(header, header->PlayerShipHullId())); });
  Guarded("the enemy hull", [&] { enemy_ship = toast_text::TitleCase(HullName(header, header->EnemyShipHullId())); });

  const auto side = [](const std::string& name, const std::string& ship) {
    return name.empty() ? ship : ship.empty() ? name : fmt::format("{} ({})", name, ship);
  };
  const auto left  = side(player, player_ship);
  const auto right = !pvp && !enemy.empty() ? enemy : side(enemy, enemy_ship);
  if (left.empty() || right.empty())
    return left + right;
  return left + " vs " + right;
}

// "[TAG] Owner started an Armada!\nLv.042 Target"
std::string ArmadaCreatedBody(ArmadaCreatedData* data)
{
  std::string body;
  Guarded("armada data", [&] {
    auto* attack   = data ? data->ArmadaAttack() : nullptr;
    auto* alliance = data ? data->Alliance() : nullptr;
    auto* owner    = attack ? attack->Owner() : nullptr;
    auto* target   = attack ? attack->TargetUserProfile() : nullptr;

    const auto tag         = alliance ? to_string(alliance->tag()) : std::string{};
    auto       owner_name  = owner ? to_string(owner->name()) : std::string{};
    auto       target_name = target ? to_string(target->name()) : std::string{};
    if (target_name.empty() && target)
      target_name = EnemyName(target->locaId());
    if (owner_name.empty())
      owner_name = "An alliance member";
    if (target_name.empty() || target_name == "Unknown")
      target_name = "Armada target";

    const auto owner_label = tag.empty() ? owner_name : fmt::format("[{}] {}", tag, owner_name);
    const auto level       = attack ? attack->TargetLevel() : 0;
    body                   = fmt::format("{} started an Armada!\nLv.{:03} {}", owner_label, level, target_name);
  });
  return body;
}

// The string form of each element: strings as they are, everything else through its ToString override.
std::vector<std::string> ParameterValues(il2cpp::Array<Il2CppObject*>* parameters)
{
  std::vector<std::string> values;
  for (il2cpp_array_size_t i = 0; parameters && i < parameters->size(); ++i) {
    auto*         element = (*parameters)[i];
    Il2CppString* text    = nullptr;
    if (element) {
      Guarded("a text parameter", [&] { text = TextOf(static_cast<il2cpp::Object*>(element)); });
    }
    values.push_back(to_string(text));
  }
  return values;
}

// Fills placeholders from whichever array field of `object` resolves the most of them. The parameter array has
// moved and been renamed between game builds, including generated backing fields, so this scans the runtime
// fields rather than naming one.
std::string FormatFromArrayFields(const std::string& text, Il2CppObject* object, std::string& fields_seen)
{
  auto best = text;
  for (auto* cls = object ? il2cpp_object_get_class(object) : nullptr; cls; cls = il2cpp_class_get_parent(cls)) {
    void* iter = nullptr;
    while (auto* field = il2cpp_class_get_fields(cls, &iter)) {
      auto* type = il2cpp_field_get_type(field);
      if ((il2cpp_field_get_flags(field) & FIELD_ATTRIBUTE_STATIC) || !type
          || (il2cpp_type_get_type(type) != IL2CPP_TYPE_SZARRAY && il2cpp_type_get_type(type) != IL2CPP_TYPE_ARRAY))
        continue;

      il2cpp::Array<Il2CppObject*>* array = nullptr;
      il2cpp_field_get_value(object, field, &array);
      fields_seen += fmt::format("{}{}[{}]", fields_seen.empty() ? "" : ", ", il2cpp_field_get_name(field),
                                 array ? std::to_string(array->size()) : "null");
      if (!array || array->size() == 0)
        continue;
      auto candidate = FormatPlaceholders(text, ParameterValues(array));
      if (toast_text::CountPlaceholders(candidate) < toast_text::CountPlaceholders(best))
        best = std::move(candidate);
    }
  }
  return best;
}

// Fills placeholders from Toast.TextParameters, or failing that from any array on the toast or its text context.
std::string FormatToastText(const std::string& text, Toast* toast, LocaleTextContext* context)
{
  if (auto* parameters = toast->TextParameters(); parameters && parameters->size() > 0)
    return FormatPlaceholders(text, ParameterValues(parameters));

  std::string fields_seen;
  for (auto* source : {static_cast<Il2CppObject*>(toast), static_cast<Il2CppObject*>(context)})
    if (auto result = FormatFromArrayFields(text, source, fields_seen); result != text)
      return result;
  if (text.find('{') != std::string::npos)
    spdlog::warn("[Notify] Placeholders unresolved; array fields seen: {}. Template: \"{}\"",
                 fields_seen.empty() ? "none" : fields_seen, text);
  return text;
}

// The toast's own localized text with its parameters filled in.
std::string ToastText(Toast* toast)
{
  auto* context = toast->TextLocaleTextContext();
  if (!context) {
    spdlog::debug("[Notify] No TextLocaleTextContext for toast state {}", static_cast<int>(toast->State()));
    return {};
  }

  // LocaleUtilities.Localize fills the context's own parameters; the toast may carry them instead.
  if (auto* text = LocaleUtilities::Localize(context, true, false)) {
    auto result = to_string(text);
    return result.find('{') == std::string::npos ? result : FormatToastText(result, toast, context);
  }

  Il2CppString* text    = nullptr;
  auto*         manager = LanguageManager::Instance();
  if (!manager || !manager->Localize(text, context) || !text) {
    spdlog::debug("[Notify] toast state {} could not be localized", static_cast<int>(toast->State()));
    return {};
  }
  return FormatToastText(to_string(text), toast, context);
}

std::string EventCategory(Toast* toast)
{
  auto*      data     = toast->Data();
  auto*      event    = data ? data->As<EventModel>() : nullptr;
  const auto category = event ? event->category() : -1;
  return category >= 0 && category < static_cast<int32_t>(std::size(kEventCategories)) ? kEventCategories[category]
                                                                                       : "";
}

std::string NotificationBody(Toast* toast)
{
  const auto  state = toast->State();
  auto*       data  = toast->Data();
  std::string body;
  if (state == ToastState::ArmadaCreated)
    body = ArmadaCreatedBody(static_cast<ArmadaCreatedData*>(data));
  else if (auto* header = data && Contains(kBattleToasts, state) ? data->As<BattleResultHeader>() : nullptr)
    body = BattleBody(header);
  if (body.empty())
    body = toast_text::StripRichText(ToastText(toast));
  if (body.empty())
    body = "(no details available)";

  if (Contains(kEventToasts, state))
    if (const auto category = EventCategory(toast); !category.empty())
      body = category + " - " + body;
  return body;
}

#if _WIN32
void ShowWindowsNotification(const char* title, const std::string& body)
{
  // Notifications need a WinRT apartment on the game thread, set up by the first one. The game may already have
  // one in another mode, which only matters if notifications then fail.
  static const bool apartment = [] {
    try {
      winrt::init_apartment(winrt::apartment_type::single_threaded);
    } catch (const winrt::hresult_error& e) {
      spdlog::warn("[Notify] Windows notification service failed: {:#x} {}", static_cast<unsigned long>(e.code()),
                   winrt::to_string(e.message()));
    } catch (...) {
      spdlog::warn("[Notify] Windows notification service failed (unknown error)");
    }
    return true;
  }();
  (void)apartment;
  try {
    using namespace winrt::Windows::UI::Notifications;
    auto xml   = ToastNotificationManager::GetTemplateContent(ToastTemplateType::ToastText02);
    auto nodes = xml.GetElementsByTagName(L"text");
    nodes.Item(0).InnerText(winrt::to_hstring(title));
    nodes.Item(1).InnerText(winrt::to_hstring(body));
    ToastNotificationManager::CreateToastNotifier(L"Star Trek Fleet Command").Show(ToastNotification(xml));
  } catch (const winrt::hresult_error& e) {
    spdlog::warn("[Notify] WinRT notification failed: {}", winrt::to_string(e.message()));
  } catch (...) {
    spdlog::warn("[Notify] WinRT notification failed (unknown error)");
  }
}
#endif

// TODO: macOS has no notification delivery yet; the body code above runs on Windows only.
void Notify(Toast* toast)
{
#if _WIN32
  const auto  state = toast->State();
  const auto& types = Config::Get().ui.notify_banner_types;
  if (std::ranges::find(types, static_cast<int>(state)) == types.end())
    return;
  const auto title = std::ranges::find(kTitles, state, &std::pair<ToastState, const char*>::first);
  if (title == std::end(kTitles)) {
    spdlog::debug("[Notify] No title mapping for toast state {}, skipping", static_cast<int>(state));
    return;
  }
  ShowWindowsNotification(title->second, NotificationBody(toast));
#else
  (void)toast;
#endif
}

// Notifies, then reports whether the in-game banner should be shown.
bool ShowBanner(Toast* toast)
{
  if (toast)
    notification_audio_play(Config::Get().NotificationSoundForToast(static_cast<int>(toast->State())));
  const auto& ui = Config::Get().ui;
  if (!toast || (ui.disabled_banner_types.empty() && ui.notify_banner_types.empty()))
    return true;
  Notify(toast);
  const auto& disabled = Config::Get().ui.disabled_banner_types;
  return std::ranges::find(disabled, static_cast<int>(toast->State())) == disabled.end();
}

void ToastObserver_EnqueueToast_Hook(auto original, ToastObserver* _this, Toast* toast)
{
  if (ShowBanner(toast))
    original(_this, toast);
}

void ToastObserver_EnqueueOrCombineToast_Hook(auto original, ToastObserver* _this, Toast* toast, uintptr_t compare)
{
  if (ShowBanner(toast))
    original(_this, toast, compare);
}
} // namespace

void InstallToasts()
{

  if (auto m = ToastObserver::Class::Method("EnqueueToast"))
    SPUD_STATIC_DETOUR(m.Pointer(), ToastObserver_EnqueueToast_Hook);
  if (auto m = ToastObserver::Class::Method("EnqueueOrCombineToast"))
    SPUD_STATIC_DETOUR(m.Pointer(), ToastObserver_EnqueueOrCombineToast_Hook);
}
