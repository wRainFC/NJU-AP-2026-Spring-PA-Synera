# Synera: Synergy Auto-Arena

Synera 是一个使用 C++23 和 Raylib 实现的单机轻量级自走棋游戏。玩家在准备阶段购买单位、调整站位、搭配羁绊和装备；战斗阶段由单位自动寻路、攻击和释放技能；结算阶段根据胜负发放金币、掉落装备并推进关卡。

## 基本信息

- 姓名：xxx
- 学号：xxx
- 项目阶段完成度：
  - 阶段一：已完成
  - 阶段二：已完成
  - 阶段三：已完成
  - 阶段四：已完成一部分 UI/资源扩展基础，包括可缩放窗口、贴图 fallback 和单位 sprite-sheet 动画支持

## 构建与运行

环境要求：

- CMake 3.28+
- 支持 C++23 的编译器
- Raylib 5.5
- cereal 1.3.2
- Catch2 3

如果本机没有安装 Raylib、cereal 或 Catch2，CMake 会通过 `FetchContent` 拉取依赖。

```bash
cmake -S . -B build
cmake --build build --target synera
./build/synera
```

运行测试：

```bash
cmake --build build --target synera_tests
ctest --test-dir build --output-on-failure
```

## 操作说明

- 鼠标拖拽单位：在备战区和玩家半场之间调整站位。
- 拖拽装备：从装备栏拖到己方单位身上穿戴。
- 点击商店单位：购买单位并放入备战区。
- Refresh：手动刷新商店。
- Lock：锁定/解锁商店。
- Level Up：花费金币提升人口上限。
- Start Combat：开始当前轮战斗。
- Save / Load：保存或读取 `saves/manual.json`。保存仅允许在准备阶段进行。
- 将己方单位拖到 Sell 区域：出售单位并获得金币。

## 文件结构

```text
Synera/
  CMakeLists.txt
  README.md
  PLAN.md
  PA说明文档.md
  Raylib_Cpp23_设计文档.md
  UIdesign.md
  assets/
    textures/
  src/
    main.cpp
    app/        应用入口、主循环、全局配置
    board/      六边形棋盘、备战区、寻路
    core/       核心数据模型、单位、玩家、技能、商店、元数据
    systems/    战斗、回合、商店、羁绊、升星、装备、存档系统
    ui/         Raylib 窗口、输入投影、布局、渲染与贴图资源
  tests/        Catch2 单元测试
```

## 核心类与数据结构

- `GameApp`：应用主循环，负责调度输入、规则系统更新和渲染。
- `GameWindow`：封装 Raylib 窗口、虚拟画布和鼠标坐标投影，支持窗口缩放后的稳定命中测试。
- `GameState`：游戏全局状态，持有玩家、棋盘、备战区、商店、装备池和全部单位。
- `Unit`：统一的战斗实体，通过 `Owner` 区分我方和敌方，通过 `traits` 参与羁绊计算。
- `Ability`：技能多态基类，当前实现了直线伤害、范围治疗、眩晕攻击和空技能。
- `Board` / `Bench`：分别维护主棋盘和备战区的占用关系。
- `Pathfinder`：在六边形棋盘上寻找接近目标攻击范围的路径。
- `CombatSystem`：驱动单位状态机、索敌、移动、普攻、回蓝和施法。
- `RoundSystem`：负责 Prep / Combat / Resolve 阶段切换、敌人生成、胜负结算和轮次推进。
- `ShopSystem` / `ShopPool`：负责商店刷新、锁定、购买和出售。
- `SynergySystem`：统计场上羁绊并应用属性或机制效果。
- `UpgradeSystem`：实现同名同星单位三合一升星。
- `EquipmentSystem`：实现装备掉落、装备池和穿戴限制。
- `SaveSystem`：使用 cereal 将游戏状态序列化为 JSON 并读回。
- `Renderer` / `RenderAssets`：负责 UI 渲染、贴图加载、sprite-sheet 动画和几何 fallback。

## 关键算法与实现说明

### 六边形棋盘

项目采用 pointy-top 六边形显示，并在规则层使用 Axial 坐标。前端显示和数组存储使用 odd-r offset 坐标转换。这样可以让战斗距离、邻居遍历和寻路逻辑都基于稳定的六边形数学模型。

### 寻路与阻挡

`Pathfinder::findPathToAttackRange` 使用带启发式优先级的搜索方式，从当前单位位置寻找一个位于目标攻击范围内的空格。路径搜索会跳过被占用格和目标所在格，避免单位穿过或重叠。

### 目标锁定

`CombatSystem::acquireTarget` 在所有存活敌方单位中选择目标。当前实现基于六边形距离排序，并用生命值、列坐标、行坐标和 ID 作为平局规则，保证索敌结果稳定可复现。

### 战斗状态机

单位状态包括：

- `Idle`
- `Moving`
- `Attacking`
- `Casting`
- `Stunned`
- `Dead`

战斗更新时，单位会先处理眩晕和死亡，再寻找目标；目标不在攻击范围内则移动，目标在范围内则根据法力值决定施法或普攻。

### 技能多态

技能统一继承自 `Ability`，通过虚函数 `cast(Unit&, AbilityContext&)` 实现不同效果。当前英雄技能包括：

- Fire Line：沿六边形方向造成直线伤害。
- Healing Aura：治疗附近友军。
- Stun Strike：对当前目标造成伤害并短暂眩晕。
- Noop：训练假人和 fallback 单位使用的空技能。

### 羁绊计算

`SynergySystem` 只统计玩家场上单位的 traits。当前实现的有效羁绊包括：

- Warrior：达到阈值后战士增加攻击力。
- Guardian：达到阈值后玩家场上单位增加生命值。
- Mystic：达到阈值后玩家场上单位降低最大法力需求。
- Ranger：达到阈值后游侠普攻造成两段伤害。

`Mage` 和 `Assassin` 目前作为预留标签展示在 UI 中，方便后续扩展。

### 升星与装备

`UpgradeSystem` 在玩家购买或获得单位后检查同名同星单位，满足 3 个时保留最近获得的单位并移除另外两个，星级提升后重新计算属性。

装备由 `EquipmentSystem` 管理，击败敌人后有概率掉落到装备池。每个单位最多穿戴一件装备。当前基础装备包括：

- Sword：攻击力 +15
- Vest：生命值 +150
- Glove：攻击间隔乘以 0.8
- Crystal：最大法力值 -30，最低不低于 20

### 存档读档

`SaveSystem` 使用 cereal JSON archive 保存和恢复当前游戏状态。保存内容包括玩家状态、阶段、商店、装备池、棋盘/备战区位置、单位状态、星级和装备。保存只允许在准备阶段执行，读档失败会返回错误信息并保留当前状态。

## GUI 与资源

UI 使用 Raylib 绘制。窗口由 `GameWindow` 管理，内部固定虚拟画布为 `1600 x 900`，默认窗口尺寸与虚拟画布一致；真实窗口变化时通过 letterbox 缩放显示，并将鼠标坐标投影回虚拟画布。

贴图资源是可选的。没有 PNG 资源时，游戏会 fallback 到几何图形和文字，仍然可以完整运行。

资源路径约定：

```text
assets/
  fonts/ui.ttf
  fonts/ui.otf
  textures/
    board/player_hex.png
    board/enemy_hex.png
    equipment/iron_sword.png
    equipment/chain_vest.png
    equipment/swift_glove.png
    equipment/mana_crystal.png
    ui/button.png
    ui/button_disabled.png
    ui/trait_active.png
    ui/trait_inactive.png
    ui/sell_area.png
    ui/panel.png
    units/<template_id>.png
    units/player_default.png
    units/enemy_default.png
    units/<template_id>/player_idle.png
    units/<template_id>/enemy_idle.png
```

字体会优先加载 `assets/fonts/ui.ttf`，不存在时尝试 `assets/fonts/ui.otf`，仍不存在则使用 Raylib 默认字体。当前随项目提供的 `ui.ttf` 为 Andrew Tyler 制作的 Minecraftia 字体。单位动画支持单行 sprite sheet，帧宽等于图片高度，帧数为 `image_width / image_height`。

## 当前验收状态

阶段一至阶段三的基础验收点已实现：

- 棋盘、备战区、统一 Unit 模型、拖拽摆放、GUI 展示。
- 三阶段循环、敌人生成、战斗状态机、寻路、普攻、回蓝、技能、胜负结算。
- 金币、商店、刷新、购买、出售、人口、羁绊、升星、装备、存档读档和完整 UI 信息展示。

当前自动化测试覆盖：

- 棋盘/备战区占用与交换
- 六边形寻路
- 单位创建与 fallback
- 放置规则与人口限制
- 战斗、技能、眩晕、死亡清理、索敌平局规则
- 回合敌人生成与结算
- 商店刷新、锁定、购买、出售
- 羁绊、升星、装备
- 存档读档与错误处理

## 已知说明

- 规则层采用六边形距离作为战斗距离。PA 文档中提到“欧氏距离”，本项目基于六边形棋盘进行了等价的棋盘距离设计。
- `Mage` 和 `Assassin` 是预留羁绊标签，目前不提供实际效果；当前实际生效羁绊数量为 4 个，满足 PA 对 4-6 个羁绊的最低要求。

## AI 使用说明

本项目开发过程中使用 AI 辅助进行需求拆解、架构规划、接口设计、代码审查和局部实现建议。所有生成内容均经过人工筛选、修改、编译和测试验证。

两个主要 AI 辅助模块如下：

1. UI 架构拆分

   AI 辅助分析了原始 `Renderer` 过重的问题，并提出将窗口管理、输入投影、资源加载、通用绘制、棋盘绘制和单位绘制拆成独立模块。最终实现为 `GameWindow`、`UiState`、`UiDrawing`、`GridItem`、`UnitItem` 和 `RenderAssets`。这些模块仍保持 UI 边界，不直接实现战斗、商店或羁绊规则。

2. 存档读档设计

   AI 辅助设计了基于 cereal 的 JSON 存档结构和错误处理路径。最终实现中，`SaveSystem` 将运行时对象转换为版本化数据结构，再恢复为 `GameState`。读档过程会校验枚举、位置和放置结果，失败时返回 `std::expected` 错误而不是崩溃。

