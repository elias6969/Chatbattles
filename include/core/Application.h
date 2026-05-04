#pragma once

#include "raylib.h"
#include <memory>
#include <vector>
#include "gamecore/BouncingBall.h"
#include "gamecore/KillFeed.h"
#include "gamecore/Stats.h"
#include "gamecore/Projectiles.h"
#include "network/WebSocketClient.h"


struct LightningFx {
  Vector2 a{};
  Vector2 b{};
  float life = 0.0f;
  float maxLife = 0.45f;
  Color color{255, 240, 120, 255};
};

class Application {
public:
  Application();
  ~Application();

  void Init();
  void Run();
  void Shutdown();

private:
  const int CELL_SIZE = 100;
  std::unordered_map<int, std::vector<int>> grid;
  std::vector<std::unique_ptr<BouncingBall>> playerballs;
  std::unique_ptr<BouncingBall> player;
  std::unique_ptr<WebSocketClient> wsClient;
  StatsManager stats;
  KillFeed killFeed;
  RenderTexture2D worldTarget{};
  RenderTexture2D worldTargetB{};
  Shader postShader{};
  Shader avatarShader{};
  Shader auraShader{};
  Shader distortShader{};
  Texture2D whiteTex{};

  // Cached aura uniform locations.
  int auraColorLoc = -1;
  int auraTimeLoc = -1;
  int auraTypeLoc = -1;

  // Cached distort uniform locations.
  int distResLoc = -1;
  int distTimeLoc = -1;
  int distCountLoc = -1;
  int distCenterLoc = -1;
  int distRadiusLoc = -1;
  int distStrengthLoc = -1;
  int distTypeLoc = -1;

  std::vector<Projectile> projectiles;
  std::vector<LightningFx> lightningFx;
  float meteorSpawnTimer = 0.0f;
  int targetW = 0;
  int targetH = 0;
  void EnsureRenderTargets();
  void DrawHUD();
  void UpdateProjectiles();
  void DrawProjectiles() const;
  void DrawAuras();
  void ApplyAbilityAuras();
  void UpdateLightningFx();
  void DrawLightningFx() const;
  bool CollectWorldDistortFx(float* centers, float* radii, float* strengths,
                             int* types, int maxFx, int& outCount) const;
  void Update();
  void Render();
};
