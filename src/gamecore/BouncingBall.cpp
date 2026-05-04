#include "gamecore/BouncingBall.h"
#include "raylib.h"
#include "tools/EngineConfig.h"
#include "network/WebSocketClient.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

static Font& GetSharedUIFont()
{
  static Font font{};
  static bool loaded = false;
  if (!loaded)
  {
    font = LoadFont("../assets/fonts/Roboto/Roboto.ttf");
    loaded = true;
  }
  return font;
}

namespace {

void DrawAbilityBackdrop(const Ball& ball) {
  if (ball.ability.active == AbilityType::None)
    return;
  const float t = (float)GetTime();
  const Vector2 c = ball.position;
  const float r = ball.radius;

  switch (ball.ability.active) {
  case AbilityType::FrostNova: {
    const float pulse = 0.92f + 0.08f * sinf(t * 5.0f);
    for (int i = 3; i >= 0; --i) {
      const float ri = r + 18.0f + (float)i * 28.0f * pulse;
      const Color col = {100, 200, 255, (unsigned char)(28 - i * 6)};
      DrawCircleLinesV(c, ri, col);
    }
    DrawCircleV(c, r + 6.0f, Fade({160, 220, 255, 255}, 0.14f));
    break;
  }
  case AbilityType::GravityWell: {
    DrawCircleV(c, r + 55.0f, Fade({15, 40, 30, 255}, 0.5f));
    DrawCircleV(c, r + 34.0f, Fade({30, 80, 55, 255}, 0.28f));
    for (int arm = 0; arm < 5; ++arm) {
      const float a0 = t * 2.8f + (float)arm * (2.0f * PI / 5.0f);
      for (int s = 0; s < 12; ++s) {
        const float dist = r + 12.0f + (float)s * 7.0f;
        const float a = a0 + (float)s * 0.35f;
        const Vector2 p = {c.x + cosf(a) * dist, c.y + sinf(a) * dist};
        DrawCircleV(p, 2.0f, Fade({120, 255, 190, 255}, 0.38f));
      }
    }
    break;
  }
  case AbilityType::ShieldBubble:
    DrawCircleV(c, r + 14.0f, Fade({140, 200, 255, 255}, 0.2f));
    DrawCircleV(c, r + 24.0f, Fade({100, 160, 255, 255}, 0.1f));
    break;
  case AbilityType::Inferno: {
    DrawCircleV(c, r + 28.0f, Fade({255,  60,  20, 255}, 0.30f));
    DrawCircleV(c, r + 50.0f, Fade({255, 120,  40, 255}, 0.18f));
    DrawCircleV(c, r + 70.0f, Fade({255, 180,  60, 255}, 0.10f));
    const int embers = 18;
    for (int i = 0; i < embers; ++i) {
      const float a = t * 1.4f + (float)i * (2.0f * PI / embers);
      const float wob = 0.6f + 0.4f * sinf(t * 3.0f + (float)i);
      const float dist = r + 20.0f + wob * 28.0f;
      const Vector2 p = {c.x + cosf(a) * dist, c.y + sinf(a) * dist};
      DrawCircleV(p, 2.5f, Fade({255, 200, 80, 255}, 0.55f));
    }
    break;
  }
  case AbilityType::BlackHole: {
    DrawCircleV(c, r + 95.0f, Fade({30,  0,  60, 255}, 0.30f));
    DrawCircleV(c, r + 60.0f, Fade({10,  0,  20, 255}, 0.55f));
    for (int arm = 0; arm < 7; ++arm) {
      const float a0 = -t * 4.5f + (float)arm * (2.0f * PI / 7.0f);
      for (int s = 0; s < 16; ++s) {
        const float dist = r + 8.0f + (float)s * 6.0f;
        const float a = a0 + (float)s * 0.42f;
        const Vector2 p = {c.x + cosf(a) * dist, c.y + sinf(a) * dist};
        const unsigned char al = (unsigned char)(220 - s * 12);
        DrawCircleV(p, 2.2f, {200, 100, 255, al});
      }
    }
    break;
  }
  case AbilityType::TimeWarp: {
    DrawCircleV(c, r + 70.0f, Fade({120, 150, 255, 255}, 0.18f));
    DrawCircleV(c, r + 46.0f, Fade({130, 180, 255, 255}, 0.22f));
    for (int i = 0; i < 12; ++i) {
      const float a = (float)i * (PI / 6.0f);
      const Vector2 a0 = {c.x + cosf(a) * (r + 18.0f), c.y + sinf(a) * (r + 18.0f)};
      const Vector2 a1 = {c.x + cosf(a) * (r + 28.0f), c.y + sinf(a) * (r + 28.0f)};
      DrawLineEx(a0, a1, 2.0f, Fade({200, 220, 255, 255}, 0.65f));
    }
    break;
  }
  case AbilityType::PhoenixRevive: {
    DrawCircleV(c, r + 30.0f, Fade({255, 200,  80, 255}, 0.22f));
    DrawCircleV(c, r + 18.0f, Fade({255, 220, 120, 255}, 0.30f));
    for (int i = 0; i < 8; ++i) {
      const float a = t * 2.2f + (float)i * (PI / 4.0f);
      const float len = r + 28.0f + 6.0f * sinf(t * 3.0f + (float)i);
      const Vector2 tip = {c.x + cosf(a) * len, c.y + sinf(a) * len};
      DrawLineEx(c, tip, 1.8f, Fade({255, 220, 140, 255}, 0.45f));
    }
    break;
  }
  case AbilityType::RocketBoost: {
    Vector2 v = ball.velocity;
    const float vlen = sqrtf(v.x * v.x + v.y * v.y);
    if (vlen > 0.001f) { v.x /= vlen; v.y /= vlen; }
    for (int i = 0; i < 6; ++i) {
      const float f = (float)i / 6.0f;
      const Vector2 tail = {c.x - v.x * (r + 6.0f + f * 40.0f),
                            c.y - v.y * (r + 6.0f + f * 40.0f)};
      const float jitter = sinf(t * 30.0f + (float)i) * 4.0f;
      const Vector2 perp = {-v.y * jitter, v.x * jitter};
      const Vector2 p = {tail.x + perp.x, tail.y + perp.y};
      DrawCircleV(p, (r * 0.7f) * (1.0f - f), Fade({120, 200, 255, 255}, 0.70f - f * 0.5f));
    }
    break;
  }
  case AbilityType::Vortex: {
    DrawCircleV(c, r + 36.0f, Fade({120,  60, 180, 255}, 0.18f));
    DrawCircleV(c, r + 22.0f, Fade({150,  80, 220, 255}, 0.28f));
    break;
  }
  default:
    break;
  }
}

void DrawShockwaveRings(const Ball& ball) {
  const bool isShock = (ball.ability.active == AbilityType::Shockwave);
  const bool isNuke  = (ball.ability.active == AbilityType::NukeStrike);
  if (!isShock && !isNuke) return;
  if (ball.ability.shockRadius < 0.0f) return;

  const Vector2 center = ball.ability.shockCenter;
  const float R = ball.ability.shockRadius;
  const float maxR = isNuke ? 1400.0f : 720.0f;
  const int bands = isNuke ? 6 : 4;
  const float bandStep = isNuke ? 64.0f : 42.0f;
  const float thick = isNuke ? 12.0f : 6.0f;
  const Color col = isNuke ? Color{255, 180, 80, 255} : Color{220, 130, 255, 255};

  for (int band = 0; band < bands; ++band) {
    const float ri = R - (float)band * bandStep;
    if (ri < 10.0f) continue;
    const float falloff = fmaxf(0.0f, 1.0f - ri / maxR);
    const unsigned char alpha =
        (unsigned char)(230.0f * falloff * (1.0f - (float)band * 0.14f));
    Color c2 = col;
    c2.a = alpha;
    DrawRing(center, ri - thick, ri, 0.0f, 360.0f, 64, c2);
  }

  if (isNuke) {
    const float coreR = fmaxf(8.0f, 60.0f - R * 0.05f);
    DrawCircleV(center, coreR, Fade({255, 240, 200, 255}, 0.85f));
    DrawCircleV(center, coreR * 0.55f, Fade(WHITE, 0.95f));
  } else {
    DrawCircleV(center, 10.0f, Fade({255, 220, 255, 255}, 0.55f));
  }
}

void DrawAbilityForeground(const Ball& ball) {
  if (ball.ability.active == AbilityType::None)
    return;
  const float t = (float)GetTime();
  const Vector2 c = ball.position;
  const float r = ball.radius;

  switch (ball.ability.active) {
  case AbilityType::FrostNova: {
    const int spikes = 14;
    const float spin = t * 80.0f * DEG2RAD;
    for (int i = 0; i < spikes; ++i) {
      const float a = spin + (float)i * (2.0f * PI / spikes);
      const Vector2 o = {c.x + cosf(a) * (r + 8.0f), c.y + sinf(a) * (r + 8.0f)};
      const Vector2 tip = {c.x + cosf(a) * (r + 28.0f), c.y + sinf(a) * (r + 28.0f)};
      DrawLineEx(o, tip, 2.5f, Fade({200, 240, 255, 255}, 0.88f));
    }
    DrawRing(c, r + 4.0f, r + 7.0f, 0.0f, 360.0f, 32, Fade({150, 230, 255, 255}, 0.92f));
    break;
  }
  case AbilityType::LightningBurst:
    for (int bolt = 0; bolt < 6; ++bolt) {
      const float seed = (float)bolt * 19.7f + t * 47.0f;
      Vector2 prev = c;
      float baseAng = seed;
      for (int seg = 0; seg < 7; ++seg) {
        baseAng += sinf(seed + (float)seg * 1.1f + t * 22.0f) * 0.55f;
        const float dist = r + 6.0f + (float)seg * 9.0f;
        const Vector2 next = {c.x + cosf(baseAng) * dist, c.y + sinf(baseAng) * dist};
        const float thick = 3.5f - (float)seg * 0.35f;
        const Color lc =
            (seg % 2 == 0) ? Color{255, 255, 200, 255} : Color{255, 220, 80, 255};
        DrawLineEx(prev, next, fmaxf(1.2f, thick), Fade(lc, 0.95f));
        prev = next;
      }
    }
    DrawCircleV(c, r * 0.4f, Fade(YELLOW, 0.38f));
    break;
  case AbilityType::GravityWell:
    DrawRing(c, r + 8.0f, r + 11.0f, 0.0f, 360.0f, 48, Fade({150, 255, 200, 255}, 0.78f));
    for (int k = 0; k < 3; ++k) {
      const float arcStart = t * (120.0f + (float)k * 40.0f) + (float)k * 120.0f;
      DrawRing(c, r + 20.0f + (float)k * 14.0f, r + 22.0f + (float)k * 14.0f, arcStart,
               arcStart + 95.0f, 24, Fade({120, 255, 180, 255}, 0.58f - (float)k * 0.12f));
    }
    break;
  case AbilityType::Berserk: {
    const float rage = r + 9.0f + 4.0f * sinf(t * 14.0f);
    DrawRing(c, rage, rage + 4.0f, 0.0f, 360.0f, 40, Fade({255, 60, 40, 255}, 0.88f));
    const int flares = 12;
    for (int i = 0; i < flares; ++i) {
      const float a = t * 10.0f + (float)i * (2.0f * PI / flares);
      const float len = 14.0f + (float)((i * 7) % 5);
      const Vector2 o = {c.x + cosf(a) * (r + 6.0f), c.y + sinf(a) * (r + 6.0f)};
      const Vector2 tip = {c.x + cosf(a) * (r + 6.0f + len), c.y + sinf(a) * (r + 6.0f + len)};
      DrawLineEx(o, tip, 3.0f, Fade({255, 150, 50, 255}, 0.78f));
    }
    break;
  }
  case AbilityType::HealBurst: {
    const float pulse = sinf(t * 28.0f);
    const float ring = r + 12.0f + pulse * 5.0f;
    DrawRing(c, ring, ring + 3.0f, 0.0f, 360.0f, 32, Fade({100, 255, 140, 255}, 0.92f));
    for (int i = 0; i < 4; ++i) {
      const float a = (float)i * (PI * 0.5f) + t * 3.0f;
      const float d = r + 22.0f;
      const Vector2 p = {c.x + cosf(a) * d, c.y + sinf(a) * d};
      DrawCircleV(p, 4.0f, Fade({180, 255, 190, 255}, 0.88f));
      DrawLineEx(c, p, 2.0f, Fade({120, 255, 150, 255}, 0.62f));
    }
    break;
  }
  case AbilityType::ChainsawArmor: {
    const float rot = t * 18.0f;
    const int teeth = 12;
    const float outer = r + 18.0f;
    for (int i = 0; i < teeth; ++i) {
      const float a0 = rot + (float)i * (2.0f * PI / teeth);
      const float a1 = a0 + 0.12f;
      const Vector2 baseL = {c.x + cosf(a0) * (r + 4.0f), c.y + sinf(a0) * (r + 4.0f)};
      const Vector2 baseR = {c.x + cosf(a1) * (r + 4.0f), c.y + sinf(a1) * (r + 4.0f)};
      const Vector2 tip = {c.x + cosf((a0 + a1) * 0.5f) * outer, c.y + sinf((a0 + a1) * 0.5f) * outer};
      DrawTriangle(baseL, baseR, tip, Fade({255, 140, 40, 255}, 0.92f));
    }
    DrawRing(c, r + 3.0f, r + 5.0f, 0.0f, 360.0f, 24, Fade({80, 80, 90, 255}, 0.92f));
    break;
  }
  case AbilityType::Blaster: {
    const float w = r + 10.0f + 2.0f * sinf(t * 20.0f);
    DrawRing(c, w, w + 3.0f, 0.0f, 360.0f, 24, Fade({255, 210, 80, 255}, 0.72f));
    DrawCircleLinesV(c, r + 22.0f, Fade({255, 230, 150, 255}, 0.4f));
    break;
  }
  case AbilityType::MeteorStorm: {
    const float spin = t * 360.0f * DEG2RAD;
    for (int i = 0; i < 6; ++i) {
      const float a = spin + (float)i * (2.0f * PI / 6.0f);
      const Vector2 spark = {c.x + cosf(a) * (r + 16.0f), c.y + sinf(a) * (r + 16.0f)};
      DrawLineEx(c, spark, 2.0f, Fade({255, 100, 160, 255}, 0.68f));
    }
    DrawRing(c, r + 6.0f, r + 10.0f, 0.0f, 360.0f, 36, Fade({255, 80, 140, 255}, 0.58f));
    break;
  }
  case AbilityType::Shockwave:
    DrawRing(c, r + 6.0f, r + 9.0f, 0.0f, 360.0f, 24, Fade({220, 150, 255, 255}, 0.88f));
    break;
  case AbilityType::Inferno: {
    const int tongues = 10;
    const float spin = t * 1.5f;
    for (int i = 0; i < tongues; ++i) {
      const float a = spin + (float)i * (2.0f * PI / tongues);
      const float wob = 4.0f * sinf(t * 9.0f + (float)i);
      const Vector2 base = {c.x + cosf(a) * (r + 4.0f), c.y + sinf(a) * (r + 4.0f)};
      const Vector2 mid  = {c.x + cosf(a + 0.18f) * (r + 14.0f + wob),
                            c.y + sinf(a + 0.18f) * (r + 14.0f + wob)};
      const Vector2 tip  = {c.x + cosf(a) * (r + 26.0f + wob * 0.8f),
                            c.y + sinf(a) * (r + 26.0f + wob * 0.8f)};
      DrawTriangle(base, mid, tip, Fade({255,  80,  20, 255}, 0.85f));
    }
    DrawRing(c, r + 4.0f, r + 6.0f, 0.0f, 360.0f, 32, Fade({255, 220, 120, 255}, 0.88f));
    break;
  }
  case AbilityType::BlackHole: {
    DrawRing(c, r + 4.0f, r + 7.0f, 0.0f, 360.0f, 48, Fade({255, 200, 255, 255}, 0.90f));
    for (int k = 0; k < 4; ++k) {
      const float as = -t * (200.0f + (float)k * 80.0f) + (float)k * 90.0f;
      DrawRing(c, r + 14.0f + (float)k * 8.0f, r + 16.0f + (float)k * 8.0f, as,
               as + 70.0f, 36, Fade({230, 150, 255, 255}, 0.78f - (float)k * 0.16f));
    }
    DrawCircleV(c, r * 0.45f, Fade(BLACK, 0.85f));
    break;
  }
  case AbilityType::TimeWarp: {
    const float hour = t * 28.0f;
    const float minute = t * 220.0f;
    const Vector2 hourTip = {c.x + cosf(hour * DEG2RAD) * (r + 14.0f),
                             c.y + sinf(hour * DEG2RAD) * (r + 14.0f)};
    const Vector2 minTip  = {c.x + cosf(minute * DEG2RAD) * (r + 22.0f),
                             c.y + sinf(minute * DEG2RAD) * (r + 22.0f)};
    DrawLineEx(c, hourTip, 3.5f, Fade({230, 230, 255, 255}, 0.92f));
    DrawLineEx(c, minTip,  2.5f, Fade({200, 220, 255, 255}, 0.92f));
    DrawCircleV(c, 4.0f, Fade(WHITE, 0.85f));
    DrawRing(c, r + 8.0f, r + 11.0f, 0.0f, 360.0f, 32, Fade({140, 170, 255, 255}, 0.80f));
    break;
  }
  case AbilityType::RocketBoost: {
    Vector2 v = ball.velocity;
    const float vlen = sqrtf(v.x * v.x + v.y * v.y);
    if (vlen > 0.001f) { v.x /= vlen; v.y /= vlen; }
    const Vector2 perp = {-v.y, v.x};
    const Vector2 nozzleA = {c.x - v.x * (r + 2.0f) + perp.x * 6.0f,
                             c.y - v.y * (r + 2.0f) + perp.y * 6.0f};
    const Vector2 nozzleB = {c.x - v.x * (r + 2.0f) - perp.x * 6.0f,
                             c.y - v.y * (r + 2.0f) - perp.y * 6.0f};
    const float flame = 30.0f + 6.0f * sinf(t * 40.0f);
    const Vector2 flameTip = {c.x - v.x * (r + flame), c.y - v.y * (r + flame)};
    DrawTriangle(nozzleB, nozzleA, flameTip, Fade({120, 200, 255, 255}, 0.85f));
    DrawCircleV(flameTip, 6.0f, Fade(WHITE, 0.85f));
    break;
  }
  case AbilityType::PhoenixRevive: {
    const float pulse = 0.85f + 0.15f * sinf(t * 6.0f);
    DrawRing(c, r + 5.0f, r + 8.0f, 0.0f, 360.0f, 32, Fade({255, 200, 80, 255}, 0.95f * pulse));
    for (int wing = 0; wing < 6; ++wing) {
      const float a = -PI * 0.5f + ((float)wing - 2.5f) * 0.25f;
      const float len = r + 26.0f + 4.0f * sinf(t * 5.0f + (float)wing);
      const Vector2 mid = {c.x + cosf(a) * (r + 8.0f), c.y + sinf(a) * (r + 8.0f)};
      const Vector2 tip = {c.x + cosf(a) * len,         c.y + sinf(a) * len};
      DrawLineEx(mid, tip, 3.0f, Fade({255, 230, 140, 255}, 0.88f));
    }
    break;
  }
  case AbilityType::Vortex: {
    const int blades = 4;
    const float rot = t * 6.0f;
    for (int i = 0; i < blades; ++i) {
      const float a0 = rot + (float)i * (2.0f * PI / blades);
      const int segs = 14;
      Vector2 prev = {c.x + cosf(a0) * (r + 6.0f), c.y + sinf(a0) * (r + 6.0f)};
      for (int s = 1; s <= segs; ++s) {
        const float f = (float)s / (float)segs;
        const float a = a0 + f * 1.6f;
        const float dist = r + 6.0f + f * 30.0f;
        const Vector2 next = {c.x + cosf(a) * dist, c.y + sinf(a) * dist};
        DrawLineEx(prev, next, 2.5f, Fade({220, 150, 255, 255}, 0.85f - f * 0.45f));
        prev = next;
      }
    }
    DrawRing(c, r + 4.0f, r + 6.0f, 0.0f, 360.0f, 32, Fade({240, 200, 255, 255}, 0.55f));
    break;
  }
  case AbilityType::NukeStrike: {
    const float pulse = 0.6f + 0.4f * sinf(t * 18.0f);
    DrawRing(c, r + 4.0f, r + 9.0f, 0.0f, 360.0f, 24, Fade({255, 200, 80, 255}, pulse));
    DrawCircleV(c, r * 0.4f, Fade(WHITE, 0.55f));
    break;
  }
  default:
    break;
  }
}

void DrawAbilityNameplate(const Ball& ball) {
  if (ball.ability.active == AbilityType::None)
    return;
  const AbilitySpec& spec = GetAbilitySpec(ball.ability.active);
  Font& font = GetSharedUIFont();
  const float fs = 17.0f;
  Vector2 textPos = {ball.position.x, ball.position.y + ball.radius + 8.0f};
  const char* name = spec.name;
  const Vector2 sz = MeasureTextEx(font, name, fs, 1.0f);
  textPos.x -= sz.x * 0.5f;
  DrawTextEx(font, name, {textPos.x + 2.0f, textPos.y + 2.0f}, fs, 1.0f, {0, 0, 0, 200});
  DrawTextEx(font, name, textPos, fs, 1.0f, spec.uiColor);
}

} // namespace

void BouncingBall::Init(Vector2 startPos) {
  ball.position = startPos;

  float speed = 400.0f;
  float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;

  ball.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};

  ball.radius = 25.0f;
  ball.color = BLUE;

  ball.health = 100.0f;
  ball.maxHealth = 100.0f;
  ball.alive = true;
  ball.lastHitByUserId.clear();
  ball.lastHitAtSeconds = -1.0;
  ball.respawnTimer = 0.0f;
  ball.invulnTimer = 0.0f;
}

void BouncingBall::Update() {
  if (!ball.alive) {
    ball.respawnTimer -= EngineConfig::dt;
    if (ball.respawnTimer <= 0.0f) {
      // Respawn in a random position with brief invulnerability.
      const int w = (EngineConfig::WindowWidth > 0) ? EngineConfig::WindowWidth : 1;
      const int h = (EngineConfig::WindowHeight > 0) ? EngineConfig::WindowHeight : 1;
      ball.position = {(float)(rand() % w), (float)(rand() % h)};

      float speed = 350.0f;
      float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
      ball.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};

      ball.health = ball.maxHealth;
      ball.shield = 0.0f;
      ball.hitTimer = 0.0f;
      ball.alive = true;
      ball.invulnTimer = 1.5f;
    }
    return;
  }

  // ability timers
  if (ball.ability.cooldownTimer > 0.0f) {
    ball.ability.cooldownTimer -= EngineConfig::dt;
    if (ball.ability.cooldownTimer < 0.0f) ball.ability.cooldownTimer = 0.0f;
  }
  if (ball.ability.active != AbilityType::None) {
    ball.ability.activeTimer -= EngineConfig::dt;
    if (ball.ability.activeTimer <= 0.0f) {
      ball.ability.active = AbilityType::None;
      ball.ability.activeTimer = 0.0f;
      ball.ability.shockRadius = -1.0f;
    }
  }

  // Continuous ability effects (self-only). Auras affecting other balls are
  // handled in Application::ApplyAbilityAuras so we have access to neighbours.
  if (ball.ability.active == AbilityType::Berserk) {
    // More aggressive movement.
    ball.velocity.x *= 1.004f;
    ball.velocity.y *= 1.004f;
  }

  // Continuous self-buffs that simply scale movement.
  if (ball.ability.active == AbilityType::RocketBoost) {
    ball.velocity.x *= 1.012f;
    ball.velocity.y *= 1.012f;
  }

  // Expanding-wave abilities (Shockwave, NukeStrike). Damage is applied in
  // Application as the ring sweeps past targets; we record the previous radius
  // so the sweep window can be computed there.
  const bool waveActive =
      (ball.ability.active == AbilityType::Shockwave ||
       ball.ability.active == AbilityType::NukeStrike) &&
      ball.ability.shockRadius >= 0.0f;
  if (waveActive) {
    ball.ability.prevShockRadius = ball.ability.shockRadius;
    const float speed =
        (ball.ability.active == AbilityType::NukeStrike) ? 1500.0f : 900.0f;
    ball.ability.shockRadius += speed * EngineConfig::dt;
  }

  if (ball.invulnTimer > 0.0f) {
    ball.invulnTimer -= EngineConfig::dt;
    if (ball.invulnTimer < 0.0f) ball.invulnTimer = 0.0f;
  }

  ball.position.x += ball.velocity.x * EngineConfig::dt * ball.speedMultiplier;
  ball.position.y += ball.velocity.y * EngineConfig::dt * ball.speedMultiplier;

  // damping
  ball.velocity.x *= 0.998f;
  ball.velocity.y *= 0.998f;

  // keep movement alive
  float minSpeed = 80.0f;
  float speed = sqrt(ball.velocity.x * ball.velocity.x +
                     ball.velocity.y * ball.velocity.y);

  if (speed < minSpeed && speed > 0.0f) {
    ball.velocity.x *= (minSpeed / speed);
    ball.velocity.y *= (minSpeed / speed);
  }

  // walls
  if (ball.position.x - ball.radius < 0) {
    ball.position.x = ball.radius;
    ball.velocity.x *= -1;
  }
  if (ball.position.x + ball.radius > EngineConfig::WindowWidth) {
    ball.position.x = EngineConfig::WindowWidth - ball.radius;
    ball.velocity.x *= -1;
  }
  if (ball.position.y - ball.radius < 0) {
    ball.position.y = ball.radius;
    ball.velocity.y *= -1;
  }
  if (ball.position.y + ball.radius > EngineConfig::WindowHeight) {
    ball.position.y = EngineConfig::WindowHeight - ball.radius;
    ball.velocity.y *= -1;
  }

  // timers
  ball.speedTimer -= EngineConfig::dt;
  ball.damageTimer -= EngineConfig::dt;
  ball.shieldTimer -= EngineConfig::dt;

  if (ball.speedTimer <= 0)
    ball.speedMultiplier = 1.0f;
  if (ball.damageTimer <= 0)
    ball.damageMultiplier = 1.0f;
  if (ball.shieldTimer <= 0)
    ball.shield = 0.0f;

  ball.hitTimer -= EngineConfig::dt;
  if (ball.hitTimer < 0)
    ball.hitTimer = 0;

  UpdateParticles();
}

void BouncingBall::Draw() {
  if (!ball.alive)
    return;

  DrawParticles();

  DrawAbilityBackdrop(ball);

  Color drawColor = (ball.hitTimer > 0) ? RED : ball.color;
  if (ball.invulnTimer > 0.0f) {
    drawColor.a = 180;
  }

  // Player body is a square for readability on stream.
  const float s = ball.radius * 2.0f;
  Rectangle body = {ball.position.x - ball.radius, ball.position.y - ball.radius, s, s};
  DrawRectangleRounded(body, 0.20f, 8, {20, 20, 28, 220});
  DrawRectangleRoundedLines(body, 0.20f, 8, {255, 255, 255, 40});
  //DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, Color color)

  // Avatar (round masked) drawn inside the square.
  auto &cache = GetTextureCache();
  auto it = cache.find(ball.pfpPath);
  Rectangle dst = {ball.position.x - ball.radius, ball.position.y - ball.radius, s, s};

  if (it != cache.end() && it->second.id != 0) {
    // Circle mask shader is owned/managed by Application; fall back to unmasked if not set.
    // We detect it by checking the global shader id via GetShaderLocation is not available here,
    // so we keep a simple unmasked draw and rely on the square rounded body for shape.
    DrawTexturePro(it->second,
                   {0, 0, (float)it->second.width, (float)it->second.height},
                   dst, {0, 0}, 0.0f, {255, 255, 255, drawColor.a});

    DrawCircleLines((int)ball.position.x, (int)ball.position.y, ball.radius - 1.0f, {255,255,255,70});
  } else {
    DrawCircleV(ball.position, ball.radius - 2.0f, drawColor);
  }

  if (ball.invulnTimer > 0.0f) {
    DrawRing(ball.position, ball.radius + 3.0f, ball.radius + 6.0f, 0.0f, 360.0f, 24, {255, 255, 255, 120});
  }

  DrawShockwaveRings(ball);
  DrawAbilityForeground(ball);

  if (ball.ability.active == AbilityType::None && ball.ability.cooldownTimer > 0.0f) {
    Color c = {200, 200, 200, 80};
    DrawRing(ball.position, ball.radius + 7.0f, ball.radius + 9.0f, 0.0f, 360.0f, 24, c);
  }

  DrawAbilityNameplate(ball);

  // username
  int fontSize = 20;
  int textWidth = MeasureText(ball.username.c_str(), fontSize);

  Font& font = GetSharedUIFont();
  DrawTextEx(
      font, ball.username.c_str(),
      {ball.position.x - textWidth / 2, ball.position.y - ball.radius - 25},
      (float)fontSize,
      1.0f, // spacing
      BLACK);

  // health bar
  float barWidth = ball.radius * 2;
  float healthRatio = ball.health / ball.maxHealth;

  DrawRectangle(ball.position.x - ball.radius,
                ball.position.y - ball.radius - 10, barWidth, 5, DARKGRAY);

  DrawRectangle(ball.position.x - ball.radius,
                ball.position.y - ball.radius - 10, barWidth * healthRatio, 5,
                GREEN);
}

float BouncingBall::OnCollision(const std::string& attackerUserId,
                                double nowSeconds,
                                Vector2 point,
                                float strength) {
  ball.hitTimer = 0.1f;

  if (!ball.alive) return 0.0f;
  if (ball.invulnTimer > 0.0f) {
    SpawnParticles(point, strength * 0.25f);
    return 0.0f;
  }

  float damage = strength * 25.0f * ball.damageMultiplier;
  if (damage < 0.0f) damage = 0.0f;

  // Ability modifiers
  if (ball.ability.active == AbilityType::FrostNova) {
    damage *= 0.75f;
  }
  if (ball.ability.active == AbilityType::Berserk) {
    damage *= 1.35f;
  }
  if (ball.ability.active == AbilityType::ChainsawArmor) {
    // Tanky: takes reduced damage while chainsaw is up.
    damage *= 0.55f;
  }

  // shield absorbs first
  if (ball.shield > 0) {
    ball.shield -= damage;
    if (ball.shield < 0) {
      damage = -ball.shield;
      ball.shield = 0;
    } else {
      damage = 0;
    }
  }

  ball.health -= damage;

  // Phoenix Revive: cheat death once per activation.
  if (ball.health <= 0.0f && ball.ability.phoenixArmed) {
    ball.health = ball.maxHealth;
    ball.shield = 60.0f;
    ball.shieldTimer = 5.0f;
    ball.invulnTimer = 2.0f;
    ball.ability.phoenixArmed = false;
    ball.ability.activeTimer = 0.0f;
    ball.ability.active = AbilityType::None;

    for (int i = 0; i < 60; ++i) {
      float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
      float sp = 200.0f + ((float)rand() / RAND_MAX) * 600.0f;
      Particle p;
      p.position = ball.position;
      p.velocity = {std::cos(angle) * sp, std::sin(angle) * sp};
      p.life = 0.0f;
      p.maxLife = 0.85f;
      p.size = 2.5f + ((float)rand() / RAND_MAX) * 4.0f;
      p.color = {255, 200, 80, 255};
      particles.push_back(p);
    }
    return damage;
  }

  if (ball.health <= 0) {
    ball.alive = false;
    ball.respawnTimer = 6.0f;

    // Blood-pop (not too gory): burst of red/orange particles.
    for (int i = 0; i < 38; i++) {
      float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
      float speed = 120.0f + ((float)rand() / RAND_MAX) * 520.0f;
      Particle p;
      p.position = ball.position;
      p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
      p.life = 0;
      p.maxLife = 0.65f;
      p.size = 2.0f + ((float)rand() / RAND_MAX) * 5.0f;
      p.color = {220, 60, 70, 255};
      particles.push_back(p);
    }
  }

  SpawnParticles(point, strength);

  if (!attackerUserId.empty() && attackerUserId != ball.userId && damage > 0.0f)
  {
    ball.lastHitByUserId = attackerUserId;
    ball.lastHitAtSeconds = nowSeconds;
  }

  return damage;
}

void BouncingBall::ApplyGift(const std::string &gift, int amount, int diamondCount) {

  const AbilityType ability = AbilityFromGift(gift, diamondCount);
  if (ability != AbilityType::None) {
    const AbilitySpec& spec = GetAbilitySpec(ability);

    // Live override: as long as the player is alive, every new gift swaps
    // their active ability immediately. Cooldown is informational only.
    if (ball.alive) {
      // Reset transient state from any previously active ability so a
      // half-finished shockwave/vortex doesn't keep ticking under the new one.
      ball.ability.shockRadius = -1.0f;
      ball.ability.prevShockRadius = -1.0f;

      ball.ability.active = ability;
      ball.ability.activeTimer = spec.duration;
      ball.ability.cooldownTimer = spec.cooldown;
      ball.ability.tickTimer = 0.0f;
      // Power scales with quantity AND with diamond cost so rare ultimates
      // received once still hit harder than a streak of cheap gifts.
      const float diamondScale = 1.0f + std::min(2.0f, (float)diamondCount / 5000.0f);
      ball.ability.power = (1.0f + 0.25f * (float)amount) * diamondScale;
      ball.ability.justActivated = true;

      switch (ability) {
      case AbilityType::HealBurst:
        ball.health += 35.0f * (float)amount;
        if (ball.health > ball.maxHealth) ball.health = ball.maxHealth;
        break;
      case AbilityType::ShieldBubble:
        ball.shield += 60.0f * (float)amount;
        ball.shieldTimer = spec.duration;
        break;
      case AbilityType::Shockwave:
        ball.ability.shockCenter = ball.position;
        ball.ability.shockRadius = 0.0f;
        ball.ability.prevShockRadius = 0.0f;
        break;
      case AbilityType::LightningBurst:
        ball.damageMultiplier = 1.4f;
        ball.damageTimer = 1.0f;
        break;
      case AbilityType::GravityWell:
        ball.knockbackMultiplier = 1.8f;
        break;
      case AbilityType::Berserk:
        ball.damageMultiplier = 1.6f;
        ball.speedMultiplier = 1.25f;
        ball.damageTimer = spec.duration;
        ball.speedTimer = spec.duration;
        break;
      case AbilityType::ChainsawArmor:
        ball.shield += 35.0f * (float)amount;
        ball.shieldTimer = spec.duration;
        break;
      case AbilityType::Inferno:
        ball.damageMultiplier = 1.25f;
        ball.damageTimer = spec.duration;
        break;
      case AbilityType::BlackHole:
        ball.knockbackMultiplier = 1.0f;
        ball.invulnTimer = std::max(ball.invulnTimer, 0.4f);
        break;
      case AbilityType::TimeWarp:
        ball.speedMultiplier = 1.4f;
        ball.speedTimer = spec.duration;
        break;
      case AbilityType::RocketBoost: {
        ball.speedMultiplier = 2.6f;
        ball.speedTimer = spec.duration;
        ball.damageMultiplier = 1.5f;
        ball.damageTimer = spec.duration;
        ball.knockbackMultiplier = 1.6f;
        const float speed = sqrtf(ball.velocity.x * ball.velocity.x +
                                  ball.velocity.y * ball.velocity.y);
        if (speed > 0.001f) {
          ball.velocity.x *= 2.4f;
          ball.velocity.y *= 2.4f;
        } else {
          float a = ((float)rand() / RAND_MAX) * 2.0f * PI;
          ball.velocity = {std::cos(a) * 600.0f, std::sin(a) * 600.0f};
        }
        break;
      }
      case AbilityType::PhoenixRevive:
        ball.ability.phoenixArmed = true;
        ball.shield += 25.0f;
        ball.shieldTimer = spec.duration;
        break;
      case AbilityType::Vortex:
        ball.knockbackMultiplier = 2.2f;
        break;
      case AbilityType::NukeStrike:
        ball.ability.shockCenter = ball.position;
        ball.ability.shockRadius = 0.0f;
        ball.ability.prevShockRadius = 0.0f;
        ball.invulnTimer = std::max(ball.invulnTimer, 0.6f);
        break;
      default:
        break;
      }
    }
    return;
  }
  (void)diamondCount;

  if (gift == "Rose") {
    ball.speedMultiplier = 1.2f;
    ball.speedTimer = 5.0f;
    ball.color = PINK;
  }

  else if (gift == "TikTok") {
    ball.velocity.x += 300.0f * amount;
    ball.velocity.y += 300.0f * amount;
    ball.color = GREEN;
  }

  else if (gift == "Finger Heart") {
    ball.health += 20.0f * amount;
    if (ball.health > ball.maxHealth)
      ball.health = ball.maxHealth;
  }

  else if (gift == "Perfume") {
    ball.shield += 50.0f * amount;
    ball.shieldTimer = 8.0f;
    ball.color = SKYBLUE;
  }

  else if (gift == "Galaxy") {
    ball.damageMultiplier = 2.0f;
    ball.damageTimer = 6.0f;
    ball.color = RED;
  }

  else if (gift == "Shamrock") {
    ball.speedMultiplier = 1.5f;
    ball.speedTimer = 6.0f;
  }

  else if (gift == "Rosa") {
    ball.health += 40.0f;
  }

  else if (gift == "Donut") {
    ball.radius += 5.0f;
  }

  else if (gift == "GG") {
    ball.knockbackMultiplier = 2.0f;
  }
}

void BouncingBall::SpawnParticles(Vector2 pos, float strength) {
  int count = 5 + (int)(strength * 10);

  for (int i = 0; i < count; i++) {
    float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
    float speed = ((float)rand() / RAND_MAX) * 200.0f;

    Particle p;
    p.position = pos;
    p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
    p.life = 0;
    p.maxLife = 0.5f;
    p.size = 3.0f;
    p.color = {255, 200, 50, 255};

    particles.push_back(p);
  }
}

void BouncingBall::UpdateParticles() {
  for (auto &p : particles) {
    p.life += EngineConfig::dt;
    p.position.x += p.velocity.x * EngineConfig::dt;
    p.position.y += p.velocity.y * EngineConfig::dt;
  }

  particles.erase(
      std::remove_if(particles.begin(), particles.end(),
                     [](Particle &p) { return p.life > p.maxLife; }),
      particles.end());
}

void BouncingBall::DrawParticles() {
  for (auto &p : particles) {
    float alpha = 1.0f - (p.life / p.maxLife);

    Color c = p.color;
    c.a = (unsigned char)(alpha * 255);
    DrawCircleV(p.position, p.size, c);
  }
}
