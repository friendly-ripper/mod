#pragma once

#include "EntityGroup.h"

#include <il2cpp/il2cpp.h>

struct ServiceResponse : il2cpp::Object {
  using Class = il2cpp::Class<"Digit.Client.PrimeLib.Runtime", "Digit.PrimeServer.Models", "ServiceResponse">;

  // Google.Protobuf.Collections.RepeatedField<EntityGroup>; List resolves Count/get_Item on it.
  il2cpp::List<EntityGroup*>* EntityGroups()
  { return Get<"EntityGroups", il2cpp::List<EntityGroup*>*>(); }
};
