#include "network/WebSocketClient.h"
#include "gamecore/BouncingBall.h"
#include "tools/EngineConfig.h"

#include <iostream>
#include <nlohmann/json.hpp>
#include <unordered_map>

using json = nlohmann::json;
static std::unordered_map<std::string, Texture2D> textureCache;

void WebSocketClient::Init(const std::string &url) {
  ws.setUrl(url);

  ws.setOnMessageCallback([this](const ix::WebSocketMessagePtr &msg) {
    if (msg->type == ix::WebSocketMessageType::Message) {
      std::lock_guard<std::mutex> lock(queueMutex);
      messageQueue.push(msg->str);
    }
  });

  ws.start();
}

static void ApplyPfp(BouncingBall &ball, const std::string &pfp) {
  if (pfp.empty())
    return;

  auto it = textureCache.find(pfp);
  if (it != textureCache.end()) {
    ball.SetTexture(it->second);
    return;
  }

  Image img = LoadImage(pfp.c_str());
  if (!img.data) {
    std::cout << "Image load failed: " << pfp << std::endl;
    return;
  }

  Texture2D tex = LoadTextureFromImage(img);
  UnloadImage(img);

  if (tex.id == 0) {
    std::cout << "Texture creation failed: " << pfp << std::endl;
    return;
  }

  textureCache[pfp] = tex;

  ball.SetTexture(textureCache[pfp]); // 🔥 IMPORTANT
}

void WebSocketClient::Update(
    std::vector<std::unique_ptr<BouncingBall>> &balls) {

  std::lock_guard<std::mutex> lock(queueMutex);

  while (!messageQueue.empty()) {
    std::string msg = messageQueue.front();
    messageQueue.pop();

    try {
      auto data = json::parse(msg);

      if (!data.contains("type") || !data["type"].is_string())
        continue;
      if (!data.contains("user") || !data["user"].is_string())
        continue;
      if (!data.contains("userId") || !data["userId"].is_string())
        continue;

      const std::string type = data["type"];
      const std::string user = data["user"];
      const std::string userId = data["userId"];

      std::string pfp;
      if (data.contains("pfp") && data["pfp"].is_string()) {
        pfp = data["pfp"];
      }

      // 🔍 find existing
      BouncingBall *target = nullptr;

      for (auto &b : balls) {
        if (b->ball.userId == userId) {
          target = b.get();
          break;
        }
      }

      // ➕ create if not found
      if (!target) {
        auto newBall = std::make_unique<BouncingBall>();

        Vector2 spawn = {(float)(rand() % EngineConfig::WindowWidth),
                         (float)(rand() % EngineConfig::WindowHeight)};

        newBall->Init(spawn);
        newBall->ball.username = user;
        newBall->ball.userId = userId;

        balls.push_back(std::move(newBall));
        target = balls.back().get();
      }

      // always update name
      target->ball.username = user;

      // apply profile pic
      ApplyPfp(*target, pfp);

      // ---------------- CHAT ----------------
      if (type == "chat") {
        if (!data.contains("message") || !data["message"].is_string())
          continue;

        const std::string message = data["message"];

        if (message == "!speed") {
          target->ball.velocity.x *= 1.5f;
          target->ball.velocity.y *= 1.5f;
        } else if (message == "!stop") {
          target->ball.velocity = {0, 0};
        }
      }

      // ---------------- GIFT ----------------
      
      else if (type == "gift") {
        if (!data.contains("gift") || !data["gift"].is_string())
          continue;

        const std::string gift = data["gift"];

        int amount = 1;
        if (data.contains("amount") && data["amount"].is_number_integer()) {
          amount = data["amount"];
        }

       target->ApplyGift(gift, amount);
      }

    } catch (const std::exception &e) {
      std::cerr << "JSON parse error: " << e.what() << std::endl;
    }
  }
}

void WebSocketClient::Shutdown() {
  ws.stop();
  ws.close();

  for (auto &pair : textureCache) {
    UnloadTexture(pair.second);
  }
  textureCache.clear();
}
