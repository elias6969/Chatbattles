#pragma once

#include "raylib.h"
#include <memory>
#include <vector>
#include "gamecore/BouncingBall.h"
#include "network/WebSocketClient.h"



class Application {
public:
  Application();
  ~Application();

  void Init();
  void SetupImGui();
  void Run();
  void Shutdown();
  void guirenderinit();
  void guirenderafter();
  void guishutdown();

private:
  std::unique_ptr<BouncingBall> player;
  std::unique_ptr<WebSocketClient> wsClient;
  void Update();
  void Render();
};
