# Windows 系统构建指南

## 前提条件

1. **CMake** (3.15 或更高版本)
2. **MuJoCo 库** (已安装到 `C:/Users/29115/.mujoco/mujoco210` 或自定义路径)
3. **Visual Studio** 或 **MinGW** (用于编译)
4. **Windows SDK** (已包含在 Visual Studio 中)

## 快速开始

### 方法1: 使用默认 MuJoCo 路径
如果你的 MuJoCo 安装在默认位置 (`C:/Users/29115/.mujoco/mujoco210`):

```powershell
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake ..

# 构建项目
cmake --build . --config Release
```

### 方法2: 指定自定义 MuJoCo 路径
如果你的 MuJoCo 安装在其他位置:

```powershell
mkdir build
cd build
cmake .. -DMUJOCO_PATH="你的/MuJoCo/路径"
cmake --build . --config Release
```

### 方法3: 使用环境变量
设置环境变量 `MUJOCO_PATH`:

```powershell
$env:MUJOCO_PATH = "你的/MuJoCo/路径"
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## 输出文件

构建成功后，可执行文件将位于:
- `build/bin/Release/mujocomissile.exe` (Release 版本)
- `build/bin/Debug/mujocomissile.exe` (Debug 版本)

## 配置选项

CMakeLists.txt 提供了以下配置选项:

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_SIMULATE` | ON | 是否构建 simulate 可执行文件 |
| `FIND_GLFW` | OFF | 是否查找并链接 GLFW 库 |
| `MUJOCO_USE_FINDPACKAGE` | OFF | 是否使用 find_package 查找 MuJoCo |

### 使用配置选项示例:

```powershell
# 禁用 simulate 构建，启用 GLFW 查找
cmake .. -DBUILD_SIMULATE=OFF -DFIND_GLFW=ON

# 使用 find_package 查找 MuJoCo
cmake .. -DMUJOCO_USE_FINDPACKAGE=ON
```

## 项目结构

```
mujocomissile/
├── CMakeLists.txt          # CMake 配置文件 (新创建)
├── BUILD_WINDOWS.md        # 构建说明 (新创建)
├── src/
│   └── main.cpp           # 主源文件 (如果不存在会自动创建占位符)
├── simulate/              # 模拟器源代码目录 (可选)
├── data/                  # 数据文件
├── model/                 # 模型文件
└── doc/                   # 文档
```

## 常见问题

### 1. 找不到 MuJoCo 库
**错误信息**: `MuJoCo library not found in ...`

**解决方案**:
- 确认 MuJoCo 已正确安装
- 使用 `-DMUJOCO_PATH` 指定正确路径
- 检查路径中是否包含 `lib/mujoco.lib` 文件

### 2. 缺少 Windows SDK
**错误信息**: `Could not find SDK`

**解决方案**:
- 安装 Visual Studio 并包含 Windows SDK
- 或手动安装 Windows SDK

### 3. 编译错误
**解决方案**:
- 确保使用正确的编译器 (Visual Studio 或 MinGW)
- 检查 MuJoCo 头文件路径是否正确

### 4. 运行时缺少 DLL
**错误信息**: `无法找到 mujoco.dll`

**解决方案**:
- 确保 `mujoco.dll` 在可执行文件同一目录
- 或将 MuJoCo 的 `bin` 目录添加到系统 PATH

## 高级配置

### 自定义编译器
```powershell
# 使用 MinGW
cmake .. -G "MinGW Makefiles"

# 使用 Ninja
cmake .. -G "Ninja"
```

### 多配置构建
```powershell
# Debug 版本 (包含调试信息)
cmake --build . --config Debug

# Release 版本 (优化)
cmake --build . --config Release

# RelWithDebInfo 版本 (优化但包含调试信息)
cmake --build . --config RelWithDebInfo
```

### 清理构建
```powershell
# 清理构建文件但保留配置
cmake --build . --target clean

# 完全重新构建
Remove-Item -Recurse -Force build
mkdir build
cd build
cmake ..
```

## 注意事项

1. **路径格式**: Windows 路径可以使用正斜杠 (`/`) 或反斜杠 (`\`)，但在 CMake 中建议使用正斜杠
2. **权限**: 确保有权限在安装目录中写入文件
3. **防病毒软件**: 某些防病毒软件可能会阻止构建过程，必要时添加例外

## 获取帮助

如果遇到问题:
1. 检查 MuJoCo 是否正确安装
2. 验证路径和权限
3. 查看 CMake 输出中的错误信息
4. 参考 `doc/相关教程/` 中的文档
