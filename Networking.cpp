#include "Networking.h"

#include <enet/enet.h>
#include <nlohmann/json.hpp>

#include <cstring>

namespace {

ENetHost* AsHost(void* host) {
    return static_cast<ENetHost*>(host);
}

ENetPeer* AsPeer(void* peer) {
    return static_cast<ENetPeer*>(peer);
}

ENetAddress MakeAddress(const std::string& host, unsigned short port) {
    ENetAddress address;
    enet_address_set_host(&address, host.c_str());
    address.port = port;
    return address;
}

} // namespace

NetworkSession::NetworkSession()
    : role_(Role::NONE), host_(nullptr), peer_(nullptr), initialized_(false) {}

NetworkSession::~NetworkSession() {
    Shutdown();
}

bool NetworkSession::InitializeIfNeeded() {
    if (initialized_) {
        return true;
    }

    if (enet_initialize() != 0) {
        return false;
    }

    initialized_ = true;
    return true;
}

bool NetworkSession::CreateHostForRole(Role role, unsigned short port, const std::string& host, bool connectToPeer) {
    if (!InitializeIfNeeded()) {
        return false;
    }

    DestroyHost();

    role_ = role;

    if (role == Role::HOST) {
        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = port;
        host_ = enet_host_create(&address, 1, 2, 0, 0);
        if (!host_) {
            role_ = Role::NONE;
            return false;
        }
        return true;
    }

    if (role == Role::CLIENT) {
        host_ = enet_host_create(nullptr, 1, 2, 0, 0);
        if (!host_) {
            role_ = Role::NONE;
            return false;
        }

        ENetAddress address = MakeAddress(host, port);
        peer_ = enet_host_connect(AsHost(host_), &address, 2, 0);
        if (!peer_) {
            DestroyHost();
            role_ = Role::NONE;
            return false;
        }

        ENetEvent event;
        if (enet_host_service(AsHost(host_), &event, 3000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
            peer_ = event.peer;
            return true;
        }

        DestroyHost();
        role_ = Role::NONE;
        return false;
    }

    (void)connectToPeer;
    return false;
}

bool NetworkSession::StartHost(unsigned short port) {
    return CreateHostForRole(Role::HOST, port, "", false);
}

bool NetworkSession::Connect(const std::string& host, unsigned short port) {
    return CreateHostForRole(Role::CLIENT, port, host, true);
}

void NetworkSession::DestroyHost() {
    if (host_) {
        enet_host_destroy(AsHost(host_));
        host_ = nullptr;
    }
    peer_ = nullptr;
}

void NetworkSession::Shutdown() {
    DestroyHost();
    role_ = Role::NONE;
    if (initialized_) {
        enet_deinitialize();
        initialized_ = false;
    }
}

bool NetworkSession::SendJson(const std::string& payload, bool reliable) {
    if (!host_ || !peer_) {
        return false;
    }

    const ENetPacketFlag flag = reliable ? ENET_PACKET_FLAG_RELIABLE : static_cast<ENetPacketFlag>(0);
    ENetPacket* packet = enet_packet_create(payload.data(), payload.size() + 1, flag);
    if (!packet) {
        return false;
    }

    if (enet_peer_send(AsPeer(peer_), 0, packet) != 0) {
        enet_packet_destroy(packet);
        return false;
    }

    enet_host_flush(AsHost(host_));
    return true;
}

bool NetworkSession::SendSnapshot(const NetworkSnapshot& snapshot) {
    nlohmann::json json;
    json["type"] = "snapshot";
    json["gameState"] = snapshot.gameState;
    json["lives"] = snapshot.lives;
    json["score"] = snapshot.score;
    json["level"] = snapshot.level;
    json["combo"] = snapshot.combo;
    json["frameCounter"] = snapshot.frameCounter;
    json["ballSpeedIncrease"] = snapshot.ballSpeedIncrease;
    json["levelReadyCountdown"] = snapshot.levelReadyCountdown;
    json["multiballActive"] = snapshot.multiballActive;
    json["ballSlowActive"] = snapshot.ballSlowActive;
    json["droppedPowerUpThisLevel"] = snapshot.droppedPowerUpThisLevel;
    json["ball"] = {snapshot.ballX, snapshot.ballY, snapshot.ballSpeedX, snapshot.ballSpeedY};
    json["extraBall"] = {snapshot.extraBallX, snapshot.extraBallY, snapshot.extraBallSpeedX, snapshot.extraBallSpeedY};
    json["paddle"] = {snapshot.paddleX, snapshot.paddleY, snapshot.paddleWidth};
    json["remotePaddle"] = {snapshot.remotePaddleX, snapshot.remotePaddleY, snapshot.remotePaddleWidth};
    json["brickActive"] = snapshot.brickActive;

    nlohmann::json powerupArray = nlohmann::json::array();
    for (const auto& powerUp : snapshot.powerups) {
        powerupArray.push_back({
            {"type", powerUp.type},
            {"x", powerUp.x},
            {"y", powerUp.y},
            {"active", powerUp.active}
        });
    }
    json["powerups"] = powerupArray;

    return SendJson(json.dump(), true);
}

bool NetworkSession::SendRemoteInput(float paddleX) {
    nlohmann::json json;
    json["type"] = "input";
    json["paddleX"] = paddleX;
    return SendJson(json.dump(), false);
}

bool NetworkSession::Poll(NetworkEventBatch& batch) {
    if (!host_) {
        return false;
    }

    bool receivedAnything = false;
    ENetEvent event;

    while (enet_host_service(AsHost(host_), &event, 0) > 0) {
        receivedAnything = true;
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                peer_ = event.peer;
                batch.connected = true;
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                if (peer_ == event.peer) {
                    peer_ = nullptr;
                }
                batch.disconnected = true;
                break;
            case ENET_EVENT_TYPE_RECEIVE: {
                std::string payload(reinterpret_cast<char*>(event.packet->data));
                nlohmann::json json = nlohmann::json::parse(payload, nullptr, false);
                if (!json.is_discarded()) {
                    const std::string messageType = json.value("type", "");
                    if (messageType == "snapshot") {
                        batch.hasSnapshot = true;
                        batch.snapshot.gameState = json.value("gameState", 0);
                        batch.snapshot.lives = json.value("lives", 3);
                        batch.snapshot.score = json.value("score", 0);
                        batch.snapshot.level = json.value("level", 1);
                        batch.snapshot.combo = json.value("combo", 0);
                        batch.snapshot.frameCounter = json.value("frameCounter", 0);
                        batch.snapshot.ballSpeedIncrease = json.value("ballSpeedIncrease", 1.0f);
                        batch.snapshot.levelReadyCountdown = json.value("levelReadyCountdown", 0);
                        batch.snapshot.multiballActive = json.value("multiballActive", false);
                        batch.snapshot.ballSlowActive = json.value("ballSlowActive", false);
                        batch.snapshot.droppedPowerUpThisLevel = json.value("droppedPowerUpThisLevel", false);

                        auto ballArray = json.value("ball", std::vector<float>{400.0f, 300.0f, 2.0f, 2.0f});
                        if (ballArray.size() >= 4) {
                            batch.snapshot.ballX = ballArray[0];
                            batch.snapshot.ballY = ballArray[1];
                            batch.snapshot.ballSpeedX = ballArray[2];
                            batch.snapshot.ballSpeedY = ballArray[3];
                        }

                        auto extraBallArray = json.value("extraBall", std::vector<float>{-1000.0f, -1000.0f, 0.0f, 0.0f});
                        if (extraBallArray.size() >= 4) {
                            batch.snapshot.extraBallX = extraBallArray[0];
                            batch.snapshot.extraBallY = extraBallArray[1];
                            batch.snapshot.extraBallSpeedX = extraBallArray[2];
                            batch.snapshot.extraBallSpeedY = extraBallArray[3];
                        }

                        auto paddleArray = json.value("paddle", std::vector<float>{300.0f, 550.0f, 150.0f});
                        if (paddleArray.size() >= 3) {
                            batch.snapshot.paddleX = paddleArray[0];
                            batch.snapshot.paddleY = paddleArray[1];
                            batch.snapshot.paddleWidth = paddleArray[2];
                        }

                        auto remotePaddleArray = json.value("remotePaddle", std::vector<float>{300.0f, 80.0f, 150.0f});
                        if (remotePaddleArray.size() >= 3) {
                            batch.snapshot.remotePaddleX = remotePaddleArray[0];
                            batch.snapshot.remotePaddleY = remotePaddleArray[1];
                            batch.snapshot.remotePaddleWidth = remotePaddleArray[2];
                        }

                        batch.snapshot.brickActive = json.value("brickActive", std::vector<int>{});
                        if (json.contains("powerups") && json["powerups"].is_array()) {
                            for (const auto& item : json["powerups"]) {
                                NetworkPowerUpState powerUpState;
                                powerUpState.type = item.value("type", 0);
                                powerUpState.x = item.value("x", 0.0f);
                                powerUpState.y = item.value("y", 0.0f);
                                powerUpState.active = item.value("active", true);
                                batch.snapshot.powerups.push_back(powerUpState);
                            }
                        }
                    } else if (messageType == "input") {
                        batch.hasRemoteInput = true;
                        batch.remotePaddleX = json.value("paddleX", 0.0f);
                    }
                }
                break;
            }
            default:
                break;
        }

        if (event.packet) {
            enet_packet_destroy(event.packet);
        }
    }

    return receivedAnything;
}
