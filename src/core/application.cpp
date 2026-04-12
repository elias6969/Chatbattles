#include "core/Application.h"
#include "gamecore/BouncingBall.h"
#include "raylib.h"
#include "tools/EngineConfig.h"
#include <cmath>
#include <iostream>
#include <memory>

#pragma region imgui
#include "imgui.h"
#include "imguiThemes.h"
#include "rlImGui.h"
#pragma endregion

Application::Application() {}

void Application::Init() {

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(EngineConfig::WindowWidth, EngineConfig::WindowHeight,
             "ChatBattle");

  SetupImGui();

  wsClient = std::make_unique<WebSocketClient>();
  wsClient->Init("ws://127.0.0.1:8080");
  playerballs.clear();
}

void Application::SetupImGui() {
#pragma region imgui
  rlImGuiSetup(true);

  imguiThemes::red();

  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.FontGlobalScale = 1.0f;

  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.Colors[ImGuiCol_WindowBg].w = 0.5f;
  }

#pragma endregion
}

void Application::guirenderinit() {
#pragma region imgui
  rlImGuiBegin();

  ImGui::PushStyleColor(ImGuiCol_WindowBg, {});
  ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, {});
  ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
  ImGui::PopStyleColor(2);
#pragma endregion
}

void Application::guirenderafter() {
#pragma region imgui
  rlImGuiEnd();

  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }
#pragma endregion
}

void Application::guishutdown() {
#pragma region imgui
  rlImGuiShutdown();
#pragma endregion
}

void Application::Shutdown() { CloseWindow(); }

void Application::Update() {

  // 🔥 FIRST: apply websocket changes
  wsClient->Update(playerballs);

  // 1️⃣ Move balls
  for (auto &ball : playerballs) {
    ball->Update();
  }

  // 2️⃣ Build spatial grid
  grid.clear();

  for (int i = 0; i < playerballs.size(); i++) {
    auto &b = playerballs[i]->ball;

    int cellX = (int)(b.position.x / CELL_SIZE);
    int cellY = (int)(b.position.y / CELL_SIZE);

    int key = cellX + cellY * 10000;
    grid[key].push_back(i);
  }

  // 3️⃣ Collisions
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

          // 🧱 separation
          A.position.x -= normal.x * overlap * 0.5f;
          A.position.y -= normal.y * overlap * 0.5f;

          B.position.x += normal.x * overlap * 0.5f;
          B.position.y += normal.y * overlap * 0.5f;

          // 💥 push (ANTI-CLUMP FIX)
          float push = 60.0f;

          A.velocity.x -= normal.x * push;
          A.velocity.y -= normal.y * push;

          B.velocity.x += normal.x * push;
          B.velocity.y += normal.y * push;

          Vector2 point = {(A.position.x + B.position.x) * 0.5f,
                           (A.position.y + B.position.y) * 0.5f};

          float strength = fabs(A.velocity.x - B.velocity.x) / 300.0f;

          playerballs[indices[i]]->OnCollision(point, strength);
          playerballs[indices[j]]->OnCollision(point, strength);
        }
      }
    }
  }
}

void Application::Render() {
  EngineConfig::dt = GetFrameTime();
  // wsClient->Update(playerballs);
  //  player->Update();

  BeginDrawing();
  ClearBackground(GRAY);
  EngineConfig::UpdateWindowSize();

  for (auto &ball : playerballs) {

    ball->Draw();
  }

  guirenderinit();

  ImGui::SetNextWindowBgAlpha(0.0f);
  ImGui::Begin("Config");

  if (!playerballs.empty()) {
    ImGui::Text("Ball vel X: %f", playerballs[0]->ball.velocity.x);
    ImGui::Text("Ball vel Y: %f", playerballs[0]->ball.velocity.y);
  }
  ImGui::End();

  guirenderafter();

  EndDrawing();
}

void Application::Run() {
  Init();

  while (!WindowShouldClose()) {
    Update();
    Render();
  }

  guishutdown();
  wsClient->Shutdown();
  Shutdown();
}

Application::~Application() {}
