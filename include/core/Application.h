#pragma once

#include "raylib.h"
#include <vector>

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
  void Update();
  void Render();
};
