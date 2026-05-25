# Breakout 3D

## 简介

`Breakout 3D` 是一个使用 `raylib` 开发的打砖块风格游戏，代码库包含单机玩法、局域网/网络对战（基于 ENet）、道具与多关卡存档。该项目实现了若干性能优化（对象池、粒子复用、按需异步加载）。

## 本仓库概览

- 可执行目标：构建后默认二进制名为 `game`（在构建目录中）。见 [CMakeLists.txt](CMakeLists.txt#L1)。
- 主要源码：`main.cpp`, `Game.cpp` / `Game.h`, `Networking.cpp` / `Networking.h`, `Ball*.cpp` / `Paddle*.cpp`, `PowerUp*.cpp`。
- 配置与数据：`config.json`, `levels.json`, `savegame.json`, `leaderboard.txt`。
- 测试：`tests/collision_test.cpp`（CMake target `collision_test`）。

## 主要依赖

- CMake >= 3.10
- C++17 编译器（例如 `g++`）
- raylib（渲染 / 输入）
- enet（网络，FetchContent 在 CMake 中拉取）
- nlohmann_json（JSON 序列化，FetchContent 在 CMake 中拉取）
- 系统库：OpenGL、X11、pthread、m、dl、rt 等

在首次运行 CMake 前请确保上述系统依赖已安装（例如在 Debian/Ubuntu 上安装对应 dev 包）。

## 快速开始（推荐：CMake）

在项目根目录执行：

```bash
mkdir -p build && cd build
cmake ..
cmake --build . -j
# 构建完成后在 build/ 目录中运行
./game
```

如果你只想编译仓库里的单个示例文件（例如 `rotating_cube.cpp`），可以使用 g++：

```bash
g++ -g rotating_cube.cpp -o output/rotating_cube -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./output/rotating_cube
```

工作区也包含 VS Code 构建任务（例如 `Build rotating_cube` 与 `Build game (CMake)`）。

## 网络/多人

- 默认网络端口：`12345`（主机与客户端均使用此端口）。实现位于 `NetworkSession` / `Game::StartNetworkHost()` 和 `Game::StartNetworkClient()` 中。见 [Game.cpp](Game.cpp#L430-L470)。
- 在主菜单:
	- 按 `H` 启动为主机（Host），程序会启动 ENet 主机并开始广播游戏快照（约 30Hz）。
	- 按 `C` 连接为客户端（Client），当前实现会尝试连接本地 `127.0.0.1`；要连接远程主机，请修改源码中调用 `StartNetworkClient("<HOST>")` 或运行在目标主机上并确保端口开放。
- 主机会广播 `snapshot` 消息，客户端发送 `input`（远端挡板位置）。详细协议为 JSON（使用 `nlohmann::json` 序列化）。实现位于 [Networking.cpp](Networking.cpp#L1)。

注意：在跨机器联机时请确保防火墙/路由允许 TCP/UDP（ENet 使用 UDP）端口 `12345`。若要自定义端口，可修改 `Game::StartNetworkHost()` / `StartNetworkClient()` 的端口参数。

## 控件（默认）

- 左/右方向键：移动挡板（主机/客户端在各自设备上移动）。
- 空格（Space）：在菜单中发射/开始游戏。
- `P`：暂停 / 恢复。
- `R`：在 GAME OVER/VICTORY 下返回菜单。
- `L`：触发异步加载效果（演示异步任务）。
- `H`：在菜单中启动主机（Host）。
- `C`：在菜单中作为客户端连接（Connect，默认连接 localhost）。
- `G`：读取 `savegame.json`（若存在）。
- `F5`：保存游戏到 `savegame.json`。
- `F9`：从 `savegame.json` 读取存档。
- `TAB`：查看/退出排行榜（leaderboard）。
- `ESC`：在客户端连接等待时取消连接并返回菜单。

上述控件与行为在 [Game.cpp](Game.cpp#L1) 中实现并可在代码中自定义。

## 配置与存档

- `config.json`：窗口大小、球速、挡板参数、关卡等可配置项（`Game::LoadConfig` 会读取）。
- `levels.json`：关卡数据（默认会在 `Init()` 中通过 `LoadLevelsFromJSON` 加载）。
- `savegame.json`：自动/手动保存的游戏状态（`SaveGameState` / `LoadGameState`）。
- `leaderboard.txt`：排行榜（简单文本格式，分数 与 关卡一行两个数字）。

示例：编辑 `config.json` 来调整初始窗口或道具参数。

## 测试与性能

- 单元/集成测试：`tests/collision_test.cpp`，可使用 CMake 构建并通过 `ctest` 运行。

构建并运行测试示例：

```bash
cd build
cmake --build . --target collision_test -j
ctest -R collision_test --output-on-failure
```

- 性能日志：运行时会生成 `performance_log_<timestamp>.csv`，包含帧数、平均 FPS、活动粒子数与游戏状态，位于运行目录。

## 代码结构（快速导览）

- `main.cpp`：程序入口，创建 `Game` 实例并循环调用 `Init/Update/Draw/Shutdown`。
- `Game.h` / `Game.cpp`：游戏状态机、输入、网络集成、存档及主要流程。
- `Networking.h` / `Networking.cpp`：ENet 封装（`NetworkSession`），实现 JSON 消息接口。
- `Ball.*`, `Paddle.*`, `Brick.*`, `PowerUp.*`：游戏对象与逻辑。

## 贡献与开发

- 欢迎提交 Issue 或 Pull Request。请在 PR 中说明修改目的，尽量保持风格一致并包含必要注释。
- 若贡献网络/跨平台改进，请包含在不同主机上联机测试的说明（端口、防火墙、NAT 穿透等）。

## 联系方式

在仓库 Issue 页提交问题，或通过 `leaderboard.txt` 同级目录与维护者联系。