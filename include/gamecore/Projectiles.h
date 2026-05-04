#pragma once

#include "raylib.h"
#include <string>

enum class ProjectileKind : unsigned char {
  Generic = 0,
  BlasterBolt,
  Meteor,
};

struct Projectile {
  Vector2 pos{};
  Vector2 vel{};
  float radius = 6.0f;
  float life = 1.2f;
  float damage = 18.0f;
  Color color = YELLOW;
  ProjectileKind kind = ProjectileKind::Generic;
  std::string ownerUserId;
  bool active = true;
};

