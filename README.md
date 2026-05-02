# Galgame Creator

一个基于 Qt6/C++ 开发的可视化 Galgame编辑器与播放器。

本项目是我的课程作业项目：
- 学校：南开大学
- 入学年份：2025 级
- 课程：高级语言程序设计（大作业）

## 项目简介

`Galgame Creator` 提供从“项目编辑 -> 时间轴编排 -> 预览测试 -> 导出运行”的完整流程：
- 编辑器（`galgame_creator`）：管理角色、背景、对白、BGM、样式与时间轴。
- 播放器（`galplayer`）：独立加载导出的游戏包并运行。

项目目标是实现一个可落地、可演示、可导出的轻量级视觉小说制作工具。

## 主要功能

- 项目管理：新建项目、打开项目、保存项目。
- 资源编辑：角色、背景、背景音乐轨道、对话文本与语音。
- 时间轴编辑：对话时间轴可视化、选择预览、拖拽重排顺序。
- 批量生成：支持批量生成对话、按行导入台词、自动匹配语音文件名。
- 可视化预览：中间画布实时预览当前对话、角色立绘与背景切换。
- 样式设置：开始界面样式、对话框颜色、姓名/文本字号与颜色设置。
- 测试运行：一键临时导出并启动内置播放器进行联调。
- 游戏导出：支持导出为 `.galgame` 包或目录（调试模式）。

## 技术栈

- 语言：C++（面向对象）
- UI 框架：Qt 6（Core / Widgets / Multimedia）
- 构建系统：CMake（>= 3.19）
- 目标程序：
  - `galgame_creator`：编辑器
  - `galplayer`：运行时播放器

## 运行环境

- Windows（已重点适配）
- Qt 6.x
- CMake 3.19+
- 可用 C++17 编译器（MSVC / MinGW / Clang 均可）

## 编译与运行

以下示例以 Windows + CMake 命令行为例：

```bash
cmake -S . -B build
cmake --build build --config Release
```

编译后可执行文件通常位于：
- `build/Release/galgame_creator.exe`
- `build/Release/galplayer.exe`

运行编辑器：

```bash
build/Release/galgame_creator.exe
```

运行播放器（可直接指定已导出目录或 `game.json` 所在目录）：

```bash
build/Release/galplayer.exe <游戏目录>
```

## 使用流程（推荐）

1. 启动编辑器，选择“新建项目”或“打开项目”。
2. 添加角色、背景、背景音乐轨道和对话内容。
3. 在“对话时间轴”中调整顺序并逐条预览效果。
4. 使用“测试运行”快速验证剧情与资源是否正常。
5. 使用“导出游戏”生成 `.galgame` 包或调试目录。
6. 用 `galplayer` 运行导出结果。

## 项目结构

```text
galgame_creator/
├─ CMakeLists.txt
├─ src/
│  ├─ main.cpp                 # 编辑器入口
│  ├─ mainwindow.*             # 编辑器主界面与交互逻辑
│  ├─ editors/                 # 角色/对话编辑对话框
│  ├─ io/                      # 导出器与导出流程
│  ├─ models/                  # 项目数据模型（角色/背景/对话/BGM）
│  ├─ view/                    # 画布与打字机效果
│  └─ player/                  # 播放器入口与运行时窗口
```

## 导出结果说明

导出后核心内容包括：
- `game.json`：脚本与资源索引
- `assets/`：角色立绘、背景、语音、BGM 等资源
- `galplayer(.exe)`：运行时播放器
- 启动脚本（Windows 下含 `启动游戏.vbs` / `启动游戏.bat`）

## 课程作业说明

本项目用于“高级语言程序设计”课程大作业，重点体现：
- 面向对象建模能力（Project / Character / Dialogue / Background / BgmTrack）
- 图形界面开发能力（Qt Widgets）
- 工程化能力（CMake、多模块组织、导出链路）
- 综合实现能力（编辑器 + 运行时播放器的一体化设计）

## 后续可改进方向

- 分支剧情与选项跳转系统
- 立绘表情/动作切换
- 更完善的撤销/重做机制
- 自动化测试与 CI
- 多语言本地化支持


