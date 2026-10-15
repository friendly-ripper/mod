#pragma once

// The login screen, the transition screen shown between sections, and their loading tips and slide shows.

#include "prime/ui.h"
#include "prime/unity.h"

#include <il2cpp/il2cpp.h>

struct LoginSequence : il2cpp::Object {
  using Class = il2cpp::Class<"Assembly-CSharp", "Digit.Prime.Login", "LoginSequence">;

  Component* mainCanvas()
  { return Field<"_mainCanvas", Component*>(); }
};

struct LoadingTipViewController : il2cpp::Object {
  using Class = il2cpp::Class<"Assembly-CSharp", "Prime.LoadingScreen", "LoadingTipViewController">;

  TMP_Text* textMeshPro()
  { return Field<"_textMeshPro", TMP_Text*>(); }
};

struct TransitionViewController : Component {
  using Class = il2cpp::Class<"Assembly-CSharp", "Digit.Prime.LoadingScreen", "TransitionViewController">;

  Image* staticOverride()
  { return Field<"_staticOverride", Image*>(); }
  // The canvas Animator; Behaviour is enough to toggle it.
  Behaviour* animator()
  { return Field<"_animator", Behaviour*>(); }
};

struct SlideShowViewController : il2cpp::Object {
  using Class = il2cpp::Class<"Assembly-CSharp", "Digit.Prime.SlideShow", "SlideShowViewController">;

  Image* image()
  { return Field<"_image", Image*>(); }
};
