#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

enum class NotificationSound : uint8_t {
  kNone = 0,
  kDefault,
  kInfo,
  kSuccess,
  kWarning,
  kAlarm,
  kArrival,
  kSoft,
  kPing,
  kRepair,
  kCount,
};

[[nodiscard]] std::string_view                 NotificationSoundName(NotificationSound sound);
[[nodiscard]] std::optional<NotificationSound> ParseNotificationSound(std::string_view name);
void                                           PlayNotificationSound(NotificationSound sound);
