#include "network/WebSocketClient.h"
#include "gamecore/BouncingBall.h"

#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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

void WebSocketClient::Update(BouncingBall &ball) {
  std::lock_guard<std::mutex> lock(queueMutex);

  while (!messageQueue.empty()) {
    std::string msg = messageQueue.front();
    messageQueue.pop();

    try {

      auto data = json::parse(msg);

      if (!data.contains("type") || !data["type"].is_string()) {
        continue;
      }
      std::string type = data["type"];

      if (type == "chat") {

        if (!data.contains("user") || !data["user"].is_string())
          continue;
        if (!data.contains("message") || !data["message"].is_string())
          continue;

        std::string user = data["user"];
        std::string message = data["message"];

        ball.ball.username = user;

        if (message == "!speed") {
          ball.ball.velocity.x *= 1.5f;
          ball.ball.velocity.y *= 1.5f;
        }

        if (message == "!stop") {
          ball.ball.velocity = {0, 0};
        }
      }

      if (type == "gift") {

        if (!data.contains("user") || !data["user"].is_string())
          continue;

        if (!data.contains("gift") || !data["gift"].is_string())
          continue;

        std::string user = data["user"];
        std::string gift = data["gift"];

        int amount = 1;
        if (data.contains("amount") && data["amount"].is_number_integer()) {
          amount = data["amount"];
        }

        std::cout << "GIFT RECEIVED: " << user << " sent " << gift << " x"
                  << amount << std::endl;

        ball.ball.username = user;

        if (gift == "Rose") {
          ball.ball.velocity.x *= 1.1f * amount;
          ball.ball.velocity.y *= 1.1f * amount;
          ball.ball.color = PINK;
        }

        else if (gift == "TikTok") {
          ball.ball.velocity.x += 200.0f * amount;
          ball.ball.velocity.y += 200.0f * amount;
          ball.ball.color = GREEN;
        }

        else if (gift == "Galaxy") {
          // BIG effect
          ball.ball.velocity.x *= 3.0f;
          ball.ball.velocity.y *= 3.0f;
          ball.ball.color = RED;
        }

        else {
          // fallback for unknown gifts
          ball.ball.velocity.x += 50.0f * amount;
          ball.ball.velocity.y += 50.0f * amount;
        }
      }
    } catch (const std::exception &e) {
      std::cerr << "JSON parse error: " << e.what() << std::endl;
    }
  }
}

void WebSocketClient::Shutdown() { ws.stop(); }
