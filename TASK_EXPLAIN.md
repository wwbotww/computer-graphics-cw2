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


## Task 1.3 – Texturing

### 渲染更新
- **纹理坐标管线**：把 `VertexPN` 扩展为 `VertexPNT` 并从 `parlahti.obj` 读取 `vt`，在 VAO 中新增 `layout(location=2)`，顶点着色器输出 `vTexCoord`。
- **正射影像采样**：使用 `stb_image` 将 `assets/cw2/L4343A-4k.jpeg` 以 `GL_SRGB8_ALPHA8` 上传，生成 mipmap，渲染时绑定到 `GL_TEXTURE0` 并通过 `uTerrainTexture` 采样。
- **sRGB 输出与亮度**：初始化阶段开启 `glEnable(GL_FRAMEBUFFER_SRGB)`，确保“线性光照×纹理”再写入默认帧缓冲时执行 gamma 纠正，画面与文档一致。

### 着色器片段
```
vec3 lighting = uAmbientColor + uDiffuseColor * ndotl;
vec3 albedo = texture(uTerrainTexture, vTexCoord).rgb;
FragColor = vec4(albedo * lighting, 1.0);
```

### 运行
与 Task 1.2 相同：构建 `debug_x64`，执行 `./bin/main-debug-x64-clang.exe`，即可看到套用正射影像的地形，并且边缘无异常光带。


## Task 1.4 – Simple Instancing

### 目标
加载 `landingpad.obj`（含多种 MTL 材质），选取海面上相距较远的两个位置，在不复制顶点数据的前提下渲染两次着陆平台，平台需紧贴海平面但不没入水面。

### 实现
- **几何与材质**：新增 `VertexPNC`（位置/法线/颜色）与 `LandingPadGeometry`，使用 rapidobj 读取材质 `Kd` 作为顶点颜色。`landingpad.vert/frag` 负责处理 per-vertex color 并复用 Task 1.2 的环境+漫反射光照。
- **实例化绘制**：将模型加载一次，构建两个模型矩阵（先 `make_scaling(25)` 再平移）。着陆点选择 `(-100, waterLevel, 84)` 与 `(92.75, waterLevel, -22.625)`，均落在 `parlahti` 网格最低高度（海平面约 `-1m`）上，且距离超过 200m，满足“在海面且彼此不邻近”的要求。渲染循环中遍历 `landingPadModels`，对相同 VAO/VBO 调用两次 `glDrawArrays`，符合“不复制数据”的要求。
- **统一光照参数**：`LandingPadPipeline` 复用主场景的 `uLightDir/uAmbient/uDiffuse`，避免额外调参；由于模型无纹理，片元着色器直接使用 `vColor` 乘以光照结果。

### 结果
运行 `./bin/main-debug-x64-clang.exe` 后，可在海面上看到两座尺寸放大的着陆平台（一个靠近西北海域、一个位于东南海域），均以相同顶点数据绘制两遍，且材质颜色遵循 MTL 配置。


