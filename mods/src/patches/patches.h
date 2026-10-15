#pragma once

// The mod's features: what mod.cc installs, and the few calls the frame tick and the shortcut dispatch in hotkeys.cc
// make into the features that own the work. Grouped by the file that defines them.

#include <cstdint>

struct NavigationZoom;
enum class NodeDepth : int32_t;

// anomaly_timer.cc
void InstallAnomalyTimer();
void AnomalyTimerUpdate();

// audio.cc
void InstallAudio();

// buffs.cc
void InstallOutOfDockPower();
void InstallBuffCrashGuard();

// camera.cc
void InstallZoom();
void InstallPan();

// cargo.cc
void InstallCargoFormat();
void InstallInstantCargo();

// chat.cc
void InstallChat();
// Opens chat, or focuses side chat's input. Never swallows the key.
void OpenChatShortcut();
// In chat, the channel shortcuts. True when the key was used.
bool ChatChannelShortcuts();

// dock.cc
void InstallPinnedShips();
void InstallDoubleClickAssign();
void DockUpdate();

// fleet_actions.cc
void InstallFleetActions();
void InstallQueueSwitch();
// A target or object viewer card is open in space.
bool TargetViewerShown();
// The number keys. True when the key was used.
bool SelectShipShortcuts();
// Select current and the queue switch. True when the key was used.
bool FleetShortcuts();
// The actions on the selected fleet and the open viewer. True when the key must not reach the game.
bool FleetActionShortcuts();

// fleet_labels.cc, fed by camera.cc's zoom hooks.
void InstallFleetLabels();
void FleetLabelsFollowDepth(NavigationZoom* zoom, NodeDepth depth);
void FleetLabelsAfterZoom(NavigationZoom* zoom);

// forbidden_tech.cc
void InstallForbiddenTechConfirm();

// game_config.cc
void InstallGameConfigOverrides();

// hotkeys.cc
void InstallHotkeys();
// ScreenManager.Update is the mod's frame tick: it runs the *Update functions here, then the shortcut dispatch.
// Features that poll every frame call this from their install. Safe to call more than once; false when the game
// lacks the method.
bool InstallFrameTick();

// instant_warp.cc
void InstallInstantWarp();
void InstantWarpUpdate();
void CycleInstantWarpConfirmation();

// keybind_badges.cc
void InstallKeybindBadges();
// Toggles the game's keybind badges. False when the badges are not the mod's, so the key is not used.
bool ToggleKeybindBadges();
void HideKeybindBadges();

// loading_screens.cc
void InstallLoginScreen();
void InstallTransitionScreen();
void InstallLoadingTips();

// missions.cc
void InstallMissionHud();
void InstallDailyClaimFilter();
void MissionsUpdate();

// officers.cc
void InstallOfficerSort();

// screens.cc
// Presses a button a screen shortcut opened a screen for, once the screen is up. True when one was pressed.
bool PressAwaitedButton();
// The shortcuts that open, step through, or search game screens. True when the key must not reach the game.
bool ScreenShortcuts();

// shop.cc
void InstallGiftsAutoOpen();
void InstallShopLimits();
void InstallSkipRevealSequence();
void InstallArtifactExchange();

// toasts.cc
void InstallToasts();

// ui_scale.cc
void InstallUiScale();
void AdjustUiScale(bool up);
void AdjustUiShipScale(bool up);
void AdjustUiViewerScale(bool up);

// window.cc
void InstallFreeResize();
void InstallCursor();
// The monitor's physical-to-logical pixel ratio on Windows; 1 elsewhere. Cached until `refresh`.
float WindowDpiScale(bool refresh = false);

// sync/
void InstallSync();

// game/object_tracker.cc
void InstallObjectTracker();

void InstallHavenZoomHooks();
void InstallHavenHistoryHooks();

void InstallInteriorZoomHooks();
void InstallChatTitleDisplayHooks();
void InstallPinnedShipClickHooks();
void InstallRevealConfirmationHooks();
void InstallActionQueueRecovery();
void InstallQueueAddressGuard();
void InstallThinQueueProtection();

void InstallInteriorZoomHooks();
void InstallChatTitleDisplayHooks();
void InstallRevealConfirmationHooks();
void InstallActionQueueRecovery();
void InstallQueueAddressGuard();
void InstallThinQueueProtection();
void InstallPinnedShipClickHooks();
