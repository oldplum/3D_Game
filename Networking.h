#ifndef NETWORKING_H
#define NETWORKING_H

#include <string>
#include <vector>

struct NetworkPowerUpState {
    int type;
    float x;
    float y;
    bool active;
};

struct NetworkSnapshot {
    int gameState;
    int lives;
    int score;
    int level;
    int combo;
    int frameCounter;
    float ballSpeedIncrease;
    int levelReadyCountdown;
    bool multiballActive;
    bool ballSlowActive;
    bool droppedPowerUpThisLevel;
    float ballX;
    float ballY;
    float ballSpeedX;
    float ballSpeedY;
    float extraBallX;
    float extraBallY;
    float extraBallSpeedX;
    float extraBallSpeedY;
    float paddleX;
    float paddleY;
    float paddleWidth;
    float remotePaddleX;
    float remotePaddleY;
    float remotePaddleWidth;
    std::vector<int> brickActive;
    std::vector<NetworkPowerUpState> powerups;
};

struct NetworkEventBatch {
    bool connected = false;
    bool disconnected = false;
    bool hasSnapshot = false;
    bool hasRemoteInput = false;
    NetworkSnapshot snapshot;
    float remotePaddleX = 0.0f;
};

class NetworkSession {
public:
    enum class Role {
        NONE,
        HOST,
        CLIENT
    };

    NetworkSession();
    ~NetworkSession();

    bool StartHost(unsigned short port);
    bool Connect(const std::string& host, unsigned short port);
    void Shutdown();

    bool IsRunning() const { return host_ != nullptr; }
    Role GetRole() const { return role_; }

    bool Poll(NetworkEventBatch& batch);
    bool SendSnapshot(const NetworkSnapshot& snapshot);
    bool SendRemoteInput(float paddleX);

private:
    bool InitializeIfNeeded();
    bool CreateHostForRole(Role role, unsigned short port, const std::string& host, bool connectToPeer);
    void DestroyHost();
    bool SendJson(const std::string& payload, bool reliable);

    Role role_;
    void* host_;
    void* peer_;
    bool initialized_;
};

#endif
