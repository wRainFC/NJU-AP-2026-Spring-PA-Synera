# Synera Raylib + C++23 设计文档

## 1. 目标与范围

本文档基于 `PA说明文档.md`，设计一个使用 Raylib + C++23 开发的单机轻量级自走棋项目框架。项目目标是完成 Synera: Synergy Auto-Arena 的阶段一至阶段三基础功能，并为阶段四扩展预留接口。

核心交付范围：

- 使用 C++23 实现主要游戏逻辑，体现面向对象设计。
- 使用 Raylib 负责窗口、输入、2D 绘制、音频与资源加载。
- 实现准备、战斗、结算三阶段循环。
- 实现棋盘、备战区、拖拽布阵、商店、经济、羁绊、升星、装备、存档。
- 我方与敌方统一建模为 `Unit`，通过 `Owner` 字段区分控制归属。
- 技能使用多态接口实现，至少提供 3-5 个英雄技能。

本设计优先保证可验收、可迭代、代码边界清晰。画面风格以清晰展示逻辑为主，不把复杂 UI 动画作为基础阶段重点。

## 2. 技术选型

### 2.1 语言与库

- C++ 标准：C++23
- 图形与输入：Raylib
- 构建系统：CMake
- 编译器建议：GCC 13+、Clang 17+、MSVC 2022
- 资源格式：PNG、WAV/OGG、TTF 可选
- 存档格式：版本化文本格式，后续可替换为 JSON

### 2.2 Raylib 职责边界

Raylib 只承担平台与渲染层职责：

- `InitWindow`、`CloseWindow` 管理窗口生命周期。
- `BeginDrawing`、`EndDrawing` 绘制每帧。
- `GetFrameTime` 提供帧时间。
- `GetMousePosition`、`IsMouseButtonPressed`、`IsMouseButtonReleased` 处理拖拽输入。
- `DrawRectangle`、`DrawText`、`DrawTexture` 等绘制棋盘、单位、UI。
- `LoadTexture`、`UnloadTexture` 管理图片资源。
- 阶段四可使用 `InitAudioDevice`、`PlaySound` 等增加音效。

游戏规则、棋盘占用、战斗计算、存档读写不能散落在 Raylib 绘制代码中，应放在逻辑模块。

## 3. 顶层架构

### 3.1 分层结构

项目分为四层：

1. 应用层：窗口、主循环、全局协调。
2. 游戏状态层：保存当前游戏进度、阶段、玩家、单位、棋盘、商店、装备。
3. 系统层：输入、战斗、寻路、商店、羁绊、装备、存档等规则模块。
4. 表现层：Raylib 渲染、资源管理、UI 布局。

建议的核心依赖方向：

```text
main
  -> GameApp
      -> GameState
      -> InputController
      -> CombatSystem / ShopSystem / SynergySystem / SaveSystem
      -> Renderer / AssetManager
```

渲染层读取 `GameState`，但不修改核心规则。输入层把鼠标操作转换为明确的游戏命令，例如“从备战区拖到棋盘某格”。

### 3.2 推荐目录结构

```text
Synera/
  CMakeLists.txt
  PA说明文档.md
  Raylib_Cpp23_设计文档.md
  assets/
    fonts/
    textures/
    audio/
  saves/
    save_01.sav
  src/
    main.cpp
    app/
      GameApp.hpp
      GameApp.cpp
      GameConfig.hpp
    core/
      Types.hpp
      GameState.hpp
      GameState.cpp
      Player.hpp
      Player.cpp
      Unit.hpp
      Unit.cpp
      Ability.hpp
      Ability.cpp
      Equipment.hpp
      Equipment.cpp
      Synergy.hpp
      Synergy.cpp
    board/
      Board.hpp
      Board.cpp
      Bench.hpp
      Bench.cpp
      Pathfinder.hpp
      Pathfinder.cpp
    systems/
      CombatSystem.hpp
      CombatSystem.cpp
      ShopSystem.hpp
      ShopSystem.cpp
      RoundSystem.hpp
      RoundSystem.cpp
      SynergySystem.hpp
      SynergySystem.cpp
      UpgradeSystem.hpp
      UpgradeSystem.cpp
      EquipmentSystem.hpp
      EquipmentSystem.cpp
      SaveSystem.hpp
      SaveSystem.cpp
    ui/
      Renderer.hpp
      Renderer.cpp
      InputController.hpp
      InputController.cpp
      Layout.hpp
      Layout.cpp
      AssetManager.hpp
      AssetManager.cpp
```

阶段一可以先只建立 `app/core/board/ui`，阶段二加入 `CombatSystem/Pathfinder`，阶段三加入 `Shop/Synergy/Equipment/Save`。

## 4. 核心数据模型

### 4.1 基础类型

```cpp
// src/core/Types.hpp
#pragma once

#include <cstdint>
#include <string>

using UnitId = std::uint32_t;

struct AxialPos {
    int q = 0;
    int r = 0;

    friend bool operator==(const AxialPos&, const AxialPos&) = default;
};

struct OffsetPos {
    int col = 0;
    int row = 0;

    friend bool operator==(const OffsetPos&, const OffsetPos&) = default;
};

enum class Owner {
    PlayerCtrl,
    EnemyCtrl
};

enum class Phase {
    Prep,
    Combat,
    Resolve
};

enum class UnitState {
    Idle,
    Moving,
    Attacking,
    Casting,
    Dead
};

enum class Trait {
    Warrior,
    Mage,
    Ranger,
    Guardian,
    Mystic,
    Assassin
};

enum class EquipmentType {
    IronSword,
    ChainVest,
    SwiftGlove,
    ManaCrystal
};
```

### 4.2 玩家实体

`Player` 只代表经营主体，不参与战斗碰撞与攻击。

```cpp
// src/core/Player.hpp
#pragma once

class Player {
public:
    int hp = 100;
    int gold = 8;
    int level = 1;
    int populationCap = 3;
    int currentRound = 1;

    bool isDead() const;
    bool canAfford(int cost) const;
    bool spendGold(int cost);
    void addGold(int amount);
    bool upgradePopulation();
};
```

设计规则：

- `hp <= 0` 游戏失败。
- `populationCap` 限制棋盘上我方单位数量，不限制备战区。
- 金币变化统一通过 `Player` 或经济系统处理，避免 UI 直接修改。

### 4.3 单位实体

我方和敌方都使用同一个 `Unit` 类型，敌我只由 `owner` 区分。英雄差异通过 `templateId`、属性、羁绊、技能对象体现。

```cpp
// src/core/Unit.hpp
#pragma once

#include "Types.hpp"
#include <memory>

class Ability;

struct UnitStats {
    int maxHp = 300;
    int hp = 300;
    int atk = 30;
    int range = 1;
    int maxMana = 60;
    int mana = 0;
    float attackInterval = 1.0f;
    float moveInterval = 0.25f;
};

class Unit {
public:
    UnitId id = 0;
    std::string templateId;
    std::string name;
    Owner owner = Owner::PlayerCtrl;
    UnitState state = UnitState::Idle;
    UnitStats baseStats;
    UnitStats currentStats;
    std::vector<Trait> traits;
    int star = 1;

    std::optional<AxialPos> boardPos;
    std::optional<int> benchSlot;
    std::optional<EquipmentType> equipment;
    UnitId targetId = 0;

    float attackTimer = 0.0f;
    float moveTimer = 0.0f;
    float castTimer = 0.0f;

    std::unique_ptr<Ability> ability;

    bool alive() const;
    bool canAttackTarget(const Unit& target) const;
    void receiveDamage(int amount);
    void heal(int amount);
    void gainMana(int amount);
    void resetForCombat();
    void applyStarMultiplier();
};
```

关键约束：

- `boardPos` 与 `benchSlot` 同时最多只有一个有值。
- 死亡单位不立即从 `GameState::units` 删除，只标记为 `Dead`，方便渲染死亡效果与结算。
- `currentStats` 是基础属性、星级、装备、羁绊 Buff 叠加后的结果。

### 4.4 技能多态接口

技能使用多态满足 PA 对 OOP 的要求。`Ability` 不直接依赖 Raylib，避免技能逻辑和绘制耦合。

```cpp
// src/core/Ability.hpp
#pragma once

#include "Types.hpp"
#include <string>

class GameState;
class Unit;

class Ability {
public:
    virtual ~Ability() = default;
    virtual std::string name() const = 0;
    virtual std::string description() const = 0;
    virtual void cast(Unit& caster, GameState& state) = 0;
};

class FireLineAbility final : public Ability {
public:
    std::string name() const override;
    std::string description() const override;
    void cast(Unit& caster, GameState& state) override;
};

class StunStrikeAbility final : public Ability {
public:
    std::string name() const override;
    std::string description() const override;
    void cast(Unit& caster, GameState& state) override;
};

class HealingAuraAbility final : public Ability {
public:
    std::string name() const override;
    std::string description() const override;
    void cast(Unit& caster, GameState& state) override;
};

class ArrowVolleyAbility final : public Ability {
public:
    std::string name() const override;
    std::string description() const override;
    void cast(Unit& caster, GameState& state) override;
};
```

建议首批英雄：

| 英雄 | 羁绊 | 技能 |
| --- | --- | --- |
| Iron Guard | Warrior + Guardian | 嘲讽或护盾，提升自身生存 |
| Ember Mage | Mage + Mystic | 直线 AOE 伤害 |
| Wind Ranger | Ranger | 多段箭雨或额外普攻 |
| Dawn Cleric | Mystic + Guardian | 范围治疗友军 |
| Shadow Duelist | Assassin | 突进并眩晕单体 |

## 5. 棋盘、备战区与占用规则

### 5.1 棋盘设计

```cpp
// src/board/Board.hpp
#pragma once

#include "../core/Types.hpp"
#include <optional>
#include <vector>

class Board {
public:
    Board(int width, int height);

    int width() const;
    int height() const;
    bool inBounds(AxialPos pos) const;
    bool isPlayerHalf(AxialPos pos) const;
    bool isEnemyHalf(AxialPos pos) const;

    std::optional<UnitId> occupant(AxialPos pos) const;
    bool empty(AxialPos pos) const;
    bool place(UnitId unitId, AxialPos pos);
    void remove(AxialPos pos);
    void move(AxialPos from, AxialPos to);
    void clearEnemies();
    void clearAll();

private:
    int width_;
    int height_;
    std::vector<std::optional<UnitId>> cells_;
    int index(AxialPos pos) const;
};
```

推荐默认棋盘为 `8 x 8` 的 odd-r 外层存储；后端规则统一使用 `AxialPos`，显示时转换为 pointy-top 六边形：

- `y >= 4` 为玩家半场。
- `y < 4` 为敌方半场。
- 准备阶段只能把玩家单位放到玩家半场。
- 战斗阶段双方单位都由 AI 控制，玩家不能拖拽。

### 5.2 备战区设计

```cpp
// src/board/Bench.hpp
#pragma once

#include "../core/Types.hpp"
#include <optional>
#include <vector>

class Bench {
public:
    explicit Bench(int size);

    int size() const;
    std::optional<UnitId> occupant(int slot) const;
    bool empty(int slot) const;
    std::optional<int> firstEmptySlot() const;
    bool place(UnitId unitId, int slot);
    void remove(int slot);
    void swapSlots(int a, int b);

private:
    std::vector<std::optional<UnitId>> slots_;
};
```

拖拽落位策略：

- 备战区到空棋盘格：若人口未满且目标在玩家半场，则上阵。
- 备战区到已占用棋盘格：与目标单位交换，前提是目标单位为玩家单位。
- 棋盘到空备战区：下阵。
- 棋盘到棋盘：若目标空则移动，若目标占用则交换。
- 非法目标：回弹到原位置，不修改逻辑数据。

所有落位修改统一由 `GameState` 或 `BoardPlacementService` 执行，输入层只提交请求。

## 6. 游戏状态与主循环

### 6.1 GameState

```cpp
// src/core/GameState.hpp
#pragma once

#include "Player.hpp"
#include "Unit.hpp"
#include "../board/Board.hpp"
#include "../board/Bench.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

struct ShopOffer {
    std::string unitTemplateId;
    int cost = 1;
};

class GameState {
public:
    Phase phase = Phase::Prep;
    Player player;
    Board board;
    Bench bench;
    std::unordered_map<UnitId, std::unique_ptr<Unit>> units;
    std::vector<ShopOffer> shopOffers;
    std::vector<EquipmentType> equipmentPool;

    explicit GameState(int boardW, int boardH, int benchSize);

    Unit* findUnit(UnitId id);
    const Unit* findUnit(UnitId id) const;
    std::vector<Unit*> aliveUnits();
    std::vector<Unit*> aliveUnitsByOwner(Owner owner);
    std::vector<Unit*> playerBoardUnits();
    std::vector<Unit*> enemyBoardUnits();

    UnitId createUnit(const std::string& templateId, Owner owner);
    void removeDeadEnemiesAfterCombat();
    bool isCombatFinished() const;
    bool playerWonCombat() const;
};
```

`GameState` 是存档的主要来源。凡是需要恢复的内容，都应该能从 `GameState` 序列化。

### 6.2 GameApp 主循环

```cpp
// src/app/GameApp.hpp
#pragma once

class GameApp {
public:
    void run();

private:
    void init();
    void update(float dt);
    void render();
    void shutdown();
};
```

主循环伪代码：

```cpp
void GameApp::run() {
    init();
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        update(dt);
        render();
    }
    shutdown();
}
```

阶段更新逻辑：

```text
Prep:
  - 处理拖拽、购买、刷新、升级人口、装备穿戴、保存/读取。
  - 点击开始战斗后，生成敌人并进入 Combat。

Combat:
  - 禁止拖拽和商店操作。
  - CombatSystem 每帧更新单位状态机、移动、攻击、施法、死亡。
  - 某一方全灭后进入 Resolve。

Resolve:
  - 根据胜负结算金币、血量、掉落装备、推进轮次。
  - 清理敌方单位，重置我方单位状态。
  - 若游戏未结束，回到 Prep。
```

## 7. 战斗系统设计

### 7.1 CombatSystem 接口

```cpp
// src/systems/CombatSystem.hpp
#pragma once

class GameState;
class Unit;

class CombatSystem {
public:
    void startCombat(GameState& state);
    void update(GameState& state, float dt);
    void finishCombat(GameState& state);

private:
    void updateUnit(GameState& state, Unit& unit, float dt);
    void updateIdle(GameState& state, Unit& unit);
    void updateMoving(GameState& state, Unit& unit, float dt);
    void updateAttacking(GameState& state, Unit& unit, float dt);
    void updateCasting(GameState& state, Unit& unit, float dt);
    void acquireTarget(GameState& state, Unit& unit);
    void performAttack(GameState& state, Unit& attacker, Unit& target);
};
```

### 7.2 单位状态机

状态转移：

```text
Idle
  -> Dead: hp <= 0
  -> Casting: mana >= maxMana
  -> Attacking: 已有目标且目标在攻击范围内
  -> Moving: 已有目标但不在攻击范围内

Moving
  -> Idle: 目标死亡或路径失效
  -> Attacking: 进入攻击范围
  -> Dead: hp <= 0

Attacking
  -> Casting: 普攻回蓝后法力满
  -> Idle: 目标死亡或离开范围
  -> Dead: hp <= 0

Casting
  -> Idle: 技能释放完成
  -> Dead: hp <= 0

Dead
  -> 无转出
```

为了降低复杂度，阶段二可以先采用“移动一格一个节拍”的离散移动方式。每 `moveInterval` 秒尝试向路径下一格移动，成功后更新棋盘占用。

### 7.3 索敌规则

目标候选集合：

```text
Opp(u) = 所有 owner 不同且 alive 的单位
```

排序规则：

1. 六边形距离更小者优先。
2. 当前生命值更低者优先，便于集中火力完成击杀。
3. `UnitId` 更小者优先，保证完全稳定。

若验收时要求生命值高者优先，只需要调整第二条比较器，不影响其他模块。

### 7.4 寻路与碰撞

```cpp
// src/board/Pathfinder.hpp
#pragma once

#include "../core/Types.hpp"
#include "Board.hpp"
#include <optional>
#include <vector>

class Pathfinder {
public:
    std::vector<AxialPos> findPathToAttackRange(
        const Board& board,
        AxialPos start,
        AxialPos target,
        int attackRange
    ) const;
};
```

实现策略：

- 使用 A*，邻居来自 axial 六方向，启发函数使用 hex distance。
- 起点是当前单位位置。
- 终点不是目标所在格，而是距离目标 `attackRange` 内的可站立空格。
- 目标单位所在格不可通过。
- 其他存活单位所在格不可通过。
- 每次移动前重新检查下一格是否仍为空，防止多个单位重叠。
- 若无路可走，单位保持 Idle 或短暂等待，下帧重新索敌或重新寻路。

距离计算：

```text
hexDistance(a, b) = (abs(dq) + abs(dr) + abs(ds)) / 2
其中 ds = -dq - dr
可攻击条件：hexDistance <= range
```

由于棋盘采用六边形布局，索敌、攻击范围和寻路统一使用 hex distance。

## 8. 商店、经济与人口

### 8.1 ShopSystem

```cpp
// src/systems/ShopSystem.hpp
#pragma once

#include <string>

class GameState;

class ShopSystem {
public:
    void refresh(GameState& state, bool payCost);
    bool buy(GameState& state, int offerIndex);

private:
    std::string rollUnitTemplate(int playerLevel) const;
};
```

规则建议：

- 商店固定 5 个招募位。
- 手动刷新花费 2 金币。
- 单位费用按英雄模板设置，初期全部 1-3 金币即可。
- 购买后扣金币并放入备战区第一个空位。
- 若备战区已满或金币不足，购买失败，不改变状态。
- 购买成功后立即调用升星检查。

### 8.2 经济结算

基础版本：

| 场景 | 奖励 |
| --- | --- |
| 战斗胜利 | +6 金币 |
| 战斗失败 | +3 金币，玩家扣血 |
| 手动刷新 | -2 金币 |
| 升级人口 | 按等级递增扣费 |

人口升级费用示例：

```text
level 1 -> 2: 4 gold, cap +1
level 2 -> 3: 6 gold, cap +1
level 3 -> 4: 8 gold, cap +1
```

阶段四扩展可加入利息和连胜/连败奖励，接口放在 `RoundSystem` 内。

## 9. 羁绊系统

### 9.1 数据模型

```cpp
// src/core/Synergy.hpp
#pragma once

#include "Types.hpp"
#include <string>
#include <vector>

enum class SynergyEffectKind {
    StatAura,
    Mechanic
};

struct SynergyTier {
    int requiredCount = 0;
    int hpBonus = 0;
    int atkBonus = 0;
    float attackSpeedMultiplier = 1.0f;
    float skillDamageMultiplier = 1.0f;
};

struct SynergyDefinition {
    Trait trait;
    std::string name;
    SynergyEffectKind kind;
    std::vector<SynergyTier> tiers;
};
```

### 9.2 SynergySystem

```cpp
// src/systems/SynergySystem.hpp
#pragma once

class GameState;

class SynergySystem {
public:
    void recompute(GameState& state);

private:
    void clearTemporaryBuffs(GameState& state);
    void applyActiveSynergies(GameState& state);
};
```

羁绊只统计我方场上存活或待战斗单位，不统计备战区，不统计敌人。

建议实现 5 种羁绊：

| 羁绊 | 阈值 | 类型 | 效果 |
| --- | --- | --- | --- |
| Warrior | 2/4 | 属性光环 | 战士 +100/+250 最大生命 |
| Guardian | 2/4 | 属性光环 | 守护者受到伤害降低 10%/20% |
| Mage | 3 | 机制改变 | 全体友军技能伤害 x1.5 |
| Ranger | 2/4 | 机制改变 | 游侠 20%/35% 概率额外普攻一次 |
| Mystic | 2/3 | 属性光环 | 全体友军最大法力需求 -10/-20 |

实现重点：

- 每次布阵、购买、升星、装备变化、战斗开始前调用 `recompute`。
- 先恢复单位基础属性，再叠加星级、装备、羁绊，避免 Buff 重复叠加。
- 机制类效果可记录在 `GameState` 的战斗修正字段中，例如 `skillDamageMultiplier`。

## 10. 升星与装备

### 10.1 升星机制

```cpp
// src/systems/UpgradeSystem.hpp
#pragma once

class GameState;

class UpgradeSystem {
public:
    bool tryMergeAfterGain(GameState& state, UnitId gainedUnitId);
};
```

合并规则：

- 只在准备阶段执行。
- 在我方棋盘和备战区中寻找同 `templateId`、同 `star` 的单位。
- 满 3 个 1 星合成 1 个 2 星。
- 合并后的单位保留在最近获得的第 3 个单位位置。
- 被合并消耗的两个单位从棋盘或备战区移除。
- 2 星单位属性乘以 1.7，装备保留第 3 个单位的装备。

### 10.2 装备设计

```cpp
// src/core/Equipment.hpp
#pragma once

#include "Types.hpp"
#include <string>

struct EquipmentDefinition {
    EquipmentType type;
    std::string name;
    int atkBonus = 0;
    int hpBonus = 0;
    float attackIntervalMultiplier = 1.0f;
    int maxManaDelta = 0;
};
```

基础装备：

| 装备 | 效果 |
| --- | --- |
| 铁剑 | 攻击力 +15 |
| 锁子甲 | 最大生命 +150 |
| 急速手套 | 攻击间隔 x0.8 |
| 蓝水晶 | 最大法力值 -30，最低不低于 20 |

装备交互：

- 装备栏在备战区旁边显示。
- 鼠标拖拽装备到单位身上。
- 每个 1 星单位最多 1 件装备。
- 若单位已有装备，则穿戴失败并回弹。
- 装备属性通过 `EquipmentSystem::recomputeStats` 叠加到 `currentStats`。

## 11. 关卡与敌方生成

```cpp
// src/systems/RoundSystem.hpp
#pragma once

class GameState;

class RoundSystem {
public:
    void startRound(GameState& state);
    void spawnEnemies(GameState& state);
    void resolveRound(GameState& state, bool playerWon);
    bool isGameWon(const GameState& state) const;
};
```

敌方生成建议：

| 轮次 | 敌人 |
| --- | --- |
| 1 | 1 个低血量近战 |
| 2 | 2 个近战 |
| 3 | 2 近战 + 1 远程 |
| 4+ | 属性每 3 轮提升约 20%，数量逐渐增加 |

实现方式：

- 阶段二先用硬编码关卡表。
- 阶段三可把模板和关卡表集中放在 `GameConfig.hpp`。
- 敌方放置在敌方半场固定阵型或按轮次预设坐标。
- 回合结束后删除敌方单位，保留玩家单位。

## 12. UI 与输入设计

### 12.1 布局

推荐窗口尺寸 `1280 x 720`。

```text
左上：玩家 HP / 金币 / 人口 / 当前轮次 / 当前阶段
中央：8 x 8 主棋盘
下方：8 格备战区
右侧：5 格商店、刷新按钮、升级人口按钮、开始战斗按钮
右下：装备栏、当前激活羁绊
鼠标悬停：单位属性面板
```

Raylib 中建议用 `Rectangle` 保存每个 UI 区域。`Layout` 负责把逻辑格子映射到屏幕坐标。

```cpp
// src/ui/Layout.hpp
#pragma once

#include "../core/Types.hpp"
#include "raylib.h"

class Layout {
public:
    Vector2 boardHexCenter(AxialPos pos) const;
    std::array<Vector2, 6> boardHexCorners(AxialPos pos) const;
    Rectangle boardHexBounds(AxialPos pos) const;
    Rectangle benchSlotRect(int slot) const;
    Rectangle shopOfferRect(int index) const;
    Rectangle equipmentSlotRect(int index) const;
    std::optional<AxialPos> boardPosAt(Vector2 mouse) const;
    std::optional<int> benchSlotAt(Vector2 mouse) const;
};
```

### 12.2 InputController

```cpp
// src/ui/InputController.hpp
#pragma once

class GameState;
class Layout;

class InputController {
public:
    void update(GameState& state, const Layout& layout);

private:
    void beginDrag(GameState& state, const Layout& layout);
    void updateDrag();
    void endDrag(GameState& state, const Layout& layout);
    void handleButtons(GameState& state, const Layout& layout);
};
```

拖拽状态：

```cpp
enum class DragKind {
    None,
    UnitFromBoard,
    UnitFromBench,
    Equipment
};

struct DragState {
    DragKind kind = DragKind::None;
    UnitId unitId = 0;
    int sourceSlot = -1;
    std::optional<AxialPos> sourcePos;
    int equipmentIndex = -1;
};
```

输入限制：

- `Prep` 阶段允许拖拽单位、购买、刷新、升级、装备。
- `Combat` 阶段只允许查看信息。
- `Resolve` 阶段可以自动停留 1 秒后进入下一准备阶段，或点击继续。

### 12.3 Renderer

```cpp
// src/ui/Renderer.hpp
#pragma once

class GameState;
class Layout;

class Renderer {
public:
    void draw(const GameState& state, const Layout& layout);

private:
    void drawBoard(const GameState& state, const Layout& layout);
    void drawBench(const GameState& state, const Layout& layout);
    void drawUnits(const GameState& state, const Layout& layout);
    void drawBars(const Unit& unit, Rectangle rect);
    void drawShop(const GameState& state, const Layout& layout);
    void drawEquipment(const GameState& state, const Layout& layout);
    void drawSynergies(const GameState& state);
    void drawTopBar(const GameState& state);
};
```

最低 GUI 表达要求：

- 棋盘格区分玩家半场和敌方半场。
- 单位显示名称、星级、归属颜色。
- 单位头顶显示生命条和法力条。
- 商店显示 5 个可购买单位和价格。
- 顶栏显示 HP、金币、人口、轮次、阶段。
- 装备栏显示装备名称或图标。
- 激活羁绊显示名称与当前层级。

## 13. 存档与读档

### 13.1 SaveSystem

```cpp
// src/systems/SaveSystem.hpp
#pragma once

#include <string>

class GameState;

class SaveSystem {
public:
    bool save(const GameState& state, const std::string& path) const;
    bool load(GameState& state, const std::string& path) const;
};
```

### 13.2 存档内容

必须保存：

- 存档版本号。
- 玩家 HP、金币、等级、人口、当前轮次。
- 当前阶段，建议只允许在 `Prep` 阶段保存，降低恢复复杂度。
- 所有我方单位：`id`、`templateId`、`name`、`star`、`hp`、`mana`、位置、装备。
- 备战区占用。
- 棋盘我方占用。
- 商店 5 个 offer。
- 装备池。

建议不保存：

- 敌方战斗中间状态。
- 临时路径、攻击计时器、拖拽状态。
- 羁绊临时 Buff，读档后重新计算。

版本化文本格式示例：

```text
SYNERA_SAVE 1
PLAYER 100 12 2 4 3
PHASE Prep
UNIT 1 ember_mage EmberMage 1 280 20 BENCH 0 EQUIP None
UNIT 2 iron_guard IronGuard 2 700 0 BOARD 3 6 EQUIP ChainVest
SHOP ember_mage iron_guard wind_ranger dawn_cleric shadow_duelist
EQUIPMENT IronSword ManaCrystal
END
```

保存失败要在 UI 上显示提示，不应导致游戏崩溃。

## 14. CMake 与框架搭建

### 14.1 CMakeLists.txt 草案

```cmake
cmake_minimum_required(VERSION 3.24)
project(Synera LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(synera
    src/main.cpp
    src/app/GameApp.cpp
    src/core/GameState.cpp
    src/core/Player.cpp
    src/core/Unit.cpp
    src/core/Ability.cpp
    src/core/Equipment.cpp
    src/core/Synergy.cpp
    src/board/Board.cpp
    src/board/Bench.cpp
    src/board/Pathfinder.cpp
    src/systems/CombatSystem.cpp
    src/systems/ShopSystem.cpp
    src/systems/RoundSystem.cpp
    src/systems/SynergySystem.cpp
    src/systems/UpgradeSystem.cpp
    src/systems/EquipmentSystem.cpp
    src/systems/SaveSystem.cpp
    src/ui/Renderer.cpp
    src/ui/InputController.cpp
    src/ui/Layout.cpp
    src/ui/AssetManager.cpp
)

find_package(raylib CONFIG REQUIRED)
target_link_libraries(synera PRIVATE raylib)

target_include_directories(synera PRIVATE src)

if (MSVC)
    target_compile_options(synera PRIVATE /W4)
else()
    target_compile_options(synera PRIVATE -Wall -Wextra -Wpedantic)
endif()
```

Raylib 安装方式可选：

- Linux：使用发行版包管理器安装 raylib 开发包，或从源码安装。
- Windows：使用 vcpkg 安装 `raylib`，CMake 配置时指定 toolchain。
- macOS：使用 Homebrew 安装 raylib。

### 14.2 main.cpp 草案

```cpp
#include "app/GameApp.hpp"

int main() {
    GameApp app;
    app.run();
    return 0;
}
```

### 14.3 GameApp 初始化草案

```cpp
void GameApp::init() {
    InitWindow(1280, 720, "Synera: Synergy Auto-Arena");
    SetTargetFPS(60);
    // 初始化 GameState、资源、商店首刷。
}

void GameApp::shutdown() {
    // 释放纹理、音频等 Raylib 资源。
    CloseWindow();
}
```

### 14.4 构建命令

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/synera
```

如果使用 vcpkg：

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## 15. 阶段实现路线

### 15.1 阶段一：棋盘、单位、拖拽、基础 GUI

优先实现：

- `Board`、`Bench`、`GameState`、`Unit`。
- 玩家和敌人的统一 `Unit` 类型。
- `Renderer` 绘制棋盘、备战区、单位、血条、蓝条。
- `InputController` 支持备战区和玩家半场之间拖拽。
- 非法放置回弹，已占用格子交换。
- 简单敌方生成，用于展示敌方半场单位。

验收目标：

- 能看见棋盘和备战区。
- 能拖拽我方单位上阵和下阵。
- 棋盘占用与逻辑数据一致。
- 单位基础属性可展示。

### 15.2 阶段二：战斗流程、寻路、普攻、技能

优先实现：

- `Phase` 三阶段循环。
- `RoundSystem::spawnEnemies`。
- `CombatSystem` 状态机。
- A* 寻路与防重叠移动。
- 六边形距离索敌和平局规则。
- 普攻、回蓝、死亡判定。
- 至少 3 个 `Ability` 派生类。

验收目标：

- 点击开始战斗后，双方自动移动和攻击。
- 一方全灭后进入结算。
- 单位不会重叠到同一个格子。
- 技能能自动释放且效果不同。

### 15.3 阶段三：完整经营系统

优先实现：

- 金币奖励、商店 5 格、购买、刷新。
- 人口上限和升级。
- 4-6 种羁绊，至少 2 种属性光环、1 种机制改变。
- 升星 3 合 1。
- 4 种基础装备和拖拽穿戴。
- 存档与读档。
- 完整 UI 信息展示。

验收目标：

- 准备阶段有完整经营闭环。
- 战斗胜负影响金币、血量、轮次。
- 存档后重启能恢复进度。
- UI 能清晰展示经济、人口、羁绊、装备、星级。

### 15.4 阶段四：推荐扩展

推荐选择“视听结合”作为阶段四扩展，和 Raylib 最契合：

- 普攻弹道 `Projectile`。
- 技能范围特效。
- 背景音乐和攻击音效。
- 击杀或胜利提示动画。

这类扩展展示效果明显，且不会破坏基础系统。可新增：

```text
src/core/Projectile.hpp
src/systems/ProjectileSystem.hpp
src/ui/EffectRenderer.hpp
```

## 16. 测试与调试策略

建议把核心规则写成不依赖 Raylib 的普通 C++ 类，这样后续可以补单元测试。即使不引入测试框架，也应保留调试入口和日志。

重点测试场景：

- 棋盘越界、占用、交换、回弹。
- 人口已满时不能上阵。
- 商店金币不足、备战区满时购买失败。
- 三个同名同星单位自动合成。
- 装备不能重复穿戴。
- 单位死亡后不再被索敌。
- 两个单位同时移动时不会进入同一格。
- 存档读档后金币、单位、装备、商店一致。

调试建议：

- 在 Debug 模式显示格子坐标和单位 `id`。
- 给 `CombatSystem` 加简单日志开关。
- 保持固定随机种子，方便复现商店和掉落问题。

## 17. 关键设计决策

1. 统一 `Unit` 类型，不为敌人和我方建立两套类体系，符合 PA 的对象模型要求。
2. 技能多态放在 `Ability` 层，而不是为每个英雄派生一个完整 `Unit` 子类，能减少重复字段。
3. `GameState` 保存全部可持久化状态，系统模块只操作它，不保存分散状态。
4. Raylib 代码集中在 `ui` 和 `app`，核心规则不依赖渲染库，方便测试和维护。
5. 阶段二使用六边形 A* 离散格子移动，满足阻挡和防重叠要求，同时与 pointy-top 棋盘一致。
6. 存档建议只允许在准备阶段进行，避免战斗中路径、计时器、临时 Buff 的恢复复杂度。

## 18. 最小可运行骨架顺序

建议按以下顺序搭建第一版可运行程序：

1. 创建 CMake、`main.cpp`、`GameApp`，打开 Raylib 窗口。
2. 创建 `Board`、`Bench`、`Unit`、`GameState`，硬编码 3 个玩家单位。
3. 完成 `Layout` 和 `Renderer`，绘制棋盘、备战区、单位。
4. 完成 `InputController`，实现拖拽与交换。
5. 加入 `RoundSystem`，点击按钮生成敌人并切换阶段。
6. 加入 `CombatSystem`，先实现站桩攻击，再加入移动和 A*。
7. 加入技能多态。
8. 加入商店、金币、人口。
9. 加入羁绊、升星、装备。
10. 加入存档读档和完整 UI 状态提示。

按这个顺序推进，每一步都能得到一个可演示版本，便于阶段验收和调试。
