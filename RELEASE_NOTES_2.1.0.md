# 2.1.0 — Persistent motor wiring and USB installation (prerelease)

支持 ESP32-S3 N16R8 / N8R8 的独立构建。六个电机 GPIO 不再写死，使用独立 NVS 硬件配置，兼容官方 OTA 保留接线。旧 Wi-Fi 和方向配置保持原结构。升级后没有硬件配置时电机禁用，需从设备面板、安装页或 Agent 工具确认默认接线或填写自定义接线。

增加 hardware_get / hardware_set 和受限 USB JSON 管理协议，同时提供原生 USB Serial/JTAG 和 UART0 转串口通道。不开放 USB 电机运动指令。NVS 故障不再自动清空，GPIO 无效、物理内存不匹配或 PWM 初始化失败时禁用运动。

所有 OTA 路径在激活新分区前检查板型、分区和硬件配置协议描述；2.0.x 及其他板型包被拒绝，避免恢复旧引脚常量。兼容描述不是签名，不提供额外的固件真实性保证。保留大小、SHA-256 和 HTTPS 校验；尚无启动失败自动回滚。

构建脚本输出分段 USB 安装包和应用 OTA 包，不擦除 NVS、不发布或烧录硬件。独立安装页检查芯片、Flash、现有分区并回读版本和接线。工具仓库为 xlle-er/LOOI-DF26-Skill。

**发布门槛：两种模块和两类 USB 通道的真机安装、重启、默认/自定义接线、断电与 OTA 验收；Windows/Linux 的真实 USB 流程仍需实测。未达到门槛前不发布稳定版。**


## 下载与迁移

- N16R8 OTA 应用：`looi-s3.bin`；N8R8 OTA 应用：`looi-s3-n8r8.bin`。首次 USB 安装使用对应 `*-usb.json` 及其分段文件，不能将 bootloader/partitions 上传为 OTA 应用。
- Agent 工具使用 `install --release v2.1.0 --board n16r8` 或 `--board n8r8`，并按实际接线指定配置；不要默认擦除 NVS。
- 从 2.0.2 升级后，先打开设备面板核对并保存六个电机 GPIO，重启后才能运行。旧自定义接线必须重新填写。
- `looi-installer-2.1.0.zip` 是可静态托管的安装站点，含同源固件资产；下载后需通过 localhost 或 HTTPS 提供页面。此发布不代表已部署在线安装站点。
- 固件与工具已通过软件测试、双规格编译和三系统工具 CI；真实 USB、电机、供电和断电 OTA 均未完成验收，因此发布为预发布版本。稳定版 latest 保持 2.0.2。


## 预览版 OTA 开关

网页、iOS、Android 和设备面板均加入默认关闭的“预览版更新”开关；检查和安装明确选择 stable/preview 通道，保存的固件带通道标签。关闭后禁止上传缓存的预览包。预览通道由单独的 `preview` 预发布维护清单，下载仍指向具体版本，不改变稳定版 latest。

The preview OTA toggle is off by default across web, iOS, Android and the embedded panel. Cached previews require opt-in again before installation. Local signed iOS project sources are synchronized with the public iOS repository; private signing settings are unchanged.
