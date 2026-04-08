#pragma once

#include "raylib.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

struct Ball {
  Vector2 position;
  Vector2 velocity;
  float radius;
  Color color;
  std::string username;
};

class BouncingBall {
public:
  void Init();
  void Update();
  void Draw();

  Ball ball;
  std::unique_ptr<Font> font;
private:
};
