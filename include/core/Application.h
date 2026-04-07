#pragma once

#include "raylib.h"
#include <memory>
#include <vector>
#include "gamecore/BouncingBall.h"



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
  void Update();
  void Render();
};
