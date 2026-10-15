// The shop, gift and inventory popups: auto_open_bulk_claim_flyout on the gifts screen, the chest purchase and
// donation limits, the view button on offers that are cooling down, and always_skip_reveal_sequence.

#include "config.h"
#include "patches/patches.h"

#include "prime/inventory.h"

#include <il2cpp/il2cpp.h>
#include <spud/detour.h>

#include <algorithm>
#include <atomic>

namespace
{
// --- Gifts: open the bulk claim drawer ---

// The gifts screen that is showing, and whether its drawer was opened since it appeared.
ShopListScrollerViewController* shown_scroller = nullptr;
bool                            drawer_opened  = false;

void OpenBulkClaimDrawer(ShopListScrollerViewController* controller)
{
  if (!Config::Get().ui.auto_open_bulk_claim_flyout || !controller || drawer_opened)
    return;

  auto* drawer  = controller->selectionDrawerWidget();
  auto* context = drawer ? drawer->Context() : nullptr;
  if (!context || !context->Enabled())
    return;

  // Mark first so a native callback cannot cause a nested lifecycle update to open the drawer twice.
  drawer_opened = true;
  drawer->OnOpenButtonClicked();
}

void ShopListScrollerViewController_AboutToShow_Hook(auto original, ShopListScrollerViewController* _this)
{
  shown_scroller = _this;
  drawer_opened  = false;
  original(_this);
  OpenBulkClaimDrawer(_this);
}

void ShopListScrollerViewController_AboutToHide_Hook(auto original, ShopListScrollerViewController* _this)
{
  if (shown_scroller == _this) {
    shown_scroller = nullptr;
    drawer_opened  = false;
  }
  original(_this);
}

// The drawer only becomes usable once the game fills it.
void ShopListScrollerViewController_UpdateDrawer_Hook(auto original, ShopListScrollerViewController* _this)
{
  original(_this);
  if (shown_scroller == _this)
    OpenBulkClaimDrawer(_this);
}

// --- Purchase and donation limits ---

// The most the game's chest purchase popup handles; extend_chest_purchase_max is clamped to it.
constexpr int64_t kMaximumChestPurchaseMax = 160;

// The popup's limits are adjusted where its row is rendered. IsChestPurchase is set too late for the MaxItemsToUse
// setter, and adjusting the context rather than the slider leaves every other slider alone.
void InventoryUseRowWidget_SetWidgetData_Hook(auto original, InventoryUseRowWidget* _this)
{
  auto*       context = _this ? _this->Context() : nullptr;
  const auto& ui      = Config::Get().ui;
  if (context && context->IsChestPurchase()) {
    const auto native_max  = context->MaxItemsToUse();
    const auto bounded_max = std::min(static_cast<int64_t>(ui.extend_chest_purchase_max), kMaximumChestPurchaseMax);
    if (native_max > 0 && bounded_max > native_max)
      context->SetMaxItemsToUse(bounded_max);
  } else if (context && ui.extend_donation_slider && context->IsDonationUse()) {
    // extend_donation_max replaces the game's donation cap; 0 or less means no cap. Either way the slider stops at
    // what the player owns.
    const auto owned  = context->AmountOwned();
    auto       target = ui.extend_donation_max > 0 ? static_cast<int64_t>(ui.extend_donation_max) : owned;
    if (owned > 0)
      target = std::min(target, owned);
    if (target > 0 && target != context->MaxItemsToUse())
      context->SetMaxItemsToUse(target);
  }
  original(_this);
}

// --- Offers on cooldown ---

// An offer that is cooling down cannot be bought; its button opens the offer's details instead.
void BundleDataWidget_OnActionButtonPressedCallback_Hook(auto original, BundleDataWidget* _this)
{
  if (_this->CurrentState() & BundleDataWidget::kCooldownTimerOn)
    _this->AuxViewButtonPressedHandler();
  else
    original(_this);
}

// --- Reveal sequence ---

bool ShopSceneManager_ShouldShowRevealSequence_Hook(auto original, void* _this, bool ignore)
{
  const auto result = original(_this, ignore);
  return result && !Config::Get().ui.always_skip_reveal_sequence;
}

// --- Artifact Exchange All ---

// Hides the button after the popup binds. Individual exchanges stay available.
void InventoryUsePopupViewController_OnDidBindCanvasContext_Hook(auto original, InventoryUsePopupViewController* _this)
{
  original(_this);
  if (!_this || !Config::Get().ui.disable_exchange_all)
    return;
  auto* button = _this->convertAllButton();
  if (auto* object = button ? button->gameObject() : nullptr)
    object->SetActive(false);
}
} // namespace

void InstallGiftsAutoOpen()
{
  using Scroller = ShopListScrollerViewController::Class;
  if (auto m = Scroller::Method("AboutToShow"))
    SPUD_STATIC_DETOUR(m.Pointer(), ShopListScrollerViewController_AboutToShow_Hook);
  if (auto m = Scroller::Method("AboutToHide"))
    SPUD_STATIC_DETOUR(m.Pointer(), ShopListScrollerViewController_AboutToHide_Hook);
  if (auto m = Scroller::Method("UpdateDrawer"))
    SPUD_STATIC_DETOUR(m.Pointer(), ShopListScrollerViewController_UpdateDrawer_Hook);
}

void InstallShopLimits()
{
  if (auto m = InventoryUseRowWidget::Class::Method("SetWidgetData", 0))
    SPUD_STATIC_DETOUR(m.Pointer(), InventoryUseRowWidget_SetWidgetData_Hook);
  if (auto m = BundleDataWidget::Class::Method("OnActionButtonPressedCallback"))
    SPUD_STATIC_DETOUR(m.Pointer(), BundleDataWidget_OnActionButtonPressedCallback_Hook);
}


std::atomic_bool popup_shown{false};

bool ShouldSuppressAdditionalPopup()
{
  bool expected = false;
  return !popup_shown.compare_exchange_strong(expected, true);
}

void InterstitialViewController_AboutToShow_Hook(auto original, InterstitialViewController* _this)
{
  original(_this);
  if (Config::Get().ui.only_show_first_popup && _this && ShouldSuppressAdditionalPopup())
    _this->CloseWhenReady();
}

void ShopSceneManager_ShowPlcOfferPopup_Hook(auto original, void* _this)
{
  if (!Config::Get().ui.only_show_first_popup || !ShouldSuppressAdditionalPopup())
    original(_this);
}

void InstallSkipRevealSequence()
{
  if (auto m = ShopSceneManager::Method("ShouldShowRevealSequence"))
    SPUD_STATIC_DETOUR(m.Pointer(), ShopSceneManager_ShouldShowRevealSequence_Hook);

  using PopupShop = il2cpp::Class<"Assembly-CSharp", "Digit.Prime.Shop", "ShopSceneManager">;
  if (auto m = PopupShop::Method("ShowPlcOfferPopup", 0))
    SPUD_STATIC_DETOUR(m.Pointer(), ShopSceneManager_ShowPlcOfferPopup_Hook);
  if (auto m = InterstitialViewController::Class::Method("AboutToShow", 0))
    SPUD_STATIC_DETOUR(m.Pointer(), InterstitialViewController_AboutToShow_Hook);
}

void InstallArtifactExchange()
{
  if (auto m = InventoryUsePopupViewController::Class::Method("OnDidBindCanvasContext", 0))
    SPUD_STATIC_DETOUR(m.Pointer(), InventoryUsePopupViewController_OnDidBindCanvasContext_Hook);
}
