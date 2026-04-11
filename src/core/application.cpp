#include "core/Application.h"
#include "gamecore/BouncingBall.h"
#include "raylib.h"
#include "tools/EngineConfig.h"
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

  player = std::make_unique<BouncingBall>();
  player->Init();
  wsClient = std::make_unique<WebSocketClient>();
  wsClient->Init("ws://127.0.0.1:8080");

  for (int i = 0; i < 1; i++) {
    auto ball = std::make_unique<BouncingBall>();
    ball->Init();
    playerballs.push_back(std::move(ball));
  }
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

void Application::Update() {}

void Application::Render() {
  EngineConfig::dt = GetFrameTime();
  wsClient->Update(playerballs);
  // player->Update();
  for (auto &ball : playerballs) {
    ball->Update();
  }

  BeginDrawing();
  ClearBackground(GRAY);
  EngineConfig::UpdateWindowSize();

  for (auto &ball : playerballs) {

    ball->Draw();
  }

  guirenderinit();

  ImGui::SetNextWindowBgAlpha(0.0f);
  ImGui::Begin("Config");
  ImGui::Text("Ball velocity: %f", static_cast<float>(player->ball.velocity.x));
  ImGui::Text("Ball velocity: %f", static_cast<float>(player->ball.velocity.y));
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
