#include "core/Application.h"
#include "gamecore/BouncingBall.h"
#include "gamecore/Abilities.h"
#include "raylib.h"
#include "rlgl.h"
#include "tools/EngineConfig.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

Application::Application() {}

void Application::Init() {

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(EngineConfig::WindowWidth, EngineConfig::WindowHeight,
             "ChatBattle");

  wsClient = std::make_unique<WebSocketClient>();
  wsClient->Init("ws://127.0.0.1:8080");
  playerballs.clear();

  EnsureRenderTargets();
}

void Application::EnsureRenderTargets()
{
  const int w = (EngineConfig::WindowWidth > 0) ? EngineConfig::WindowWidth : GetScreenWidth();
  const int h = (EngineConfig::WindowHeight > 0) ? EngineConfig::WindowHeight : GetScreenHeight();
  if (w <= 0 || h <= 0) return;

  if (w == targetW && h == targetH && worldTarget.id != 0 && postShader.id != 0) return;

  if (worldTarget.id != 0) UnloadRenderTexture(worldTarget);
  if (worldTargetB.id != 0) UnloadRenderTexture(worldTargetB);
  if (postShader.id != 0) UnloadShader(postShader);
  if (avatarShader.id != 0) UnloadShader(avatarShader);
  if (auraShader.id != 0) UnloadShader(auraShader);
  if (distortShader.id != 0) UnloadShader(distortShader);
  if (whiteTex.id != 0) UnloadTexture(whiteTex);

  targetW = w;
  targetH = h;
  worldTarget  = LoadRenderTexture(targetW, targetH);
  worldTargetB = LoadRenderTexture(targetW, targetH);
  postShader    = LoadShader(nullptr, "assets/shaders/postprocess.fs");
  avatarShader  = LoadShader(nullptr, "assets/shaders/circle_mask.fs");
  auraShader    = LoadShader(nullptr, "assets/shaders/aura.fs");
  distortShader = LoadShader(nullptr, "assets/shaders/world_distort.fs");

  // 1x1 white texture for aura quads (so fragTexCoord runs 0..1 across the rect).
  Image whiteImg = GenImageColor(1, 1, WHITE);
  whiteTex = LoadTextureFromImage(whiteImg);
  UnloadImage(whiteImg);

  {
    int loc = GetShaderLocation(postShader, "resolution");
    if (loc >= 0) {
      float res[2] = {(float)targetW, (float)targetH};
      SetShaderValue(postShader, loc, res, SHADER_UNIFORM_VEC2);
    }
  }

  {
    int softLoc = GetShaderLocation(avatarShader, "softEdge");
    if (softLoc >= 0) {
      float soft = 0.03f;
      SetShaderValue(avatarShader, softLoc, &soft, SHADER_UNIFORM_FLOAT);
    }
  }

  auraColorLoc = GetShaderLocation(auraShader, "auraColor");
  auraTimeLoc  = GetShaderLocation(auraShader, "auraTime");
  auraTypeLoc  = GetShaderLocation(auraShader, "auraType");

  distResLoc      = GetShaderLocation(distortShader, "resolution");
  distTimeLoc     = GetShaderLocation(distortShader, "time");
  distCountLoc    = GetShaderLocation(distortShader, "fxCount");
  distCenterLoc   = GetShaderLocation(distortShader, "fxCenter");
  distRadiusLoc   = GetShaderLocation(distortShader, "fxRadius");
  distStrengthLoc = GetShaderLocation(distortShader, "fxStrength");
  distTypeLoc     = GetShaderLocation(distortShader, "fxType");

  if (distResLoc >= 0) {
    float res[2] = {(float)targetW, (float)targetH};
    SetShaderValue(distortShader, distResLoc, res, SHADER_UNIFORM_VEC2);
  }
}

static void DrawPanel(Rectangle r, Color bg, Color border)
{
  DrawRectangleRec(r, bg);
  DrawRectangleLinesEx(r, 2.0f, border);
}

void Application::DrawHUD()
{
  const int w = GetScreenWidth();
  const int h = GetScreenHeight();
  const double now = GetTime();

  const bool connected = wsClient ? wsClient->IsConnected() : false;

  // Phone-first scaling (TikTok portrait streams)
  const float baseW = 720.0f;
  const float scale = fmaxf(0.85f, fminf(1.35f, (float)w / baseW));
  const float pad = 12.0f * scale;
  const int font = (int)(18 * scale);
  const int fontSmall = (int)(16 * scale);

  // Status (top-left)
  Rectangle status = {pad, pad, 250 * scale, 66 * scale};
  //DrawPanel(status, {10, 10, 14, 160}, {255, 255, 255, 40});
  //DrawText(TextFormat("WS: %s", connected ? "connected" : "disconnected"), (int)status.x + (int)(10*scale), (int)status.y + (int)(10*scale), fontSmall, connected ? GREEN : RED);
  //DrawText(TextFormat("Players: %d", (int)playerballs.size()), (int)status.x + (int)(10*scale), (int)status.y + (int)(34*scale), fontSmall, RAYWHITE);

  // Top kills (top-center) - compact for portrait
  Rectangle kills = {(float)(w * 0.5f - 190.0f*scale), pad, 380*scale, 132*scale};
  DrawPanel(kills, {10, 10, 14, 140}, {255, 255, 255, 40});
  DrawText("Top Kills", (int)kills.x + (int)(10*scale), (int)kills.y + (int)(8*scale), fontSmall, RAYWHITE);
  auto topKills = stats.TopKills(5);
  for (int i = 0; i < (int)topKills.size(); ++i) {
    const auto* s = topKills[i];
    DrawText(TextFormat("%d) %s  K:%d  S:%d", i + 1, s->username.c_str(), s->kills, s->streak),
             (int)kills.x + (int)(10*scale), (int)kills.y + (int)(32*scale) + i * (int)(20*scale), fontSmall, {220, 220, 230, 255});
  }

  // Top damage (under kills in portrait, otherwise top-right)
  bool portrait = (h > w);
  Rectangle dmg = portrait
      ? Rectangle{kills.x, kills.y + kills.height + pad, kills.width, 132*scale}
      : Rectangle{(float)(w - pad - 360*scale), pad, 360*scale, 132*scale};
  //DrawPanel(dmg, {10, 10, 14, 140}, {255, 255, 255, 40});
  //DrawText("Top Damage", (int)dmg.x + (int)(10*scale), (int)dmg.y + (int)(8*scale), fontSmall, RAYWHITE);
  auto topDmg = stats.TopDamage(5);
  for (int i = 0; i < (int)topDmg.size(); ++i) {
    const auto* s = topDmg[i];
    //DrawText(TextFormat("%d) %s  D:%.0f  K:%d", i + 1, s->username.c_str(), s->damageDealt, s->kills),
             //(int)dmg.x + (int)(10*scale), (int)dmg.y + (int)(32*scale) + i * (int)(20*scale), fontSmall, {220, 220, 230, 255});
  }

  // Kill feed (bottom-left)
  /**Rectangle feed = {pad, (float)(h - pad - 170*scale), 460*scale, 170*scale};
  DrawPanel(feed, {10, 10, 14, 140}, {255, 255, 255, 40});
  DrawText("Kill Feed", (int)feed.x + (int)(10*scale), (int)feed.y + (int)(8*scale), fontSmall, RAYWHITE);
  int y = (int)feed.y + (int)(32*scale);
  for (const auto& e : killFeed.Get(now)) {
    DrawText(e.text.c_str(), (int)feed.x + (int)(10*scale), y, fontSmall, {240, 240, 245, 255});
    y += (int)(20*scale);
    if (y > feed.y + feed.height - (int)(22*scale)) break;
  }**/

  // Ability hint (bottom-right): show gift-only nature
  //Rectangle hint = {(float)(w - pad - 260*scale), (float)(h - pad - 62*scale), 260*scale, 62*scale};
  //DrawPanel(hint, {10, 10, 14, 120}, {255, 255, 255, 35});
  //DrawText("Abilities: gifts", (int)hint.x + (int)(10*scale), (int)hint.y + (int)(10*scale), fontSmall, RAYWHITE);
  //DrawText("Big gifts = chaos", (int)hint.x + (int)(10*scale), (int)hint.y + (int)(32*scale), fontSmall, {220,220,230,255});
}

void Application::UpdateProjectiles()
{
  const double now = GetTime();

  for (auto& p : projectiles) {
    if (!p.active) continue;
    p.life -= EngineConfig::dt;
    if (p.life <= 0.0f) { p.active = false; continue; }
    p.pos.x += p.vel.x * EngineConfig::dt;
    p.pos.y += p.vel.y * EngineConfig::dt;

    // collide vs players
    for (auto& b : playerballs) {
      if (!b) continue;
      if (!b->ball.alive) continue;
      if (b->ball.userId == p.ownerUserId) continue;

      float dx = b->ball.position.x - p.pos.x;
      float dy = b->ball.position.y - p.pos.y;
      float rr = b->ball.radius + p.radius;
      if (dx*dx + dy*dy <= rr*rr) {
        // Apply “projectile” damage by translating it into collision strength.
        // Use a strength that approximates p.damage (since OnCollision scales strength).
        float strength = p.damage / 25.0f;
        float dmg = b->OnCollision(p.ownerUserId, now, p.pos, strength);
        if (dmg > 0.0f) stats.RecordDamage(p.ownerUserId, b->ball.userId, dmg);

        if (!b->ball.alive) {
          stats.RecordDeath(b->ball.userId);
          stats.RecordKill(p.ownerUserId, b->ball.userId, now);
          const std::string killerName = stats.All().count(p.ownerUserId) ? stats.All().at(p.ownerUserId).username : p.ownerUserId;
          const std::string victimName = !b->ball.username.empty() ? b->ball.username : b->ball.userId;
          killFeed.Add(now, killerName + " blasted " + victimName);
        }

        // blood-pop particles
        for (int i = 0; i < 18; ++i) {
          Particle blood;
          blood.position = p.pos;
          float ang = ((float)GetRandomValue(0, 10000) / 10000.0f) * 2.0f * PI;
          float sp = (float)GetRandomValue(120, 420);
          blood.velocity = {cosf(ang)*sp, sinf(ang)*sp};
          blood.life = 0.0f;
          blood.maxLife = 0.55f;
          blood.size = (float)GetRandomValue(2, 5);
          // reuse ball particle system: inject into victim particles by calling collision particles
        }

        p.active = false;
        break;
      }
    }
  }
}

void Application::DrawProjectiles() const
{
  const float t = (float)GetTime();
  for (const auto& p : projectiles) {
    if (!p.active) continue;

    Vector2 dir = p.vel;
    float vlen = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (vlen > 0.001f) {
      dir.x /= vlen;
      dir.y /= vlen;
    } else {
      dir = {0.0f, 1.0f};
    }

    if (p.kind == ProjectileKind::Meteor) {
      for (int i = 0; i < 8; ++i) {
        float f = (float)i / 8.0f;
        Vector2 tail = {p.pos.x - dir.x * p.radius * (2.2f + f * 4.5f),
                        p.pos.y - dir.y * p.radius * (2.2f + f * 4.5f)};
        float r = p.radius * (0.95f - f * 0.55f);
        Color c = {255, (unsigned char)(90 + (int)(f * 80.0f)), 40,
                   (unsigned char)(220 - i * 22)};
        DrawCircleV(tail, r, c);
      }
      DrawCircleV(p.pos, p.radius * 0.85f, {60, 20, 35, 255});
      DrawCircleV(p.pos, p.radius * 0.45f, {255, 240, 200, 255});
      float flick = 0.6f + 0.4f * sinf(t * 40.0f + p.pos.x * 0.05f);
      DrawCircleLines((int)p.pos.x, (int)p.pos.y, p.radius + 3.0f * flick,
                      Fade({255, 200, 120, 255}, 0.75f));
    } else if (p.kind == ProjectileKind::BlasterBolt) {
      Vector2 perp = {-dir.y, dir.x};
      float len = p.radius * 2.8f;
      Vector2 a = {p.pos.x + perp.x * p.radius * 0.9f - dir.x * len * 0.5f,
                   p.pos.y + perp.y * p.radius * 0.9f - dir.y * len * 0.5f};
      Vector2 b = {p.pos.x - perp.x * p.radius * 0.9f - dir.x * len * 0.5f,
                   p.pos.y - perp.y * p.radius * 0.9f - dir.y * len * 0.5f};
      Vector2 tip = {p.pos.x + dir.x * len * 1.1f, p.pos.y + dir.y * len * 1.1f};
      DrawTriangle(a, b, tip, Fade(p.color, 0.35f));
      DrawLineEx(a, tip, 3.0f, Fade({255, 255, 255, 255}, 0.9f));
      DrawLineEx(b, tip, 3.0f, Fade({255, 255, 255, 255}, 0.9f));
      DrawCircleV(p.pos, p.radius * 0.5f, {255, 255, 255, 230});
      DrawCircleV(tip, p.radius * 0.35f, Fade(p.color, 0.85f));
    } else {
      DrawCircleV(p.pos, p.radius, p.color);
      DrawCircleLines((int)p.pos.x, (int)p.pos.y, p.radius + 2.0f, {255, 255, 255, 60});
    }
  }
}

static constexpr float kFrostNovaRadius    = 170.0f;
static constexpr float kGravityWellRadius  = 260.0f;
static constexpr float kBlackHoleRadius    = 340.0f;
static constexpr float kBlackHoleCoreRadius = 90.0f;
static constexpr float kInfernoRadius      = 150.0f;
static constexpr float kVortexRadius       = 140.0f;
static constexpr float kLightningRange     = 520.0f;
static constexpr int   kLightningTargets   = 3;
static constexpr float kShockRingThickness = 36.0f;
static constexpr float kInfernoTickInterval = 0.25f;
static constexpr float kVortexTickInterval  = 0.20f;
static constexpr float kBlackHoleTickInterval = 0.30f;

void Application::ApplyAbilityAuras()
{
  const double now = GetTime();
  const float dt = EngineConfig::dt;

  for (auto& a : playerballs) {
    if (!a) continue;
    if (!a->ball.alive) continue;
    auto& A = a->ball;
    const AbilityType act = A.ability.active;
    if (act == AbilityType::None && !A.ability.justActivated) continue;

    if (act == AbilityType::FrostNova) {
      const float r2 = kFrostNovaRadius * kFrostNovaRadius;
      for (auto& b : playerballs) {
        if (!b || b.get() == a.get()) continue;
        if (!b->ball.alive) continue;
        if (b->ball.invulnTimer > 0.0f) continue;
        const float dx = b->ball.position.x - A.position.x;
        const float dy = b->ball.position.y - A.position.y;
        const float d2 = dx * dx + dy * dy;
        if (d2 > r2) continue;
        const float falloff = 1.0f - (sqrtf(d2) / kFrostNovaRadius);
        const float slow = 1.0f - (0.55f * falloff) * dt * 6.0f;
        const float clamp = slow < 0.85f ? 0.85f : slow;
        b->ball.velocity.x *= clamp;
        b->ball.velocity.y *= clamp;
      }
    }

    if (act == AbilityType::GravityWell) {
      const float r2 = kGravityWellRadius * kGravityWellRadius;
      const float pullStrength = 720.0f;
      for (auto& b : playerballs) {
        if (!b || b.get() == a.get()) continue;
        if (!b->ball.alive) continue;
        const float dx = A.position.x - b->ball.position.x;
        const float dy = A.position.y - b->ball.position.y;
        const float d2 = dx * dx + dy * dy;
        if (d2 > r2 || d2 < 1.0f) continue;
        const float d = sqrtf(d2);
        const float falloff = 1.0f - (d / kGravityWellRadius);
        const float pull = pullStrength * (0.4f + falloff);
        b->ball.velocity.x += (dx / d) * pull * dt;
        b->ball.velocity.y += (dy / d) * pull * dt;
      }
    }

    const bool waveActive = (act == AbilityType::Shockwave || act == AbilityType::NukeStrike);
    if (waveActive && A.ability.shockRadius > 0.0f) {
      const bool isNuke = (act == AbilityType::NukeStrike);
      const float ringThick = isNuke ? 70.0f : kShockRingThickness;
      const float r0 = A.ability.prevShockRadius;
      const float r1 = A.ability.shockRadius;
      const float impulse = isNuke ? 1200.0f : 480.0f;
      const float strengthScale = isNuke ? 4.5f : 1.4f;
      const char* verb = isNuke ? " nuked " : " shockwaved ";

      for (auto& b : playerballs) {
        if (!b || b.get() == a.get()) continue;
        if (!b->ball.alive) continue;
        if (b->ball.userId == A.userId) continue;
        const float dx = b->ball.position.x - A.ability.shockCenter.x;
        const float dy = b->ball.position.y - A.ability.shockCenter.y;
        const float d = sqrtf(dx * dx + dy * dy);
        if (d <= r0 - ringThick * 0.5f) continue;
        if (d > r1 + ringThick * 0.5f) continue;

        Vector2 normal = (d > 0.001f) ? Vector2{dx / d, dy / d} : Vector2{1.0f, 0.0f};
        b->ball.velocity.x += normal.x * impulse;
        b->ball.velocity.y += normal.y * impulse;

        const Vector2 hitPoint = {b->ball.position.x - normal.x * b->ball.radius,
                                  b->ball.position.y - normal.y * b->ball.radius};
        const float strength = strengthScale * A.ability.power;
        const float dmg = b->OnCollision(A.userId, now, hitPoint, strength);
        if (dmg > 0.0f) stats.RecordDamage(A.userId, b->ball.userId, dmg);
        if (!b->ball.alive) {
          stats.RecordDeath(b->ball.userId);
          stats.RecordKill(A.userId, b->ball.userId, now);
          const auto& all = stats.All();
          const auto kIt = all.find(A.userId);
          const std::string killerName = (kIt != all.end() && !kIt->second.username.empty())
                                             ? kIt->second.username
                                             : A.userId;
          const std::string victimName = !b->ball.username.empty() ? b->ball.username : b->ball.userId;
          killFeed.Add(now, killerName + std::string(verb) + victimName);
        }
      }
    }

    if (act == AbilityType::Inferno) {
      A.ability.tickTimer -= dt;
      const bool burnTick = (A.ability.tickTimer <= 0.0f);
      if (burnTick) A.ability.tickTimer = kInfernoTickInterval;

      const float r2 = kInfernoRadius * kInfernoRadius;
      for (auto& b : playerballs) {
        if (!b || b.get() == a.get()) continue;
        if (!b->ball.alive) continue;
        const float dx = b->ball.position.x - A.position.x;
        const float dy = b->ball.position.y - A.position.y;
        const float d2 = dx * dx + dy * dy;
        if (d2 > r2) continue;

        if (burnTick) {
          const float d = sqrtf(d2);
          Vector2 n = (d > 0.001f) ? Vector2{dx / d, dy / d} : Vector2{1.0f, 0.0f};
          b->ball.velocity.x += n.x * 60.0f;
          b->ball.velocity.y += n.y * 60.0f;
          const Vector2 hitPoint = {b->ball.position.x, b->ball.position.y};
          const float strength = 0.55f * A.ability.power;
          const float dmg = b->OnCollision(A.userId, now, hitPoint, strength);
          if (dmg > 0.0f) stats.RecordDamage(A.userId, b->ball.userId, dmg);
          if (!b->ball.alive) {
            stats.RecordDeath(b->ball.userId);
            stats.RecordKill(A.userId, b->ball.userId, now);
            const auto& all = stats.All();
            const auto kIt = all.find(A.userId);
            const std::string killerName = (kIt != all.end() && !kIt->second.username.empty())
                                               ? kIt->second.username
                                               : A.userId;
            const std::string victimName = !b->ball.username.empty() ? b->ball.username : b->ball.userId;
            killFeed.Add(now, killerName + " incinerated " + victimName);
          }
        }
      }
    }

    if (act == AbilityType::BlackHole) {
      A.ability.tickTimer -= dt;
      const bool tick = (A.ability.tickTimer <= 0.0f);
      if (tick) A.ability.tickTimer = kBlackHoleTickInterval;

      const float r2 = kBlackHoleRadius * kBlackHoleRadius;
      const float core2 = kBlackHoleCoreRadius * kBlackHoleCoreRadius;
      const float pullStrength = 1800.0f;
      for (auto& b : playerballs) {
        if (!b || b.get() == a.get()) continue;
        if (!b->ball.alive) continue;
        const float dx = A.position.x - b->ball.position.x;
        const float dy = A.position.y - b->ball.position.y;
        const float d2 = dx * dx + dy * dy;
        if (d2 > r2 || d2 < 1.0f) continue;
        const float d = sqrtf(d2);
        const float falloff = 1.0f - (d / kBlackHoleRadius);
        const float pull = pullStrength * (0.5f + falloff * falloff);
        b->ball.velocity.x += (dx / d) * pull * dt;
        b->ball.velocity.y += (dy / d) * pull * dt;
        b->ball.velocity.x *= 0.92f;
        b->ball.velocity.y *= 0.92f;

        if (tick && d2 < core2) {
          const Vector2 hitPoint = {b->ball.position.x, b->ball.position.y};
          const float strength = 1.8f * A.ability.power;
          const float dmg = b->OnCollision(A.userId, now, hitPoint, strength);
          if (dmg > 0.0f) stats.RecordDamage(A.userId, b->ball.userId, dmg);
          if (!b->ball.alive) {
            stats.RecordDeath(b->ball.userId);
            stats.RecordKill(A.userId, b->ball.userId, now);
            const auto& all = stats.All();
            const auto kIt = all.find(A.userId);
            const std::string killerName = (kIt != all.end() && !kIt->second.username.empty())
                                               ? kIt->second.username
                                               : A.userId;
            const std::string victimName = !b->ball.username.empty() ? b->ball.username : b->ball.userId;
            killFeed.Add(now, killerName + " devoured " + victimName);
          }
        }
      }
    }

    if (act == AbilityType::TimeWarp) {
      const float slow = std::max(0.55f, 1.0f - 4.0f * dt);
      for (auto& b : playerballs) {
        if (!b || b.get() == a.get()) continue;
        if (!b->ball.alive) continue;
        if (b->ball.invulnTimer > 0.0f) continue;
        b->ball.velocity.x *= slow;
        b->ball.velocity.y *= slow;
      }
    }

    if (act == AbilityType::Vortex) {
      A.ability.tickTimer -= dt;
      const bool tick = (A.ability.tickTimer <= 0.0f);
      if (tick) A.ability.tickTimer = kVortexTickInterval;

      const float r2 = kVortexRadius * kVortexRadius;
      for (auto& b : playerballs) {
        if (!b || b.get() == a.get()) continue;
        if (!b->ball.alive) continue;
        const float dx = b->ball.position.x - A.position.x;
        const float dy = b->ball.position.y - A.position.y;
        const float d2 = dx * dx + dy * dy;
        if (d2 > r2 || d2 < 1.0f) continue;
        const float d = sqrtf(d2);
        Vector2 n = {dx / d, dy / d};
        Vector2 tangent = {-n.y, n.x};

        b->ball.velocity.x += (n.x * 220.0f + tangent.x * 520.0f) * dt;
        b->ball.velocity.y += (n.y * 220.0f + tangent.y * 520.0f) * dt;

        if (tick) {
          const Vector2 hitPoint = {b->ball.position.x, b->ball.position.y};
          const float strength = 0.9f * A.ability.power;
          const float dmg = b->OnCollision(A.userId, now, hitPoint, strength);
          if (dmg > 0.0f) stats.RecordDamage(A.userId, b->ball.userId, dmg);
          if (!b->ball.alive) {
            stats.RecordDeath(b->ball.userId);
            stats.RecordKill(A.userId, b->ball.userId, now);
            const auto& all = stats.All();
            const auto kIt = all.find(A.userId);
            const std::string killerName = (kIt != all.end() && !kIt->second.username.empty())
                                               ? kIt->second.username
                                               : A.userId;
            const std::string victimName = !b->ball.username.empty() ? b->ball.username : b->ball.userId;
            killFeed.Add(now, killerName + " shredded " + victimName);
          }
        }
      }
    }

    if (A.ability.justActivated && act == AbilityType::LightningBurst) {
      struct TargetRef { BouncingBall* ball; float d2; };
      std::vector<TargetRef> targets;
      targets.reserve(playerballs.size());
      for (auto& b : playerballs) {
        if (!b || b.get() == a.get()) continue;
        if (!b->ball.alive) continue;
        const float dx = b->ball.position.x - A.position.x;
        const float dy = b->ball.position.y - A.position.y;
        const float d2 = dx * dx + dy * dy;
        if (d2 > kLightningRange * kLightningRange) continue;
        targets.push_back({b.get(), d2});
      }
      std::sort(targets.begin(), targets.end(),
                [](const TargetRef& l, const TargetRef& r) { return l.d2 < r.d2; });

      Vector2 chainFrom = A.position;
      const int hits = (int)std::min((size_t)kLightningTargets, targets.size());
      for (int i = 0; i < hits; ++i) {
        BouncingBall* t = targets[i].ball;
        const Vector2 hitPoint = t->ball.position;

        Vector2 dir = {hitPoint.x - chainFrom.x, hitPoint.y - chainFrom.y};
        const float dlen = sqrtf(dir.x * dir.x + dir.y * dir.y);
        if (dlen > 0.001f) { dir.x /= dlen; dir.y /= dlen; }
        const float impulse = 380.0f;
        t->ball.velocity.x += dir.x * impulse;
        t->ball.velocity.y += dir.y * impulse;

        const float strength = 1.6f * A.ability.power - (float)i * 0.25f;
        const float dmg = t->OnCollision(A.userId, now, hitPoint, std::max(0.6f, strength));
        if (dmg > 0.0f) stats.RecordDamage(A.userId, t->ball.userId, dmg);
        if (!t->ball.alive) {
          stats.RecordDeath(t->ball.userId);
          stats.RecordKill(A.userId, t->ball.userId, now);
          const auto& all = stats.All();
          const auto kIt = all.find(A.userId);
          const std::string killerName = (kIt != all.end() && !kIt->second.username.empty())
                                             ? kIt->second.username
                                             : A.userId;
          const std::string victimName = !t->ball.username.empty() ? t->ball.username : t->ball.userId;
          killFeed.Add(now, killerName + " electrocuted " + victimName);
        }

        LightningFx fx;
        fx.a = chainFrom;
        fx.b = hitPoint;
        fx.life = 0.0f;
        fx.maxLife = 0.45f;
        fx.color = {255, 240, 120, 255};
        lightningFx.push_back(fx);
        chainFrom = hitPoint;
      }
    }

    A.ability.justActivated = false;
  }
}

void Application::UpdateLightningFx()
{
  for (auto& fx : lightningFx) {
    fx.life += EngineConfig::dt;
  }
  lightningFx.erase(std::remove_if(lightningFx.begin(), lightningFx.end(),
                                   [](const LightningFx& f) { return f.life >= f.maxLife; }),
                    lightningFx.end());
}

void Application::DrawLightningFx() const
{
  for (const auto& fx : lightningFx) {
    const float t01 = fx.life / fx.maxLife;
    const float alpha = 1.0f - t01;

    const Vector2 d = {fx.b.x - fx.a.x, fx.b.y - fx.a.y};
    const float len = sqrtf(d.x * d.x + d.y * d.y);
    if (len < 0.001f) continue;
    const Vector2 dir = {d.x / len, d.y / len};
    const Vector2 perp = {-dir.y, dir.x};

    const int segs = 10;
    Vector2 prev = fx.a;
    for (int i = 1; i <= segs; ++i) {
      const float f = (float)i / (float)segs;
      const float jitter =
          (i == segs) ? 0.0f : ((float)GetRandomValue(-100, 100) / 100.0f) * 14.0f * (1.0f - t01);
      Vector2 next = {fx.a.x + dir.x * len * f + perp.x * jitter,
                      fx.a.y + dir.y * len * f + perp.y * jitter};
      DrawLineEx(prev, next, 6.0f, Fade({255, 255, 255, 255}, alpha * 0.55f));
      DrawLineEx(prev, next, 3.0f, Fade(fx.color, alpha));
      prev = next;
    }
    DrawCircleV(fx.a, 6.0f * alpha + 2.0f, Fade({255, 255, 200, 255}, alpha));
    DrawCircleV(fx.b, 8.0f * alpha + 3.0f, Fade(fx.color, alpha));
  }
}

void Application::DrawAuras()
{
  if (auraShader.id == 0 || whiteTex.id == 0) return;

  // If our aura shader failed to compile, raylib silently falls back to the
  // default shader (id == rlGetShaderIdDefault). In that case we'd render a
  // plain white square per active ability, which looks awful. Skip instead.
  if (auraShader.id == (unsigned int)rlGetShaderIdDefault()) return;
  if (auraColorLoc < 0 || auraTypeLoc < 0) return;

  const float t = (float)GetTime();

  BeginBlendMode(BLEND_ADDITIVE);
  BeginShaderMode(auraShader);

  for (const auto& bp : playerballs) {
    if (!bp) continue;
    const Ball& b = bp->ball;
    if (!b.alive) continue;
    if (b.ability.active == AbilityType::None) continue;

    const int kind = GetAuraVisualKind(b.ability.active);
    if (kind <= 0) continue;

    const AbilitySpec& spec = GetAbilitySpec(b.ability.active);
    const float fade = (b.ability.activeTimer < 0.5f)
                          ? (b.ability.activeTimer / 0.5f)
                          : 1.0f;
    float color[4] = {
        spec.uiColor.r / 255.0f,
        spec.uiColor.g / 255.0f,
        spec.uiColor.b / 255.0f,
        0.85f * fade,
    };
    float kindF = (float)kind;

    SetShaderValue(auraShader, auraColorLoc, color, SHADER_UNIFORM_VEC4);
    if (auraTimeLoc >= 0)
      SetShaderValue(auraShader, auraTimeLoc, &t, SHADER_UNIFORM_FLOAT);
    SetShaderValue(auraShader, auraTypeLoc, &kindF, SHADER_UNIFORM_FLOAT);

    const float size = b.radius * 6.5f;
    Rectangle src = {0, 0, 1, 1};
    Rectangle dst = {b.position.x, b.position.y, size, size};
    Vector2 origin = {size * 0.5f, size * 0.5f};
    DrawTexturePro(whiteTex, src, dst, origin, 0.0f, WHITE);
  }

  EndShaderMode();
  EndBlendMode();
}

bool Application::CollectWorldDistortFx(float* centers, float* radii,
                                        float* strengths, int* types,
                                        int maxFx, int& outCount) const
{
  outCount = 0;
  for (const auto& bp : playerballs) {
    if (!bp) continue;
    const Ball& b = bp->ball;
    if (!b.alive) continue;
    if (b.ability.active == AbilityType::None) continue;

    int t = 0;
    float r = 0.0f;
    float s = 0.0f;
    if (!GetAbilityWorldDistort(b.ability.active, t, r, s)) continue;

    // Soft-fade the effect strength as the ability winds down.
    if (b.ability.activeTimer < 0.5f) s *= b.ability.activeTimer / 0.5f;
    if (s < 0.01f) continue;

    if (outCount >= maxFx) break;

    centers[outCount * 2 + 0] = b.position.x;
    centers[outCount * 2 + 1] = b.position.y;
    radii[outCount] = r;
    strengths[outCount] = s;
    types[outCount] = t;
    ++outCount;
  }
  return outCount > 0;
}

void Application::Shutdown() {
  if (worldTargetB.id != 0) UnloadRenderTexture(worldTargetB);
  if (auraShader.id != 0) UnloadShader(auraShader);
  if (distortShader.id != 0) UnloadShader(distortShader);
  if (whiteTex.id != 0) UnloadTexture(whiteTex);
  CloseWindow();
}

// Dev-only: trigger an ability on a random alive ball via the keyboard.
// Maps each key to the gift name `AbilityFromGift` recognises so the same
// activation path the websocket uses is exercised end-to-end.
struct DebugAbilityKey {
  int           key;
  const char*   gift;
  int           diamondCount;
  const char*   label;
};

static const DebugAbilityKey kDebugAbilityKeys[] = {
    {KEY_ONE,   "Rose",         1,    "Heal Burst"},
    {KEY_TWO,   "Perfume",      20,   "Shield Bubble"},
    {KEY_THREE, "Finger Heart", 5,    "Frost Nova"},
    {KEY_FOUR,  "Lollipop",     10,   "Berserk"},
    {KEY_FIVE,  "Hearts",       199,  "Lightning Burst"},
    {KEY_SIX,   "Doughnut",     30,   "Gravity Well"},
    {KEY_SEVEN, "Necklace",     400,  "Chainsaw Armor"},
    {KEY_EIGHT, "TikTok",       1,    "Shockwave"},
    {KEY_NINE,  "Heels",        700,  "Blaster"},
    {KEY_ZERO,  "Pool Party",   4999, "Meteor Storm"},
    {KEY_Q,     "LOVE Balloon", 699,  "Inferno"},
    {KEY_W,     "Carousel",     2020, "Black Hole"},
    {KEY_E,     "Planet",       15000,"Time Warp"},
    {KEY_R,     "Speedboat",    1888, "Rocket Boost"},
    {KEY_T,     "Lion",         29999,"Phoenix Revive"},
    {KEY_Y,     "Motorcycle",   2988, "Vortex"},
    {KEY_U,     "Rocket",       20000,"Nuke Strike"},
};

void Application::Update() {

  wsClient->Update(playerballs);

  // Keep stats usernames in sync with latest websocket data.
  for (const auto& b : playerballs)
  {
    if (!b) continue;
    if (!b->ball.userId.empty())
    {
      stats.EnsurePlayer(b->ball.userId, b->ball.username);
    }
  }

  // ---------------- Dev hotkeys for ability testing ----------------
  // Press 1..0 / Q W E R T Y U to grant a random alive ball that ability.
  for (const auto& k : kDebugAbilityKeys) {
    if (!IsKeyPressed(k.key)) continue;

    std::vector<BouncingBall*> alive;
    alive.reserve(playerballs.size());
    for (const auto& b : playerballs) {
      if (!b) continue;
      if (b->ball.alive) alive.push_back(b.get());
    }

    BouncingBall* target = nullptr;
    if (!alive.empty()) {
      target = alive[GetRandomValue(0, (int)alive.size() - 1)];
    } else {
      // No live players -> spawn a debug dummy so the FX is still visible.
      const int w = (EngineConfig::WindowWidth > 0) ? EngineConfig::WindowWidth : 1;
      const int h = (EngineConfig::WindowHeight > 0) ? EngineConfig::WindowHeight : 1;
      auto fresh = std::make_unique<BouncingBall>();
      fresh->Init({(float)GetRandomValue(80, w - 80),
                   (float)GetRandomValue(80, h - 80)});
      fresh->ball.username = "TEST_BOT";
      fresh->ball.userId = std::string("dbg_") + std::to_string(GetRandomValue(1, 999999));
      stats.EnsurePlayer(fresh->ball.userId, fresh->ball.username);
      playerballs.push_back(std::move(fresh));
      target = playerballs.back().get();
    }

    target->ApplyGift(k.gift, 1, k.diamondCount);
    killFeed.Add(GetTime(),
                 std::string("[DEBUG] ") + target->ball.username + " -> " + k.label);
  }

  for (auto &ball : playerballs) {
    ball->Update();
  }

  ApplyAbilityAuras();
  UpdateLightningFx();

  // Spawn blaster shots + meteor storm shots based on active abilities.
  // (Gift-only abilities are applied in BouncingBall::ApplyGift.)
  for (const auto& b : playerballs) {
    if (!b) continue;
    if (!b->ball.alive) continue;

    if (b->ball.ability.active == AbilityType::Blaster) {
      // Fire at nearest target occasionally.
      static float fireAccum = 0.0f;
      fireAccum += EngineConfig::dt;
      if (fireAccum >= 0.25f) {
        fireAccum = 0.0f;

        const BouncingBall* nearest = nullptr;
        float best = 1e30f;
        for (const auto& other : playerballs) {
          if (!other) continue;
          if (!other->ball.alive) continue;
          if (other->ball.userId == b->ball.userId) continue;
          float dx = other->ball.position.x - b->ball.position.x;
          float dy = other->ball.position.y - b->ball.position.y;
          float d2 = dx*dx + dy*dy;
          if (d2 < best) { best = d2; nearest = other.get(); }
        }

        if (nearest) {
          Vector2 dir = {nearest->ball.position.x - b->ball.position.x,
                         nearest->ball.position.y - b->ball.position.y};
          float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
          if (len > 0.001f) { dir.x/=len; dir.y/=len; }
          Projectile p;
          p.pos = b->ball.position;
          p.vel = {dir.x * 650.0f, dir.y * 650.0f};
          p.radius = 6.0f;
          p.life = 1.2f;
          p.damage = 22.0f;
          p.color = {255, 220, 120, 255};
          p.kind = ProjectileKind::BlasterBolt;
          p.ownerUserId = b->ball.userId;
          projectiles.push_back(std::move(p));
        }
      }
    }

    if (b->ball.ability.active == AbilityType::MeteorStorm) {
      meteorSpawnTimer -= EngineConfig::dt;
      if (meteorSpawnTimer <= 0.0f) {
        meteorSpawnTimer = 0.12f;
        Projectile m;
        m.pos = {(float)GetRandomValue(0, GetScreenWidth()), -20.0f};
        m.vel = {(float)GetRandomValue(-60, 60), (float)GetRandomValue(720, 980)};
        m.radius = (float)GetRandomValue(7, 11);
        m.life = 1.4f;
        m.damage = 28.0f;
        m.color = {255, 120, 190, 255};
        m.kind = ProjectileKind::Meteor;
        m.ownerUserId = b->ball.userId;
        projectiles.push_back(std::move(m));
      }
    }
  }

  UpdateProjectiles();

  grid.clear();

  for (int i = 0; i < playerballs.size(); i++) {
    auto &b = playerballs[i]->ball;

    int cellX = (int)(b.position.x / CELL_SIZE);
    int cellY = (int)(b.position.y / CELL_SIZE);

    int key = cellX + cellY * 10000;
    grid[key].push_back(i);
  }

  // Collisions
  for (auto &[key, indices] : grid) {
    for (int i = 0; i < indices.size(); i++) {
      for (int j = i + 1; j < indices.size(); j++) {

        auto &A = playerballs[indices[i]]->ball;
        auto &B = playerballs[indices[j]]->ball;

        if (!A.alive || !B.alive)
          continue;

        float dx = B.position.x - A.position.x;
        float dy = B.position.y - A.position.y;

        float distSq = dx * dx + dy * dy;
        float r = A.radius + B.radius;

        if (distSq < r * r) {

          float dist = sqrt(distSq);
          if (dist == 0)
            continue;

          Vector2 normal = {dx / dist, dy / dist};

          float overlap = r - dist + 0.5f;

          // separation
          A.position.x -= normal.x * overlap * 0.5f;
          A.position.y -= normal.y * overlap * 0.5f;

          B.position.x += normal.x * overlap * 0.5f;
          B.position.y += normal.y * overlap * 0.5f;

          // push (ANTI-CLUMP FIX)
          float push = 60.0f * 0.5f * (A.knockbackMultiplier + B.knockbackMultiplier);

          A.velocity.x -= normal.x * push;
          A.velocity.y -= normal.y * push;

          B.velocity.x += normal.x * push;
          B.velocity.y += normal.y * push;

          Vector2 point = {(A.position.x + B.position.x) * 0.5f,
                           (A.position.y + B.position.y) * 0.5f};

          float strength = fabs(A.velocity.x - B.velocity.x) / 300.0f;
          const double now = GetTime();

          // Chainsaw armor turns the wearer into a melee threat: the *other*
          // ball takes more damage on contact.
          float strFromA = strength * (A.ability.active == AbilityType::ChainsawArmor ? 1.7f : 1.0f);
          float strFromB = strength * (B.ability.active == AbilityType::ChainsawArmor ? 1.7f : 1.0f);

          // Apply damage to both, attributing each hit to the other userId.
          float dmgToA = playerballs[indices[i]]->OnCollision(B.userId, now, point, strFromB);
          float dmgToB = playerballs[indices[j]]->OnCollision(A.userId, now, point, strFromA);

          if (dmgToA > 0.0f) stats.RecordDamage(B.userId, A.userId, dmgToA);
          if (dmgToB > 0.0f) stats.RecordDamage(A.userId, B.userId, dmgToB);

          // Ability physics: shockwave adds an extra impulse on hit.
          if (playerballs[indices[i]]->ball.ability.active == AbilityType::Shockwave) {
            float extra = 140.0f;
            B.velocity.x += normal.x * extra;
            B.velocity.y += normal.y * extra;
          }
          if (playerballs[indices[j]]->ball.ability.active == AbilityType::Shockwave) {
            float extra = 140.0f;
            A.velocity.x -= normal.x * extra;
            A.velocity.y -= normal.y * extra;
          }

          // Death attribution window (seconds).
          const double assistWindow = 6.0;

          if (!A.alive)
          {
            stats.RecordDeath(A.userId);
            if (!A.lastHitByUserId.empty() && (now - A.lastHitAtSeconds) <= assistWindow)
            {
              stats.RecordKill(A.lastHitByUserId, A.userId, now);
              const auto& all = stats.All();
              const auto kIt = all.find(A.lastHitByUserId);
              const std::string killerName = (kIt != all.end() && !kIt->second.username.empty()) ? kIt->second.username : A.lastHitByUserId;
              const std::string victimName = !A.username.empty() ? A.username : A.userId;
              killFeed.Add(now, killerName + " eliminated " + victimName);
            }
          }

          if (!B.alive)
          {
            stats.RecordDeath(B.userId);
            if (!B.lastHitByUserId.empty() && (now - B.lastHitAtSeconds) <= assistWindow)
            {
              stats.RecordKill(B.lastHitByUserId, B.userId, now);
              const auto& all = stats.All();
              const auto kIt = all.find(B.lastHitByUserId);
              const std::string killerName = (kIt != all.end() && !kIt->second.username.empty()) ? kIt->second.username : B.lastHitByUserId;
              const std::string victimName = !B.username.empty() ? B.username : B.userId;
              killFeed.Add(now, killerName + " eliminated " + victimName);
            }
          }
        }
      }
    }
  }
}

void Application::Render() {
  EngineConfig::dt = GetFrameTime();
  EngineConfig::UpdateWindowSize();
  EnsureRenderTargets();

  // ---- 1) Scene + GPU auras into worldTarget --------------------------------
  BeginTextureMode(worldTarget);
  ClearBackground({12, 12, 18, 255});

  for (auto &ball : playerballs) {
    ball->Draw();
  }
  DrawProjectiles();
  DrawLightningFx();
  DrawAuras();
  EndTextureMode();

  // ---- 2) Optional world-space distortion pass into worldTargetB ------------
  constexpr int kMaxFx = 6;
  float fxCenters[kMaxFx * 2] = {0};
  float fxRadii[kMaxFx]       = {0};
  float fxStrength[kMaxFx]    = {0};
  int   fxTypes[kMaxFx]       = {0};
  int   fxCount = 0;
  const bool needDistort =
      CollectWorldDistortFx(fxCenters, fxRadii, fxStrength, fxTypes, kMaxFx, fxCount);

  RenderTexture2D* finalRT = &worldTarget;

  if (needDistort && distortShader.id != 0) {
    if (distCountLoc >= 0)
      SetShaderValue(distortShader, distCountLoc, &fxCount, SHADER_UNIFORM_INT);
    if (distCenterLoc >= 0)
      SetShaderValueV(distortShader, distCenterLoc, fxCenters, SHADER_UNIFORM_VEC2, fxCount);
    if (distRadiusLoc >= 0)
      SetShaderValueV(distortShader, distRadiusLoc, fxRadii, SHADER_UNIFORM_FLOAT, fxCount);
    if (distStrengthLoc >= 0)
      SetShaderValueV(distortShader, distStrengthLoc, fxStrength, SHADER_UNIFORM_FLOAT, fxCount);
    if (distTypeLoc >= 0)
      SetShaderValueV(distortShader, distTypeLoc, fxTypes, SHADER_UNIFORM_INT, fxCount);
    if (distTimeLoc >= 0) {
      float t = (float)GetTime();
      SetShaderValue(distortShader, distTimeLoc, &t, SHADER_UNIFORM_FLOAT);
    }

    BeginTextureMode(worldTargetB);
    ClearBackground(BLACK);
    BeginShaderMode(distortShader);
    DrawTexturePro(worldTarget.texture,
                   {0, 0, (float)worldTarget.texture.width, -(float)worldTarget.texture.height},
                   {0, 0, (float)targetW, (float)targetH},
                   {0, 0}, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();

    finalRT = &worldTargetB;
  }

  // ---- 3) Postprocess to screen -------------------------------------------
  BeginDrawing();
  ClearBackground(BLACK);

  BeginShaderMode(postShader);
  DrawTexturePro(finalRT->texture,
                 {0, 0, (float)finalRT->texture.width, -(float)finalRT->texture.height},
                 {0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                 {0, 0}, 0.0f, WHITE);
  EndShaderMode();

  DrawHUD();

  EndDrawing();
}

void Application::Run() {
  Init();

  while (!WindowShouldClose()) {
    Update();
    Render();
  }

  wsClient->Shutdown();
  Shutdown();
}

Application::~Application() {}
