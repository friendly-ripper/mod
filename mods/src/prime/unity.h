#pragma once

// Unity engine and .NET types the bindings share: math structs, scene objects, UI components, input and time.

#include "prime/KeyCode.h"
#include "str_utils.h"

#include <il2cpp/il2cpp.h>

#include <cstdint>
#include <string>
#include <string_view>

struct Vector2 {
  float x, y;
};

struct Vector3 {
  float x, y, z;
};

struct Color {
  float r, g, b, a;
};

struct Rect {
  float x, y, width, height;
};

// System.TimeSpan: 100 ns ticks.
struct TimeSpan {
  int64_t ticks;
};

// .NET classes the mod allocates or boxes into.
using SystemObject = il2cpp::Class<"mscorlib", "System", "Object">;
using SystemInt64  = il2cpp::Class<"mscorlib", "System", "Int64">;
using SystemByte   = il2cpp::Class<"mscorlib", "System", "Byte">;

// The text of any managed object: a string as itself, anything else through its ToString override.
inline Il2CppString* TextOf(il2cpp::Object* object)
{
  if (!object)
    return nullptr;
  auto* name = il2cpp_class_get_name(object->klass);
  if (name && std::string_view(name) == "String")
    return reinterpret_cast<Il2CppString*>(object);
  return object->Call<"ToString", Il2CppString*>();
}

// UnityEngine.Input and UnityEngine.Time through their internal calls, which skip the managed wrappers. A call that
// a Unity upgrade renamed is reported once and then reads as nothing pressed.
struct Input {
  // True when Unity lacks the call, so a caller that skips key checks on idle frames still checks.
  static bool anyKey()
  {
    static const il2cpp::ICall<bool()> call{"UnityEngine.Input::get_anyKey()"};
    return !call || call();
  }
  static bool anyKeyDown()
  {
    static const il2cpp::ICall<bool()> call{"UnityEngine.Input::get_anyKeyDown()"};
    return !call || call();
  }
  static bool GetKey(KeyCode key)
  {
    static const il2cpp::ICall<bool(KeyCode)> call{"UnityEngine.Input::GetKeyInt(UnityEngine.KeyCode)"};
    return call(key);
  }
  static bool GetKeyDown(KeyCode key)
  {
    static const il2cpp::ICall<bool(KeyCode)> call{"UnityEngine.Input::GetKeyDownInt(UnityEngine.KeyCode)"};
    return call(key);
  }
  static bool GetMouseButton(int32_t button)
  {
    static const il2cpp::ICall<bool(int32_t)> call{"UnityEngine.Input::GetMouseButton(System.Int32)"};
    return call(button);
  }
  static int32_t touchCount()
  {
    static const il2cpp::ICall<int32_t()> call{"UnityEngine.Input::get_touchCount()"};
    return call();
  }
  static Vector3 mousePosition()
  {
    static const il2cpp::ICall<void(Vector3*)> call{
        "UnityEngine.Input::get_mousePosition_Injected(UnityEngine.Vector3&)"};
    Vector3 position{};
    call(&position);
    return position;
  }
};

struct Time {
  // -1 when Unity lacks the call.
  static int32_t frameCount()
  {
    static const il2cpp::ICall<int32_t()> call{"UnityEngine.Time::get_frameCount()"};
    return call ? call() : -1;
  }
  static float deltaTime()
  {
    static const il2cpp::ICall<float()> call{"UnityEngine.Time::get_deltaTime()"};
    return call();
  }
};

struct Screen {
  using Class = il2cpp::Class<"UnityEngine.CoreModule", "UnityEngine", "Screen">;

  static int32_t width()
  { return Class::Get<"width", int32_t>(); }
  static int32_t height()
  { return Class::Get<"height", int32_t>(); }
};

using Cursor = il2cpp::Class<"UnityEngine.CoreModule", "UnityEngine", "Cursor">;

// UnityEngine.Object's statics. Alive is Unity's bool(Object): false for null, and for objects whose native side was
// destroyed even while a GC handle still holds the managed wrapper.
struct UnityObject {
  using Class = il2cpp::Class<"UnityEngine.CoreModule", "UnityEngine", "Object">;

  static bool Alive(Il2CppObject* object)
  { return object && Class::Call<"op_Implicit", bool>(object); }
  static void Destroy(Il2CppObject* object)
  { Class::Call<"Destroy">(object); }
  // Scans every loaded object; keep it off per-frame paths.
  template <typename T> static T* Find()
  {
    auto* type = T::Class::TypeObject();
    return type ? static_cast<T*>(Class::Call<"FindObjectOfType", Il2CppObject*>(type)) : nullptr;
  }
};

struct Transform;

struct GameObject : il2cpp::Object {
  using Class = il2cpp::Class<"UnityEngine.CoreModule", "UnityEngine", "GameObject">;

  std::string Name()
  { return to_string(Get<"name", Il2CppString*>()); }
  Transform* transform()
  { return Get<"transform", Transform*>(); }
  void SetActive(bool active)
  { Call<"SetActive">(active); }

  // Passing System.Type selects the (Type) overloads over the string and generic ones. A null Type would throw in
  // managed code, so a missing class yields null instead.
  template <typename T> T* GetComponent()
  {
    auto* type = T::Class::TypeObject();
    return type ? Call<"GetComponent", T*>(type) : nullptr;
  }
  template <typename T> T* AddComponent()
  {
    auto* type = T::Class::TypeObject();
    return type ? Call<"AddComponent", T*>(type) : nullptr;
  }
};

struct Transform : il2cpp::Object {
  using Class = il2cpp::Class<"UnityEngine.CoreModule", "UnityEngine", "Transform">;

  int32_t childCount()
  { return Get<"childCount", int32_t>(); }
  Transform* GetChild(int32_t index)
  { return Call<"GetChild", Transform*>(index); }
  Transform* parent()
  { return Get<"parent", Transform*>(); }
  GameObject* gameObject()
  { return Get<"gameObject", GameObject*>(); }
  Vector3 localScale()
  { return Get<"localScale", Vector3>(); }
  void SetLocalScale(Vector3 value)
  { Set<"localScale">(value); }
  void SetLocalEulerAngles(Vector3 value)
  { Set<"localEulerAngles">(value); }
  void SetParent(Transform* parent, bool worldPositionStays)
  { Call<"SetParent">(parent, worldPositionStays); }
  void SetAsFirstSibling()
  { Call<"SetAsFirstSibling">(); }
  // Finds a descendant by slash-separated path, e.g. "Hint/Background".
  Transform* Find(Il2CppString* path)
  { return Call<"Find", Transform*>(path); }
};

struct RectTransform : Transform {
  using Class = il2cpp::Class<"UnityEngine.CoreModule", "UnityEngine", "RectTransform">;

  void Place(Vector2 anchor_min, Vector2 anchor_max, Vector2 pivot, Vector2 size, Vector2 position)
  {
    Set<"anchorMin">(anchor_min);
    Set<"anchorMax">(anchor_max);
    Set<"pivot">(pivot);
    Set<"sizeDelta">(size);
    Set<"anchoredPosition">(position);
  }
};

// UnityEngine.Component, the base of every MonoBehaviour. Use As<Component>() on other bindings.
struct Component : il2cpp::Object {
  using Class = il2cpp::Class<"UnityEngine.CoreModule", "UnityEngine", "Component">;

  Transform* transform()
  { return Get<"transform", Transform*>(); }
  GameObject* gameObject()
  { return Get<"gameObject", GameObject*>(); }
};

struct Behaviour : Component {
  using Class = il2cpp::Class<"UnityEngine.CoreModule", "UnityEngine", "Behaviour">;

  void SetEnabled(bool value)
  { Set<"enabled">(value); }
};

struct Camera : il2cpp::Object {
  using Class = il2cpp::Class<"UnityEngine.CoreModule", "UnityEngine", "Camera">;

  void SetFarClipPlane(float value)
  { Set<"farClipPlane">(value); }
  // UnityEngine.CameraClearFlags
  void SetClearFlags(int32_t value)
  { Set<"clearFlags">(value); }
  void SetBackgroundColor(Color value)
  { Set<"backgroundColor">(value); }
};

struct Canvas : il2cpp::Object {
  using Class = il2cpp::Class<"UnityEngine.UIModule", "UnityEngine", "Canvas">;
};

struct CanvasScaler : il2cpp::Object {
  using Class = il2cpp::Class<"UnityEngine.UI", "UnityEngine.UI", "CanvasScaler">;

  // The property, which the scaler applies on its next Update; not the managed SetScaleFactor method, which writes
  // Canvas.scaleFactor at once.
  void SetScaleFactor(float value)
  { Set<"scaleFactor">(value); }
};

struct Texture2D : il2cpp::Object {
  using Class = il2cpp::Class<"UnityEngine.CoreModule", "UnityEngine", "Texture2D">;

  int32_t width()
  { return Get<"width", int32_t>(); }
  int32_t height()
  { return Get<"height", int32_t>(); }
};

struct ImageConversion {
  using Class = il2cpp::Class<"UnityEngine.ImageConversionModule", "UnityEngine", "ImageConversion">;

  // Decodes PNG or JPG bytes into `texture`, resizing it.
  static bool LoadImage(Texture2D* texture, il2cpp::Array<uint8_t>* data, bool markNonReadable = false)
  { return Class::Call<"LoadImage", bool>(texture, data, markNonReadable); }
};

struct Sprite : il2cpp::Object {
  using Class = il2cpp::Class<"UnityEngine.CoreModule", "UnityEngine", "Sprite">;

  static Sprite* Create(Texture2D* texture, Rect rect, Vector2 pivot)
  { return Class::Call<"Create", Sprite*>(texture, rect, pivot); }

  float pixelsPerUnit()
  { return Get<"pixelsPerUnit", float>(); }
};

// UnityEngine.UI.Image; color and raycastTarget come from Graphic.
struct Image : Component {
  using Class = il2cpp::Class<"UnityEngine.UI", "UnityEngine.UI", "Image">;

  enum class Type : int32_t {
    Simple = 0,
    Sliced = 1,
    Tiled  = 2,
    Filled = 3,
  };

  Sprite* sprite()
  { return Get<"sprite", Sprite*>(); }
  void SetSprite(Sprite* value)
  { Set<"sprite">(value); }
  Sprite* overrideSprite()
  { return Get<"overrideSprite", Sprite*>(); }
  void SetOverrideSprite(Sprite* value)
  { Set<"overrideSprite">(value); }
  void SetType(Type value)
  { Set<"type">(value); }
  void SetPreserveAspect(bool value)
  { Set<"preserveAspect">(value); }
  void SetPixelsPerUnitMultiplier(float value)
  { Set<"pixelsPerUnitMultiplier">(value); }
  void SetColor(Color value)
  { Set<"color">(value); }
  void SetRaycastTarget(bool value)
  { Set<"raycastTarget">(value); }
  void SetVerticesDirty()
  { Call<"SetVerticesDirty">(); }
};

struct Button : il2cpp::Object {
  using Class = il2cpp::Class<"UnityEngine.UI", "UnityEngine.UI", "Button">;

  void Press()
  { Call<"Press">(); }
};

struct EventSystem : il2cpp::Object {
  using Class = il2cpp::Class<"UnityEngine.UI", "UnityEngine.EventSystems", "EventSystem">;

  static EventSystem* current()
  { return Class::Get<"current", EventSystem*>(); }

  GameObject* currentSelectedGameObject()
  { return Get<"currentSelectedGameObject", GameObject*>(); }
  void SetSelectedGameObject(GameObject* object)
  { Call<"SetSelectedGameObject">(object); }
};

struct PointerEventData : il2cpp::Object {
  using Class = il2cpp::Class<"UnityEngine.UI", "UnityEngine.EventSystems", "PointerEventData">;
};

struct Toggle : Component {
  using Class = il2cpp::Class<"UnityEngine.UI", "UnityEngine.UI", "Toggle">;

  bool isOn()
  { return Get<"isOn", bool>(); }

  // Goes through the pointer handler instead of the isOn setter, so interactable checks and listeners behave as for
  // a real click.
  void SimulateClick()
  {
    if (auto* event = il2cpp::New<PointerEventData>(EventSystem::current()))
      Call<"OnPointerClick">(event);
  }
};

struct TMP_Text : Component {
  using Class = il2cpp::Class<"Unity.TextMeshPro", "TMPro", "TMP_Text">;

  // TMP_FontAsset
  il2cpp::Object* font()
  { return Get<"font", il2cpp::Object*>(); }
  void SetFont(il2cpp::Object* value)
  { Set<"font">(value); }
  void SetFontSize(float value)
  { Set<"fontSize">(value); }
  // TextAlignmentOptions
  void SetAlignment(int32_t value)
  { Set<"alignment">(value); }
  void SetColor(Color value)
  { Set<"color">(value); }
  void SetRaycastTarget(bool value)
  { Set<"raycastTarget">(value); }
  void SetEnableWordWrapping(bool value)
  { Set<"enableWordWrapping">(value); }
  void SetText(Il2CppString* value)
  { Set<"text">(value); }
};

struct TextMeshProUGUI : TMP_Text {
  using Class = il2cpp::Class<"Unity.TextMeshPro", "TMPro", "TextMeshProUGUI">;
};

struct TMP_InputField : il2cpp::Object {
  using Class = il2cpp::Class<"Unity.TextMeshPro", "TMPro", "TMP_InputField">;

  bool isFocused()
  { return Get<"isFocused", bool>(); }
  void ActivateInputField()
  { Call<"ActivateInputField">(); }
};

struct InputAction : il2cpp::Object {
  using Class = il2cpp::Class<"Unity.InputSystem", "UnityEngine.InputSystem", "InputAction">;

  Il2CppString* name()
  { return Get<"name", Il2CppString*>(); }
};

struct InputActionReference : il2cpp::Object {
  using Class = il2cpp::Class<"Unity.InputSystem", "UnityEngine.InputSystem", "InputActionReference">;

  InputAction* action()
  { return Get<"action", InputAction*>(); }
};

// UnityEngine.InputSystem.InputAction.CallbackContext, passed empty. Generated code takes it by value with the
// platform's C++ ABI, so this layout must match the managed struct.
struct InputActionCallbackContext {
  void*   state;
  int32_t action_index;
};
static_assert(sizeof(InputActionCallbackContext) == 16);

enum class TouchPhase : int32_t {
  Began      = 0,
  Moved      = 1,
  Stationary = 2,
  Ended      = 3,
  Canceled   = 4,
};

struct TKTouch : il2cpp::Object {
  using Class = il2cpp::Class<"TouchKit", "", "TKTouch">;

  TouchPhase phase()
  { return Field<"phase", TouchPhase>(); }
  void SetPhase(TouchPhase phase)
  { SetField<"phase">(phase); }
};
