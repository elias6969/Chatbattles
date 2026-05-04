#include "gamecore/Abilities.h"

static const AbilitySpec kNone   {AbilityType::None,           "None",            GRAY,                       0.0f, 0.0f};
static const AbilitySpec kShock  {AbilityType::Shockwave,      "Shockwave",       {190,  90, 255, 255},       0.55f, 7.0f};
static const AbilitySpec kFrost  {AbilityType::FrostNova,      "Frost Nova",      {120, 200, 255, 255},       2.0f, 10.0f};
static const AbilitySpec kLight  {AbilityType::LightningBurst, "Lightning",       {255, 240, 120, 255},       0.40f, 9.0f};
static const AbilitySpec kGrav   {AbilityType::GravityWell,    "Gravity Well",    {120, 255, 180, 255},       3.0f, 12.0f};
static const AbilitySpec kBers   {AbilityType::Berserk,        "Berserk",         {255,  90,  90, 255},       4.5f, 14.0f};
static const AbilitySpec kHeal   {AbilityType::HealBurst,      "Heal Burst",      {120, 255, 120, 255},       0.20f, 9.0f};
static const AbilitySpec kShield {AbilityType::ShieldBubble,   "Shield",          {160, 200, 255, 255},       6.0f, 16.0f};
static const AbilitySpec kSaw    {AbilityType::ChainsawArmor,  "Chainsaw",        {255, 170,  90, 255},       7.0f, 18.0f};
static const AbilitySpec kBlast  {AbilityType::Blaster,        "Blaster",         {255, 220, 120, 255},       6.0f, 14.0f};
static const AbilitySpec kMeteor {AbilityType::MeteorStorm,    "Meteor Storm",    {255, 120, 190, 255},       5.0f, 25.0f};
static const AbilitySpec kInferno{AbilityType::Inferno,        "Inferno",         {255, 110,  40, 255},       4.0f, 13.0f};
static const AbilitySpec kBlackHole{AbilityType::BlackHole,    "Black Hole",      {180,  80, 255, 255},       3.0f, 22.0f};
static const AbilitySpec kTime   {AbilityType::TimeWarp,       "Time Warp",       {130, 160, 255, 255},       3.5f, 24.0f};
static const AbilitySpec kRocket {AbilityType::RocketBoost,    "Rocket Boost",    {120, 200, 255, 255},       2.5f, 11.0f};
static const AbilitySpec kPhoenix{AbilityType::PhoenixRevive,  "Phoenix",         {255, 200,  80, 255},      12.0f, 30.0f};
static const AbilitySpec kVortex {AbilityType::Vortex,         "Vortex",          {200, 120, 255, 255},       2.5f, 14.0f};
static const AbilitySpec kNuke   {AbilityType::NukeStrike,     "Nuke",            {255, 180,  80, 255},       0.7f, 25.0f};

const AbilitySpec& GetAbilitySpec(AbilityType t) {
  switch (t) {
  case AbilityType::Shockwave:      return kShock;
  case AbilityType::FrostNova:      return kFrost;
  case AbilityType::LightningBurst: return kLight;
  case AbilityType::GravityWell:    return kGrav;
  case AbilityType::Berserk:        return kBers;
  case AbilityType::HealBurst:      return kHeal;
  case AbilityType::ShieldBubble:   return kShield;
  case AbilityType::ChainsawArmor:  return kSaw;
  case AbilityType::Blaster:        return kBlast;
  case AbilityType::MeteorStorm:    return kMeteor;
  case AbilityType::Inferno:        return kInferno;
  case AbilityType::BlackHole:      return kBlackHole;
  case AbilityType::TimeWarp:       return kTime;
  case AbilityType::RocketBoost:    return kRocket;
  case AbilityType::PhoenixRevive:  return kPhoenix;
  case AbilityType::Vortex:         return kVortex;
  case AbilityType::NukeStrike:     return kNuke;
  default: return kNone;
  }
}

static AbilityType AbilityFromName(const std::string& gift)
{
  // Cheap / spammable -------------------------------------------------------
  if (gift == "Rose")             return AbilityType::HealBurst;
  if (gift == "Ice Cream Cone")   return AbilityType::HealBurst;
  if (gift == "Lion 222")         return AbilityType::HealBurst;
  if (gift == "TikTok")           return AbilityType::Shockwave;
  if (gift == "Finger Heart")     return AbilityType::FrostNova;
  if (gift == "Mic")              return AbilityType::ShieldBubble;
  if (gift == "Lollipop")         return AbilityType::Berserk;
  if (gift == "Perfume")          return AbilityType::ShieldBubble;
  if (gift == "Weights")          return AbilityType::RocketBoost;
  if (gift == "Doughnut")         return AbilityType::GravityWell;
  if (gift == "Donut")            return AbilityType::GravityWell;

  // Mid -------------------------------------------------------------------
  if (gift == "Love you")         return AbilityType::HealBurst;
  if (gift == "Paper Crane")      return AbilityType::FrostNova;
  if (gift == "Hand Hearts")      return AbilityType::HealBurst;
  if (gift == "Music Note")       return AbilityType::LightningBurst;
  if (gift == "Hearts")           return AbilityType::LightningBurst;
  if (gift == "Goggles")          return AbilityType::FrostNova;
  if (gift == "Lock and Key")     return AbilityType::ShieldBubble;
  if (gift == "Rock 'n' Roll")    return AbilityType::ChainsawArmor;
  if (gift == "Air Dancer")       return AbilityType::Berserk;
  if (gift == "Swing")            return AbilityType::GravityWell;
  if (gift == "Necklace")         return AbilityType::ChainsawArmor;
  if (gift == "Coral")            return AbilityType::MeteorStorm;

  // High ------------------------------------------------------------------
  if (gift == "Ice Machine")      return AbilityType::FrostNova;
  if (gift == "Heels")            return AbilityType::Blaster;
  if (gift == "LOVE Balloon")     return AbilityType::Inferno;
  if (gift == "Swan")             return AbilityType::PhoenixRevive;
  if (gift == "Garland")          return AbilityType::Vortex;
  if (gift == "Garland ")         return AbilityType::Vortex;

  // Premium ---------------------------------------------------------------
  if (gift == "Treehouse")        return AbilityType::Inferno;
  if (gift == "Speedboat")        return AbilityType::RocketBoost;
  if (gift == "Carousel")         return AbilityType::BlackHole;
  if (gift == "Motorcycle")       return AbilityType::Vortex;
  if (gift == "Ferris Wheel")     return AbilityType::BlackHole;
  if (gift == "Private Jet")      return AbilityType::NukeStrike;
  if (gift == "Pool Party")       return AbilityType::MeteorStorm;
  if (gift == "Submarine")        return AbilityType::Vortex;

  // Ultimate --------------------------------------------------------------
  if (gift == "Interstellar")     return AbilityType::BlackHole;
  if (gift == "Planet")           return AbilityType::TimeWarp;
  if (gift == "Rocket")           return AbilityType::NukeStrike;
  if (gift == "Lion")             return AbilityType::PhoenixRevive;
  if (gift == "TikTok Universe")  return AbilityType::NukeStrike;

  // Legacy aliases that older builds and the original mapping used.
  if (gift == "Galaxy")           return AbilityType::MeteorStorm;
  if (gift == "GG")               return AbilityType::Blaster;
  if (gift == "Chainsaw")         return AbilityType::ChainsawArmor;
  if (gift == "Shamrock")         return AbilityType::RocketBoost;
  if (gift == "Rosa")             return AbilityType::HealBurst;

  return AbilityType::None;
}

static AbilityType AbilityFromTier(int diamondCount)
{
  if (diamondCount >= 20000) return AbilityType::TimeWarp;
  if (diamondCount >=  5000) return AbilityType::NukeStrike;
  if (diamondCount >=  2000) return AbilityType::BlackHole;
  if (diamondCount >=  1000) return AbilityType::Vortex;
  if (diamondCount >=   500) return AbilityType::MeteorStorm;
  if (diamondCount >=   200) return AbilityType::ChainsawArmor;
  if (diamondCount >=   100) return AbilityType::LightningBurst;
  if (diamondCount >=    30) return AbilityType::ShieldBubble;
  if (diamondCount >=     5) return AbilityType::FrostNova;
  if (diamondCount >=     1) return AbilityType::HealBurst;
  return AbilityType::None;
}

AbilityType AbilityFromGift(const std::string& gift, int diamondCount) {
  AbilityType t = AbilityFromName(gift);
  if (t != AbilityType::None) return t;
  return AbilityFromTier(diamondCount);
}

int GetAuraVisualKind(AbilityType t) {
  switch (t) {
  case AbilityType::FrostNova:      return 1;
  case AbilityType::Inferno:        return 2;
  case AbilityType::Vortex:         return 3;
  case AbilityType::BlackHole:      return 4;
  case AbilityType::LightningBurst: return 5;
  case AbilityType::ShieldBubble:   return 6;
  case AbilityType::HealBurst:      return 7;
  case AbilityType::Berserk:        return 8;
  case AbilityType::TimeWarp:       return 9;
  case AbilityType::PhoenixRevive:  return 10;
  case AbilityType::RocketBoost:    return 11;
  case AbilityType::GravityWell:    return 12;
  case AbilityType::ChainsawArmor:  return 13;
  case AbilityType::Shockwave:      return 4;  // reuse accretion ring look
  case AbilityType::NukeStrike:     return 8;  // reuse jagged shards
  case AbilityType::Blaster:        return 11; // reuse forward halo
  case AbilityType::MeteorStorm:    return 2;  // reuse ember pattern
  default: return 0;
  }
}

bool GetAbilityWorldDistort(AbilityType t, int& outType,
                            float& outRadius, float& outStrength) {
  switch (t) {
  case AbilityType::BlackHole:
    outType = 0; outRadius = 360.0f; outStrength = 0.95f; return true;
  case AbilityType::TimeWarp:
    outType = 1; outRadius = 700.0f; outStrength = 0.85f; return true;
  case AbilityType::NukeStrike:
    outType = 2; outRadius = 800.0f; outStrength = 0.95f; return true;
  case AbilityType::Vortex:
    outType = 3; outRadius = 220.0f; outStrength = 0.80f; return true;
  default: return false;
  }
}
