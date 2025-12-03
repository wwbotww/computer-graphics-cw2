# Task 1 Summary

本文档记录 CW2 作业中 Task 1.1 与 Task 1.2 的实现细节、测试方法以及运行结果。

## Task 1.1 – Matrix/Vector Functions (`vmlib`)

### 实现内容
- **矩阵/向量乘法**：在 `vmlib/mat44.hpp` 中补全 `Mat44f operator*(Mat44f, Mat44f)` 与 `Vec4f operator*(Mat44f, Vec4f)`，使用标准双重循环计算逐元素 dot，避免中间对象。
- **旋转矩阵**：实现 `make_rotation_x/y/z(float)`，依据右手系构造 4×4 旋转矩阵，支持弧度输入。
- **仿射变换**：实现 `make_translation(Vec3f)`、`make_perspective_projection(float fov, float aspect, float near, float far)`，其中投影矩阵使用 OpenGL NDC 约定，并调用 Task 预先实现的 `make_perspective_projection` 在后续渲染中使用。
- **数值健壮性**：增加 `safe_normalize` 帮助函数，在零长度向量时退化到指定 fallback，避免摄像机或法线归一化时产生 NaN。

### 测试
- 在 `vmlib-test/` 下新增 Catch2 用例：
  - `mult.cpp`：验证矩阵与单位矩阵相乘结果、与已知矩阵乘积对比，以及矩阵作用于点/方向的差异。
  - `rotation.cpp`：检查 0° 时保持恒等，±90° 时各轴旋转结果与预期方向一致。
  - `translation.cpp`：确认平移矩阵仅影响 `w=1` 的点，对方向量无影响。
  - `projection.cpp`：保留示例测试并新增 90° FOV、1:1 视角的数值断言。
- 运行命令：
  ```bash
  ./premake5.apple gmake2
  make config=debug_x64 vmlib-test
  ./bin/vmlib-test-debug-x64-clang.exe
  ```
  输出 `All tests passed (109 assertions in 6 test cases)`。

## Task 1.2 – 3D Renderer Basics (`main`)

### 渲染管线
- **OpenGL 初始化**：在 `main/main.cpp` 使用 GLFW/GLAD 创建 4.1 Core Context，启用 SRGB、深度测试、背面剔除，并注册调试输出（Debug build）。
- **Shader 管理**：新增 `assets/cw2/terrain.vert` 与 `terrain.frag`，顶点阶段计算世界坐标与 normal matrix，片元阶段执行 ambient + diffuse（n·l）光照。`TerrainPipeline` 结构体集中保存 program 与 uniform 位置。
- **模型加载**：通过 rapidobj 解析 `assets/cw2/parlahti.obj`，在 `load_parlahti_mesh()` 中创建 interleaved VBO/VAO，同时计算 `min/max/center/radius` 供相机初始化。
- **相机/输入**：实现 `AppState` 与 `Camera`、`InputState`，利用 GLFW 回调（键盘、鼠标移动、按键、framebuffer resize）构建第一人称相机：
  - WSAD/EQ 控制移动；Shift/Ctrl 调整速度（基准 35m/s，乘以 fast/slow）。
  - 右键切换鼠标捕获，光标移动映射到 yaw/pitch（弧度），并限制 pitch ±(π/2−0.01)。
  - `update_camera()` 使用 `Secondsf` 保证帧率无关；窗口尺寸变化时自动更新 `make_perspective_projection`。

### 视觉设定
- 采用灰度环境光/漫反射（ambient `vec3(0.25)`、diffuse `vec3(0.75)`）与方向光 `(0,1,-1)`，`glClearColor` 设定为深蓝。

### 运行步骤
```bash
cd "/Users/young/Desktop/computer graphics/cw2/cw2"
./premake5.apple gmake2
make config=debug_x64 -j8         # 构建所有目标
./bin/main-debug-x64-clang.exe    # 运行第一人称相机渲染器
```
程序启动后会在终端打印：
```
RENDERER Apple M1 Pro
VENDOR Apple
VERSION 4.1 Metal - 88.1
SHADING_LANGUAGE_VERSION 4.10
```
窗口中可用 WSAD/EQ、Shift/Ctrl、右键鼠标进行导航。


