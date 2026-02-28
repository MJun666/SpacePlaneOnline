# 🚀 联机风暴 (SpacePlaneOnline)

基于 C++ 编写的 2D 多人在线弹幕射击游戏 (STG)。
本项目从零实现了一个**纯服务端权威 (Server-Authoritative)** 的网络同步架构，彻底杜绝了客户端作弊的可能，并实现了多玩家同屏流畅战斗。

## 🌟 核心游戏特性
* **无尽风暴 (动态难度)**：随着服务器运行时间增加，敌机生成频率与弹幕密度呈几何级数上升。
* **战利品天降**：击毁敌机有概率全服掉落增益道具，物理弹跳轨迹全网实时同步。
* **全服荣耀榜 (Global Leaderboard)**：基于本地二进制落盘的持久化积分排行榜，记录全服最强王者，无惧服务器重启。

## 🛠️ 核心技术栈
* **开发语言**：C++ 14/17
* **网络通信**：Boost.Asio (基于 TCP 协议的异步网络框架)
* **数据序列化**：Google Protocol Buffers (Protobuf)
* **图形与音频渲染**：SDL2, SDL_image, SDL_ttf, SDL_mixer
* **包管理工具**：vcpkg

## ⚙️ 后端架构亮点 (Server-Side)
1. **权威服务器架构 (Server-Authoritative)**：
   - 客户端仅负责发送按键指令 (`UP/DOWN/FIRE`) 与插值渲染。
   - 服务器接管所有的核心物理运算：包括玩家坐标校验、子弹生成与飞行轨迹、AABB 碰撞检测与伤害判定。
2. **状态同步机制**：
   - 服务器以固定 TickRate (约 30 Tick/s) 运行物理逻辑。
   - 每帧计算完毕后，将全局 `GameSnapshot` (包含所有玩家、子弹、敌机、道具坐标) 打包广播给所有存活的客户端。
3. **零依赖的数据持久化**：
   - 摒弃了沉重的外部数据库，复用 Protobuf 序列化机制，将内存中的排行榜数据直接以二进制流 (`.dat`) 高效落盘持久化。

## 🚀 编译与运行指南
1. 确保已配置好 `vcpkg` 并安装了所需的依赖 (`sdl2`, `boost-asio`, `protobuf`)。
2. 使用 Visual Studio 或 CMake 编译 `GameServer` 与 `GameClient`。
3. **运行顺序**：
   - 首先运行 `GameServer.exe` 启动服务端。
   - 随后运行一个或多个 `GameClient.exe` 接入服务器开始游戏。