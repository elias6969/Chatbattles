#pragma once

#include <queue>
#include <mutex>
#include <string>

#include <ixwebsocket/IXWebSocket.h>

class BouncingBall;

class WebSocketClient {
public:
    void Init(const std::string& url);
    void Update(std::vector<std::unique_ptr<BouncingBall>> &balls);
    void Shutdown();

private:
    ix::WebSocket ws;

    std::queue<std::string> messageQueue;
    std::mutex queueMutex;
};
