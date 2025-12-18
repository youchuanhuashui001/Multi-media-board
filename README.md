# GitHub 开发流程与代码管理指南

本文档用于规范基于 ARM 开发板软件项目的代码管理流程，涵盖代码拉取（含子模块）及功能开发的完整闭环。

## 1. 获取代码 (Pulling Code)

由于嵌入式项目（如 U-Boot、Linux Kernel 驱动、SDK）常引用外部仓库，正确处理 **Submodules (子模块)** 至关重要。

### 情况 A：首次克隆仓库

如果是一个新项目，必须使用递归克隆指令，以确保所有子模块的内容都能被下载。

```
# 核心指令：--recurse-submodules
git clone --recurse-submodules <repository_url>

```

### 情况 B：已克隆仓库，补全子模块

如果你已经 clone 了仓库，但发现子模块文件夹是空的，或者需要更新子模块配置：

```
cd <repository_directory>

# 初始化并递归更新子模块
git submodule update --init --recursive

```

## 2. 开发工作流 (Development Workflow)

**核心原则**：

1. 永远不要直接在 `main` (或 `master`) 分支上修改代码。

2. 每个功能或修复都应当在独立的 **Feature Branch** 上进行。

3. 只有代码稳定且经过测试后，才通过 **Pull Request (PR)** 合并回主分支。


### 步骤 1：同步主分支 (Sync Main)

在开始任何新工作之前，确保你的本地 `main` 分支是最新的。

```
# 1. 切换回主分支
git checkout main

# 2. 拉取远程最新的代码
git pull origin main

```

### 步骤 2：创建功能分支 (Create Branch)

基于最新的 `main` 创建一个新分支。

- 命名建议：`feat/功能名` (功能开发) 或 `fix/问题名` (修复 Bug)。


```
# 创建并切换到新分支 (例如开发 readme 文档)
git checkout -b feat/readme

# 注意：虽然 git br (branch) 可以创建分支，但 checkout -b 能一步完成创建并切换

```

### 步骤 3：开发与提交 (Develop & Commit)

在本地进行代码编写和硬件验证。

```
# 查看文件状态
git status

# 添加修改的文件 (推荐添加具体文件，而非无脑 add .)
git add README.md src/main.c

# 提交更改
# 建议格式：type: description (例如 "docs: update development guide")
git commit -m "docs: add detailed github workflow instructions"

```

> **提示**：如果开发周期较长，主分支有了更新，你可以中途执行 `git pull origin main` 将主分支的更新合并到你当前的分支，以减少最终合并时的冲突。

### 步骤 4：推送到远程 (Push)

代码初步稳定后，将其推送到 GitHub。

```
# 第一次推送该分支时，需要建立追踪关系 (-u)
git push -u origin feat/readme

# 后续推送只需执行：
git push

```

## 3. 合并代码 (Pull Request & Merge)

代码推送到 GitHub 后，合并流程转至网页端操作。

1. **发起 PR**：

    - 登录 GitHub 仓库页面。

    - 点击黄色的 **"Compare & pull request"** 提示框。

    - 填写标题和描述（说明在哪个 ARM 板子上测试过，结果如何）。

2. **代码审查 (Review)**：

    - 等待同事 Review 或自行检查代码 diff。

    - 如果有修改意见，在**本地修改** -> **git commit** -> **git push**，PR 会自动更新。

3. **合并 (Merge)**：

    - 审查通过后，点击 **"Squash and merge"** (推荐) 或 "Create a merge commit"。

    - _Squash and merge_ 会将你开发过程中的多个啰嗦 commit 合并为一个整洁的 commit 存入主分支。


## 4. 收尾工作 (Cleanup)

功能合并成功后，本地环境需要清理并回到起点。

```
# 1. 切回主分支
git checkout main

# 2. 再次拉取最新代码 (此时主分支已包含你刚才合并的功能)
git pull origin main

# 3. 删除本地已完成的功能分支
git branch -d feat/readme

# 4. 删除 GitHub 上的远程分支 (防止远程仓库分支堆积)
git push origin --delete feat/readme

```

## 5. 常用命令速查表

|**场景**|**命令**|
|---|---|
|**查看分支**|`git branch -a`|
|**查看状态**|`git status`|
|**丢弃本地修改**|`git checkout -- <file>`|
|**暂存更改(不提交)**|`git stash` (之后用 `git stash pop` 恢复)|
|**查看提交历史**|`git log --oneline --graph`|
|**删除远程分支**|`git push origin --delete <branch_name>`|