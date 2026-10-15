// [config] settings_url and assets_url_override: replace the server addresses in the game's own app config as it
// loads.

#include "config.h"
#include "patches/patches.h"

#include "prime/Hub.h"

#include <il2cpp/il2cpp.h>
#include <spud/detour.h>

namespace
{
void Model_LoadConfigs_Hook(auto original, Model* _this)
{
  original(_this);
  auto* config = _this->appConfig();
  if (!config)
    return;
  const auto& game = Config::Get().game;
  if (!game.settings_url.empty())
    config->SetPlatformSettingsUrl(il2cpp_string_new(game.settings_url.c_str()));
  if (!game.assets_url_override.empty())
    config->SetAssetUrlOverride(il2cpp_string_new(game.assets_url_override.c_str()));
}
} // namespace

void InstallGameConfigOverrides()
{
  if (auto m = Model::Class::Method("LoadConfigs", 0))
    SPUD_STATIC_DETOUR(m.Pointer(), Model_LoadConfigs_Hook);
}
