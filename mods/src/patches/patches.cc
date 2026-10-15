#include "mod.h"
#include "patches/patches.h"

#include "config.h"
#include "file.h"
#include "version.h"

#include <il2cpp/il2cpp.h>
#include <spud/detour.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <iterator>

#if _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#include <libgen.h>
#include <limits.h>
#include <mach-o/dyld.h>
#endif

namespace
{
struct Feature {
  const char* name;
  void (*install)();
  // The [patches] hook group that installs it; null for the few that always install.
  bool Config::Patches::* group;
};

// Installed in this order; the log numbers them. Only the [patches] hook groups decide what is installed, so users
// can avoid clashing with other mods. Feature settings are read inside the hooks, which keeps every feature
// switchable at runtime; a hook whose feature is off must return early and cheaply.
constexpr Feature kFeatures[] = {
    {"interiorzoomhooks", InstallInteriorZoomHooks, &Config::Patches::interiorzoomhooks},
    {"chattitledisplayhooks", InstallChatTitleDisplayHooks, &Config::Patches::chattitledisplayhooks},
    {"revealconfirmhooks", InstallRevealConfirmationHooks, &Config::Patches::revealconfirmhooks},
    {"actionqueuerecoveryhooks", InstallActionQueueRecovery, &Config::Patches::actionqueuerecoveryhooks},
    {"queueaddressguardhooks", InstallQueueAddressGuard, &Config::Patches::queueaddressguardhooks},
    {"thinqueueprotectionhooks", InstallThinQueueProtection, &Config::Patches::thinqueueprotectionhooks},
    {"pinnedshipclicktoggle", InstallPinnedShipClickHooks, &Config::Patches::pinnedshipclicktoggle},

    {"HavenZoom", InstallHavenZoomHooks, &Config::Patches::havenzoomhooks},
    {"HavenHistory", InstallHavenHistoryHooks, &Config::Patches::havenhistoryhooks},
    {"UiScale", InstallUiScale, &Config::Patches::uiscalehooks},
    {"Zoom", InstallZoom, &Config::Patches::zoomhooks},
    {"FleetLabels", InstallFleetLabels, &Config::Patches::zoomhooks},
    {"Pan", InstallPan, &Config::Patches::panhooks},
    {"OutOfDockPower", InstallOutOfDockPower, &Config::Patches::bufffixhooks},
    {"BuffCrashGuard", InstallBuffCrashGuard, &Config::Patches::tempcrashfixes},
    {"Toasts", InstallToasts, &Config::Patches::toastbannerhooks},
    {"Hotkeys", InstallHotkeys, &Config::Patches::hotkeyhooks},
    {"GiftsAutoOpen", InstallGiftsAutoOpen, &Config::Patches::giftsbulkclaimhooks},
    {"DailyClaimFilter", InstallDailyClaimFilter, &Config::Patches::dailyfactionbulkclaimhooks},
#if _WIN32
    {"FreeResize", InstallFreeResize, &Config::Patches::freeresizehooks},
    {"Cursor", InstallCursor, &Config::Patches::testpatches},
#endif
    {"SkipRevealSequence", InstallSkipRevealSequence, &Config::Patches::tempcrashfixes},
    {"GameConfigOverrides", InstallGameConfigOverrides, &Config::Patches::testpatches},
    {"QueueSwitch", InstallQueueSwitch, &Config::Patches::testpatches},
    {"ShopLimits", InstallShopLimits, &Config::Patches::miscpatches},
    {"MissionHud", InstallMissionHud, &Config::Patches::missionhudtweakshooks},
    {"ArtifactExchange", InstallArtifactExchange, &Config::Patches::artifactexchangehooks},
    {"Chat", InstallChat, &Config::Patches::chatpatches},
    {"Sync", InstallSync, &Config::Patches::syncpatches},
    {"ObjectTracker", InstallObjectTracker, &Config::Patches::objecttracker},
    {"LoginScreen", InstallLoginScreen, &Config::Patches::loadingscreenhooks},
    {"TransitionScreen", InstallTransitionScreen, &Config::Patches::transitionscreenhooks},
    {"AnomalyTimer", InstallAnomalyTimer, &Config::Patches::galacticanomalytimerhooks},
    {"LoadingTips", InstallLoadingTips, &Config::Patches::loadingtiphooks},
    {"InstantCargo", InstallInstantCargo, &Config::Patches::instantcargocounterhooks},
    {"CargoFormat", InstallCargoFormat, &Config::Patches::cargoformathooks},
    {"OfficerSort", InstallOfficerSort, &Config::Patches::officersorthooks},
    {"PinnedShips", InstallPinnedShips, &Config::Patches::pinnedshiphooks},
    {"DoubleClickAssign", InstallDoubleClickAssign, &Config::Patches::doubleclickassignshiphooks},
    {"InstantWarp", InstallInstantWarp, nullptr},
    {"ForbiddenTechConfirm", InstallForbiddenTechConfirm, &Config::Patches::forbiddentechconfirmhooks},
    {"Audio", InstallAudio, &Config::Patches::audioeventhooks},
    // Watching the settings file only needs the frame tick.
    {"SettingsReload", [] { InstallFrameTick(); }, nullptr},
};

void StartLogging()
{
  std::string rotation_error;
  try {
    const auto backup = [](int index) { return std::filesystem::path(File::Log() + "." + std::to_string(index)); };
    std::filesystem::remove(backup(5));
    for (int index = 4; index >= 1; --index)
      if (std::filesystem::exists(backup(index)))
        std::filesystem::rename(backup(index), backup(index + 1));
    if (std::filesystem::exists(File::Log()))
      std::filesystem::rename(File::Log(), backup(1));
  } catch (const std::filesystem::filesystem_error& error) {
    rotation_error = error.what();
  }
  // Append if rotation failed so an existing session log is not truncated.
  auto logger = spdlog::basic_logger_mt("default", File::Log(), false);
  logger->sinks().push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  spdlog::set_default_logger(logger);

  const auto level = File::Trace() ? spdlog::level::trace : File::Debug() ? spdlog::level::debug : spdlog::level::info;
  spdlog::set_level(level);
  spdlog::flush_on(level);
  if (!rotation_error.empty())
    spdlog::warn("Failed to rotate log history; preserving existing log: {}", rotation_error);
  il2cpp::SetErrorHandler([](std::string_view message) { spdlog::error("{}", message); });
}

void LogBanner()
{
  if constexpr (VERSION_PATCH && sizeof(VERSION_COMMIT_HASH) > 1)
    spdlog::info("Initializing STFC Community Mod ({} [{}])", VER_PRODUCT_VERSION_STR, VERSION_COMMIT_HASH);
  else
    spdlog::info("Initializing STFC Community Mod ({})", VER_PRODUCT_VERSION_STR);
  spdlog::info("");
  spdlog::info("{}", File::Profile().empty() ? "Using standard names" : "Using custom names");
  spdlog::info("  Log: {}", File::Log());
  spdlog::info("  Cfg: {}", File::Config());
  spdlog::info("  Var: {}", File::Vars());
  spdlog::info("   BL: {}", File::Battles());
  spdlog::info("");
  if constexpr (VERSION_PATCH) {
    spdlog::warn("*** NOTE: Beta versions may have unexpected bugs and issues");
    spdlog::info("");
  }
  spdlog::info("Please see https://github.com/netniv/stfc-mod for latest configuration help,");
  spdlog::info("examples and future releases, or visit the STFC Community Mod discord server");
  spdlog::info("at https://discord.gg/PrpHgs7Vjs");
  spdlog::info("");
  spdlog::info("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
  spdlog::info("");
}

int64_t il2cpp_init_hook(auto original, const char* domain_name)
{
#if _WIN32 && !defined(NDEBUG)
  AllocConsole();
  FILE* console;
  freopen_s(&console, "CONOUT$", "w", stdout);
#endif

  // Nothing here may throw into the game: a mod that cannot set itself up leaves the game running without it.
  const Config* config = nullptr;
  try {
    File::Init();
    StartLogging();
    LogBanner();

    spdlog::info("Loading Configuration...");
    spdlog::info("");
    config = &Config::Get();
    spdlog::info("");
    spdlog::info("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
    spdlog::info("");
  } catch (const std::exception& error) {
    // The file logger may not exist; spdlog falls back to its console logger.
    spdlog::error("Community mod setup failed, not installing any patches: {}", error.what());
  }

  // Bindings resolve against the runtime's metadata, so nothing is installed before the domain exists.
  const auto result = original(domain_name);
  if (!config)
    return result;

  spdlog::info("Initializing code hooks:");
  for (size_t i = 0; i < std::size(kFeatures); ++i) {
    const auto& feature = kFeatures[i];
    const bool  enabled = !feature.group || config->patches.*feature.group;
    spdlog::info(" {} {:>2} of {} ({})", enabled ? "+ Patching" : "x Skipping", i + 1, std::size(kFeatures),
                 feature.name);
    if (!enabled)
      continue;
    try {
      feature.install();
    } catch (const std::exception& error) {
      spdlog::error("{} failed to install: {}", feature.name, error.what());
    }
  }

  spdlog::info("");
  if constexpr (VERSION_PATCH)
    spdlog::info("Installed beta version {}.{}.{} (Patch {})", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION,
                 VERSION_PATCH);
  else
    spdlog::info("Installed release version {}.{}.{}", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
  spdlog::info("");
  spdlog::info("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
  spdlog::info("");

  return result;
}
} // namespace

void StartMod()
{
#if _WIN32
  auto assembly = LoadLibraryA("GameAssembly.dll");
#else
  char     executable[PATH_MAX];
  uint32_t size = sizeof(executable);
  if (_NSGetExecutablePath(executable, &size) != 0) {
    fprintf(stderr, "stfc-community-mod: executable path does not fit\n");
    return;
  }
  char assembly_path[PATH_MAX];
  snprintf(assembly_path, sizeof(assembly_path), "%s/../Frameworks/GameAssembly.dylib", dirname(executable));
  auto assembly = dlopen(assembly_path, RTLD_LAZY | RTLD_GLOBAL);
#endif

  // The logger does not exist yet; it is set up inside il2cpp_init once the settings path is known.
  if (!assembly) {
    fprintf(stderr, "stfc-community-mod: failed to load GameAssembly\n");
    return;
  }

  try {
    il2cpp::Initialize(assembly);
#if _WIN32
    auto il2cpp_init = reinterpret_cast<void*>(GetProcAddress(assembly, "il2cpp_init"));
#else
    auto il2cpp_init = dlsym(assembly, "il2cpp_init");
#endif
    SPUD_STATIC_DETOUR(il2cpp_init, il2cpp_init_hook);
  } catch (const std::exception& error) {
    fprintf(stderr, "stfc-community-mod: failed to hook il2cpp_init: %s\n", error.what());
  }
}
