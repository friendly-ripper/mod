#pragma once

#include "Transform.h"
#include "Vector2.h"

#include <il2cpp/il2cpp.h>

struct RectTransform : Transform {
  using Class = il2cpp::Class<"UnityEngine.CoreModule", "UnityEngine", "RectTransform">;

  void SetAnchorMin(Vector2 value)
  { Set<"anchorMin">(value); }
  void SetAnchorMax(Vector2 value)
  { Set<"anchorMax">(value); }
  void SetPivot(Vector2 value)
  { Set<"pivot">(value); }
  void SetSizeDelta(Vector2 value)
  { Set<"sizeDelta">(value); }
  void SetAnchoredPosition(Vector2 value)
  { Set<"anchoredPosition">(value); }
};
