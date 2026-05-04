#pragma once

#include "raylib.h"
#include <string>

enum class AbilityType {
  None = 0,
  Shockwave,
  FrostNova,
  LightningBurst,
  GravityWell,
  Berserk,
  HealBurst,
  ShieldBubble,
  ChainsawArmor,
  Blaster,
  MeteorStorm,
  Inferno,
  BlackHole,
  TimeWarp,
  RocketBoost,
  PhoenixRevive,
  Vortex,
  NukeStrike,
};

struct AbilityState {
  AbilityType active = AbilityType::None;
  float activeTimer = 0.0f;
  float cooldownTimer = 0.0f;

  // Expanding-wave visuals (used by Shockwave and NukeStrike).
  float shockRadius = -1.0f;
  float prevShockRadius = -1.0f;
  Vector2 shockCenter{};

  // General purpose intensity (e.g. gift amount / diamond scaling)
  float power = 1.0f;

  // Set true on activation, consumed by Application for one-shot effects.
  bool justActivated = false;

  // Generic per-ability tick timer (Inferno DoT, Vortex pulses, etc.).
  float tickTimer = 0.0f;

  // Phoenix passive: armed = true means a lethal hit will be cheated
  // and the ball is revived to full health once.
  bool phoenixArmed = false;
};

struct AbilitySpec {
  AbilityType type;
  const char* name;
  Color uiColor;
  float duration;
  float cooldown;
};

const AbilitySpec& GetAbilitySpec(AbilityType t);

// Gift-driven mapping. Names take precedence; if the name is unknown
// (e.g. region-specific gift, gift_NNN fallback), the diamond cost is
// used to bucket the gift into a sensible ability tier.
AbilityType AbilityFromGift(const std::string& gift, int diamondCount = 0);

// Integer code passed to aura.fs (auraType uniform) so the GPU shader
// can render a different procedural pattern per ability. Keep these in
// lock-step with the if-chain inside aura.fs.
int GetAuraVisualKind(AbilityType t);

// Returns true when the ability needs a screen-space distortion pass and
// fills the out params used by world_distort.fs.
//   outType: 0=BlackHole 1=TimeWarp 2=Nuke 3=Vortex
//   outRadius: distortion radius in world pixels
//   outStrength: 0..1
bool GetAbilityWorldDistort(AbilityType t, int& outType,
                            float& outRadius, float& outStrength);

