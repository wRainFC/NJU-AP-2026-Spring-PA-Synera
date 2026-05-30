# Synera 实施计划

本文档把 `Raylib_Cpp23_设计文档.md` 落地成可执行开发计划。实现顺序不按 `PA说明文档.md` 的章节推进，而是按顶层架构先后关系推进：先稳定数据模型、系统边界和主循环，再做一条完整可玩的纵向闭环，最后横向补齐战斗、经营、羁绊、装备和存档细节。

## 1. 开发原则

1. 先定架构边界，再做功能细节。
2. 尽早做一条从准备到战斗到结算再回准备的纵向薄片。
3. 我方单位与敌方单位统一使用 `Unit`，只通过 `Owner` 区分敌我。
4. 核心规则不依赖 Raylib，Raylib 只负责窗口、输入、绘制和音频。
5. 战斗、商店、羁绊、装备、存档分别放在独立系统类中。
6. 每个迭代都必须有一个能演示的版本。
7. 代码要能解释，尤其是 AI 辅助生成的模块。

## 2. 最小目录框架

第一版只创建必要目录和文件，避免过早拆得太碎。

```text
Synera/
  CMakeLists.txt
  PLAN.md
  PA说明文档.md
  Raylib_Cpp23_设计文档.md
  README.md
  assets/
    textures/
    audio/
    fonts/
  saves/
  src/
    main.cpp
    app/
      GameApp.hpp
      GameApp.cpp
      GameConfig.hpp
    core/
      Types.hpp
      Contract.hpp
      Player.hpp
      Player.cpp
      Unit.hpp
      Unit.cpp
      Ability.hpp
      Ability.cpp
      GameState.hpp
      GameState.cpp
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
      RoundSystem.hpp
      RoundSystem.cpp
      ShopSystem.hpp
      ShopSystem.cpp
      SynergySystem.hpp
      SynergySystem.cpp
      UpgradeSystem.hpp
      UpgradeSystem.cpp
      EquipmentSystem.hpp
      EquipmentSystem.cpp
      SaveSystem.hpp
      SaveSystem.cpp
    ui/
      Layout.hpp
      Layout.cpp
      InputController.hpp
      InputController.cpp
      Renderer.hpp
      Renderer.cpp
```

后续如果资源管理变复杂，再加入 `AssetManager`。第一版可以直接用纯色矩形和文字完成 GUI。

## 3. 模块职责

### 3.1 `app`

- `main.cpp`：只创建 `GameApp` 并调用 `run()`。
- `GameApp`：初始化 Raylib、维护主循环、调用输入、系统更新和渲染。
- `GameConfig`：保存棋盘大小、窗口大小、初始金币、商店刷新费用、装备掉落概率等常量。

`GameApp` 不直接写战斗细节，不直接修改单位属性。

### 3.2 `core`

- `Types`：枚举、ID、坐标、基础值类型。
- `Contract`：前置条件、后置条件、不变量检查工具。
- `Player`：玩家血量、金币、等级、人口、当前轮次。
- `Unit`：战斗实体，包含属性、状态、位置、羁绊、装备、技能指针。
- `Ability`：技能基类和 3-5 个派生技能。
- `GameState`：当前游戏完整状态，是存档读写的核心来源。

### 3.3 `board`

- `Board`：主棋盘占用、放置、移动、清空。
- `Bench`：备战区槽位占用。
- `Pathfinder`：A* 寻路，返回下一步路径。

棋盘和备战区只维护占用关系，不负责金币、人口、技能。

### 3.4 `systems`

- `RoundSystem`：准备、战斗、结算切换，敌人生成，胜负结算。
- `CombatSystem`：单位状态机、索敌、移动、普攻、施法、死亡。
- `ShopSystem`：刷新商店、购买单位。
- `SynergySystem`：统计羁绊并应用 Buff。
- `UpgradeSystem`：同名同星 3 合 1。
- `EquipmentSystem`：装备掉落、穿戴、属性应用。
- `SaveSystem`：存档与读档。

系统类只操作 `GameState`，不直接调用 Raylib 绘制函数。

### 3.5 `ui`

- `Layout`：Axial 逻辑坐标到 pointy-top 六边形屏幕区域的转换。
- `InputController`：鼠标点击、拖拽、按钮操作。
- `Renderer`：绘制棋盘、单位、血条、蓝条、商店、装备、羁绊和顶栏。

UI 可以读取 `GameState`，但复杂规则必须委托给系统类。

## 4. 实现路线

本路线按工程依赖关系组织，不按 PA 阶段组织。每个里程碑都要求保持程序可编译、可运行、可演示。

### Milestone 0：项目骨架与工程约束

目标：先让工程能稳定构建，并建立后续代码都必须遵守的边界。

完成：

- `CMakeLists.txt`
- `main.cpp`
- `GameApp`
- `GameConfig`
- `Contract.hpp`
- `.clang-format`
- 空的目录结构

验收：

- 能打开 `1280 x 720` Raylib 窗口。
- 主循环只做 `update(dt)` 和 `render()` 调度。
- 所有常量先放入 `GameConfig`，不在逻辑中散落魔法数字。

### Milestone 1：领域模型与不变量

目标：先稳定游戏状态模型，后续系统都围绕它工作。

完成：

- `Types`
- `Player`
- `Unit`
- `Ability` 基类和 1 个空技能实现
- `Board`
- `Bench`
- `GameState`

验收：

- 能创建玩家、棋盘、备战区和若干单位。
- `Unit` 能表示我方和敌方。
- `boardPos` 和 `benchSlot` 的互斥关系有契约检查。
- 棋盘与备战区占用规则通过简单调试初始化验证。

这一步还不要求 GUI 完整，也不要求拖拽。重点是先让数据结构正确。

### Milestone 2：状态流与系统调度

目标：先建立游戏生命周期，让各系统有明确调用时机。

完成：

- `Phase`
- `RoundSystem`
- `CombatSystem` 空实现
- `ShopSystem` 空实现
- `SynergySystem` 空实现
- `EquipmentSystem` 空实现
- `UpgradeSystem` 空实现
- `SaveSystem` 空实现

验收：

- 程序能在 `Prep -> Combat -> Resolve -> Prep` 之间切换。
- 切换时通过日志或屏幕文本显示当前阶段。
- `GameApp` 只负责调度，不把规则写进主循环。

这一步先允许按钮或快捷键直接切换阶段，不要求战斗真的发生。

### Milestone 3：最小可视化与命令式输入

目标：把数据状态可视化，并建立 UI 到规则层的命令通道。

完成：

- `Layout`
- `Renderer`
- `InputController`
- 顶栏、棋盘、备战区、单位、按钮的基础绘制
- `GameCommand` 或等价的明确操作函数

验收：

- 能看到棋盘、备战区、当前阶段、金币、人口、轮次。
- 单位能以文本或色块显示，包含 HP/Mana 条。
- 输入层不直接改 `Board::cells_` 或 `Unit` 位置，只调用放置/购买/开始战斗等明确接口。

这一步可以用点击按钮触发逻辑，不急着做完整拖拽。

### Milestone 4：第一条完整纵向薄片

目标：尽早完成最小可玩的完整循环，即使规则非常粗糙。

完成：

- 准备阶段：把一个我方单位放到棋盘。
- 开战：生成一个敌人。
- 战斗：双方站桩互相攻击。
- 结算：胜利加金币，失败扣血。
- 回到准备阶段。

验收：

- 从打开游戏开始，能完整跑完一轮。
- 一方死亡后进入结算。
- 结算后能开始下一轮。
- 游戏状态没有因为阶段切换而出现空指针、重复占用或死亡单位继续攻击。

这一步不做 A* 寻路、不做商店、不做复杂技能。它验证架构是否能支持完整闭环。

### Milestone 5：布阵与人口规则

目标：补齐准备阶段最核心的交互，并把人口规则接入放置逻辑。

完成：

- 单位拖拽。
- 备战区到玩家半场。
- 玩家半场到备战区。
- 棋盘内移动。
- 占用格交换。
- 非法放置回弹。
- 人口上限检查。

验收：

- 只有 `Prep` 阶段可拖拽。
- 玩家单位只能布置在玩家半场。
- 人口满时不能继续上阵。
- 任意时刻一个格子最多一个单位。

### Milestone 6：战斗核心深化

目标：把战斗从站桩扩展为自动战斗。

完成：

- 单位状态机。
- 六边形距离索敌和平局规则。
- 普攻计时。
- 回蓝。
- 死亡判定。
- `Pathfinder::findPathToAttackRange`
- A* 移动和防重叠。

验收：

- 近战单位会向目标靠近。
- 目标死亡后重新索敌。
- 单位不会走进同一个格子。
- 一方全灭后稳定进入结算。

### Milestone 7：英雄差异与技能多态

目标：在稳定战斗循环上加入 OOP 多态能力。

完成：

- 至少 3 个派生技能。
- 英雄模板或工厂。
- 法力满时自动释放。
- 技能伤害、治疗、眩晕等效果。

建议技能：

- `FireLineAbility`：直线 AOE。
- `StunStrikeAbility`：单体伤害加短暂眩晕。
- `HealingAuraAbility`：范围治疗友军。
- `ArrowVolleyAbility`：多目标伤害。

验收：

- 不同英雄技能效果不同。
- 技能通过虚函数调用。
- 释放后法力清零。
- 技能逻辑不依赖 Raylib。

### Milestone 8：经营闭环

目标：让准备阶段从“手动摆预设单位”变成实际经营。

完成：

- `ShopSystem`
- 商店 5 格。
- 购买单位。
- 刷新商店。
- 金币支出与奖励。
- 升级人口。
- 购买后自动放入备战区。

验收：

- 金币不足时购买失败。
- 备战区满时购买失败。
- 上阵人数受人口限制。
- 战斗结算会影响下一轮经营选择。

### Milestone 9：成长系统

目标：加入长期策略价值，但不破坏已有闭环。

完成：

- `SynergySystem`
- 4-6 种羁绊。
- 至少 2 种属性光环和 1 种机制改变。
- `UpgradeSystem`
- 同名同星 3 合 1。
- `EquipmentSystem`
- 至少 4 种基础装备。
- 装备掉落和穿戴。

验收：

- 布阵变化会触发羁绊重算。
- 升星后属性提升且位置正确。
- 装备不会重复穿戴。
- 装备和羁绊的属性修改不会重复叠加。

### Milestone 10：持久化与文档闭环

目标：把项目变成可验收、可恢复、可解释的完整作品。

完成：

- `SaveSystem`
- 只允许在 `Prep` 阶段保存。
- 读档后重算羁绊、装备和派生属性。
- README。
- AI 使用说明。

验收：

- 存档读档能恢复金币、轮次、单位、装备、商店。
- 读档失败不会崩溃。
- README 能解释目录结构、核心类、关键算法和 AI 辅助部分。

## 5. 核心接口约定

### 5.1 基础类型

```cpp
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
    Stunned,
    Dead
};
```

### 5.2 `Board`

```cpp
class Board {
public:
    Board(int width, int height);

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] bool inBounds(AxialPos pos) const noexcept;
    [[nodiscard]] bool isPlayerHalf(AxialPos pos) const noexcept;
    [[nodiscard]] bool isEnemyHalf(AxialPos pos) const noexcept;
    [[nodiscard]] std::optional<UnitId> occupant(AxialPos pos) const;
    [[nodiscard]] bool empty(AxialPos pos) const;

    bool place(UnitId unitId, AxialPos pos);
    void remove(AxialPos pos);
    bool move(AxialPos from, AxialPos to);

    template <std::invocable<AxialPos> Visitor>
    void forEachNeighbor(AxialPos pos, Visitor&& visitor) const;
};
```

约定：

- 后端战斗、距离、寻路只使用 `AxialPos`。
- `Board` 内部允许通过 odd-r `OffsetPos` 映射到连续数组。
- 越界坐标不能写入。
- `place` 遇到占用直接返回 `false`。
- `move` 只负责占用变化，不修改 `Unit::boardPos`，调用方负责同步。

### 5.3 `Unit`

```cpp
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
    std::unique_ptr<Ability> ability;

    [[nodiscard]] bool alive() const noexcept;
    [[nodiscard]] bool onBoard() const noexcept;
    [[nodiscard]] bool onBench() const noexcept;
    [[nodiscard]] bool canAttackTarget(const Unit& target) const;

    void receiveDamage(int amount);
    void heal(int amount);
    void gainMana(int amount);
    void resetForCombat();
};
```

约定：

- `boardPos` 和 `benchSlot` 不能同时有值。
- `hp <= 0` 后必须进入 `Dead`。
- `currentStats` 由基础属性、星级、装备、羁绊重算得到，不能反复叠加。

### 5.4 `Ability`

```cpp
class Ability {
public:
    virtual ~Ability() = default;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual std::string_view description() const noexcept = 0;
    virtual void cast(Unit& caster, GameState& state) = 0;
};
```

约定：

- 技能逻辑不调用 Raylib。
- 技能通过 `GameState` 查找目标或友军。
- 具体技能必须是派生类，满足 PA 的多态要求。

### 5.5 `GameState`

```cpp
class GameState {
public:
    Phase phase = Phase::Prep;
    Player player;
    Board board;
    Bench bench;
    std::unordered_map<UnitId, std::unique_ptr<Unit>> units;

    [[nodiscard]] Unit* findUnit(UnitId id);
    [[nodiscard]] const Unit* findUnit(UnitId id) const;
    [[nodiscard]] std::vector<Unit*> aliveUnitsByOwner(Owner owner);
    [[nodiscard]] std::vector<Unit*> playerBoardUnits();
    [[nodiscard]] std::vector<Unit*> enemyBoardUnits();

    UnitId createUnit(std::string_view templateId, Owner owner);
};
```

约定：

- 所有可存档状态优先放在 `GameState`。
- 临时 UI 状态不放在 `GameState`，例如拖拽中的鼠标位置。
- 战斗临时计时器可以放在 `Unit`，但读档后可重置。

## 6. 代码风格规范

PA 明确把“代码风格”列入评分，并要求 OOP、STL、异常处理和格式化。本项目按以下规则执行。

### 6.1 命名

- 类型名：`PascalCase`，例如 `CombatSystem`、`AxialPos`。
- 函数名：`camelCase`，例如 `findUnit`、`startCombat`。
- 变量名：`camelCase`，例如 `currentRound`。
- 私有成员：尾部加 `_`，例如 `width_`、`cells_`。
- 枚举值：`PascalCase`，例如 `Owner::PlayerCtrl`。
- 文件名：类型文件使用 `PascalCase.hpp/.cpp`。

### 6.2 文件组织

- 头文件只放声明和很短的内联函数。
- 源文件放实现。
- 每个 `.cpp` 先包含自己的头文件，再包含标准库和其他项目头。
- 禁止把大型实现写进 `main.cpp`。
- 禁止让 `Renderer`、`InputController` 直接实现战斗规则。

### 6.3 OOP 规则

- `Ability` 必须是基类，具体技能必须派生。
- 核心实体要有清晰职责，不为了继承而继承。
- 敌我单位不建立 `PlayerUnit` 和 `EnemyUnit` 两套类型。
- 多态用于技能和可扩展规则；普通数据优先使用结构体和组合。

PA 原文要求“核心实体（单位、技能）必须使用继承和多态实现，构建基类并派生具体英雄类”。为了兼顾可维护性，本项目采用：

- `Unit` 作为统一战斗实体，避免敌我双体系。
- `Ability` 使用继承和多态，表达英雄差异。
- 如验收明确要求“具体英雄类”，可在 `UnitTemplate` 基础上增加 `HeroFactory` 或轻量派生类，但不改变战斗系统对 `Unit` 的统一处理。

### 6.4 C++23 使用规则

优先使用：

- `enum class` 表达有限状态。
- `std::optional` 表示可能不存在的位置、装备、目标。
- `std::unique_ptr` 管理技能对象。
- `std::string_view` 传递只读字符串参数。
- `std::span` 传递连续只读数组视图。
- `std::ranges` 写清晰的过滤、查找和排序逻辑。
- `std::unordered_map` 按 `UnitId` 管理单位。
- `std::array` 表达固定大小表格，例如商店 5 格。
- `std::expected` 表达可恢复错误，例如读档失败原因。
- `[[nodiscard]]` 标记不能忽略的返回值。
- `constexpr` 保存编译期常量。
- `noexcept` 标记不会抛异常的简单查询函数。

避免：

- 裸 `new` 和裸 `delete`。
- 全局可变变量。
- 魔法数字散落在代码中。
- 大量继承层级。
- 在构造函数里做复杂 IO。

### 6.5 契约式编程

C++23 没有正式标准化的语言级 contracts，因此本项目使用轻量契约工具模拟前置条件、后置条件和不变量。

建议创建 `src/core/Contract.hpp`：

```cpp
#pragma once

#include <cassert>

#define SYNERA_EXPECTS(expr) assert((expr) && "Precondition failed: " #expr)
#define SYNERA_ENSURES(expr) assert((expr) && "Postcondition failed: " #expr)
#define SYNERA_INVARIANT(expr) assert((expr) && "Invariant failed: " #expr)
```

使用规则：

- 公共函数入口检查前置条件。
- 修改棋盘、备战区、单位位置后检查不变量。
- Debug 构建启用断言，Release 构建不依赖断言保证逻辑正确。
- 用户输入导致的失败不能只靠 `assert`，必须返回 `false` 或显示错误提示。

示例：

```cpp
bool Board::place(UnitId unitId, AxialPos pos) {
    SYNERA_EXPECTS(unitId != 0);

    if (!inBounds(pos) || !empty(pos)) {
        return false;
    }

    cells_[index(pos)] = unitId;
    SYNERA_ENSURES(occupant(pos) == unitId);
    return true;
}
```

必须维护的不变量：

- 棋盘每格最多一个单位。
- 备战区每槽最多一个单位。
- 单位不能同时在棋盘和备战区。
- `Dead` 单位不能攻击、移动、施法。
- 人口不能超过上限。
- 装备数量不能超过单位容量。

### 6.6 错误处理

- 普通游戏规则失败返回 `false`，例如金币不足、备战区满、非法拖拽。
- 存档读档使用 `try-catch` 捕获文件、格式和转换错误。
- 读档失败不能崩溃，应保留当前状态并在 UI 显示错误。
- 内部不可恢复错误在 Debug 下用契约断言尽早暴露。

### 6.7 提交规范

每完成一个可验证的小步骤都提交一次。提交前必须至少运行对应构建或测试命令，并确认 `git diff --check` 没有格式空白问题。

提交信息统一使用以下格式：

```text
<type>(<scope>): <subject>

<body>

<footer>
```

要求：

- `type` 使用 `feat`、`fix`、`refactor`、`test`、`docs`、`style`、`chore` 之一。
- `scope` 使用主要影响模块，例如 `shop`、`combat`、`board`、`ui`、`docs`。
- `subject` 使用简短英文句子，说明本次提交做了什么，不超过一行。
- `body` 写清楚主要改动、验证方式和关键设计取舍。
- `footer` 没有关联 issue 或破坏性变更时写 `Refs: none`。
- 不把无关格式化、生成文件或本地构建产物混入功能提交。

## 7. 验收清单

### 阶段一

- [ ] `M x N` 棋盘和玩家/敌方半场。
- [ ] 备战区。
- [ ] 统一 `Unit` 类型。
- [ ] `Player` 实体。
- [ ] 敌方轮次生成。
- [ ] 拖拽摆放、交换、回弹。
- [ ] GUI 展示棋盘、备战区、单位、血条、蓝条。

### 阶段二

- [ ] 准备、战斗、结算循环。
- [ ] 关卡敌人自动生成。
- [ ] 单位状态机。
- [ ] 六边形距离索敌和平局规则。
- [ ] A* 寻路、防重叠碰撞。
- [ ] 普攻、回蓝、技能多态、胜负结算。

### 阶段三

- [ ] 金币、奖励和商店。
- [ ] 购买、刷新、备战区落位。
- [ ] 人口升级和上阵限制。
- [ ] 4-6 种羁绊。
- [ ] 升星。
- [ ] 装备掉落和穿戴。
- [ ] 存档读档。
- [ ] GUI 展示经济、商店、羁绊、星级、装备、轮次和阶段。

## 8. 当前最优开发顺序

这个顺序服务于顶层设计，而不是服务 PA 文档章节。

1. 建 CMake、空窗口、`GameApp` 主循环和 `GameConfig`。
2. 建 `Types`、`Contract`、`Player`、`Unit`、`Ability`、`Board`、`Bench`、`GameState`。
3. 建空系统类和阶段调度：`RoundSystem` 调 `CombatSystem`、`ShopSystem` 等。
4. 建 `Layout`、`Renderer`、`InputController`，把状态画出来。
5. 做一条完整纵向薄片：准备、生成敌人、站桩战斗、结算、回到准备。
6. 补布阵拖拽、交换、回弹和人口限制。
7. 深化战斗：状态机、索敌、普攻、回蓝、死亡、A* 移动、防重叠。
8. 加技能多态和英雄模板。
9. 加商店、购买、刷新、金币和人口升级。
10. 加羁绊、升星、装备掉落和穿戴。
11. 加存档读档，并让读档后重算派生状态。
12. 补 README、AI 使用说明和验收演示说明。

每完成一步都提交一次，提交信息遵循 `6.7 提交规范`。示例：

```text
chore(app): scaffold raylib app shell
feat(core): add game state model
feat(combat): add playable loop slice
```

## 9. 不做的事

基础闭环稳定前先不做：

- 联机。
- 复杂动画系统。
- 大型资源管理框架。
- ECS 架构。
- 脚本系统。
- 战斗中存档。
- 复杂装备合成树。

这些内容可以作为阶段四扩展，不进入基础框架。
