# LOOI DF26 Firmware

[简体中文](#简体中文) · [English](#english)

## 简体中文

ESP32-S3 底盘固件：BLE / Wi-Fi 控制、热点配网、设备面板、电机方向校准、双分区 OTA。最终硬件只有直流电机，不使用俯仰舵机和 I²C。

相关客户端：[网页 DF26](https://github.com/xlle-er/LOOI_DF26/tree/DF26) · [iOS](https://github.com/xlle-er/LOOI-DF26-IOS) · [Android](https://github.com/xlle-er/LOOI-DF26-ANDROID)。

### 硬件与首次烧录

| 功能 | GPIO |
| --- | --- |
| 左侧 IN1 / IN2 / ENA | 16 / 15 / 17 |
| 右侧 IN1 / IN2 / ENB | 7 / 6 / 5 |
| BOOT 按钮 | 0 |

ESP32 与 L298N 共地，拔掉 ENA/ENB 跳帽后使用 PWM。初次测试先架空车轮。

Arduino IDE 打开 `firmware/LooiWifiOta/LooiWifiOta.ino`，选择：

- **ESP32S3 Dev Module**；**Flash Size 16MB**；**OPI PSRAM**。
- **16M Flash (3MB APP/9.9MB FATFS)**，选项名 `app3M_fat9M_16MB`。
- 使用 USB 转串口时：Hardware CDC and JTAG，USB CDC On Boot Disabled。
- ESP32 Arduino **3.3.12**，ArduinoJson **7.4.3**，NimBLE-Arduino **2.5.1**。

Flash Size 和分区必须配套。4MB 启动配置搭配 16MB 分区会导致设备在运行程序前循环重启。首次通过 USB 安装；后续可以 OTA。项目脚本只构建，不会烧录。

### 配网与管理

无有效网络时自动开启 `LOOI-Setup-XXXXXX` 热点；平时也可长按 BOOT **3 秒后松开**。不要同时按 RST。

连接热点，打开 **http://192.168.4.1/**，用户名 **admin**，初始密码 **12345678**。密码位于 `LooiWifiOta.h` 的 `OTA_PASSWORD`，可在首次构建前修改。它是设备管理密码，不是路由器密码。

面板可扫描 2.4 GHz 网络、填写隐藏 SSID、连接和忘记网络。连接成功后保存凭据，热点保留 60 秒；连接失败保留热点，不覆盖旧配置。路由器断开后恢复热点入口。局域网通过 `_looi._tcp` 发现，设备名和稳定 ID 为 `looi-<MAC>`。

方向设置包括左右电机反向与左右交换；“前后反了”“左右转向反了”只改变对应逻辑轴。每次点动最多 **300 毫秒**。方向保存在设备中，断电、重启和普通 OTA 后仍保留。保存、接管和升级会先停车，不自动执行旧动作。

控制权由设备统一管理。需要显式接管其他客户端的控制权；租约 1 秒，客户端约每 250 毫秒续约。停止或控制权变更后，旧令牌失效。Wi-Fi 失联、租约或运动指令超时会停车。BLE 名称仍为 `LOOI Body`。

### OTA 与发布

面板、App、网页均支持本地应用 `.bin` 和在线版本。**不要上传 bootloader、partitions、merged 或其他芯片的固件**。上传前检查大小和 SHA-256，设备也验证校验值；升级期间锁定运动，完成后切换分区并重启。失败保持可重试状态，可退出升级模式。

在线清单固定为：

[looi-s3-manifest.json](https://github.com/xlle-er/LOOI-DF26-Firmware/releases/latest/download/looi-s3-manifest.json)

在线版本资产为 `looi-s3.bin`、`looi-s3-manifest.json` 和 `SHA256SUMS`。只使用本仓库公开的稳定 Release；未发布时在线检查会提示暂不可用，本地升级仍可使用。设备自行下载时必须已联网，NTP 完成同步，HTTPS 证书验证始终开启。

```sh
./scripts/build.sh
python3 tests/contracts.py
# 使用已有构建目录准备资产（不会发布）：
python3 scripts/package-release.py 2.0.0 /path/to/build-output
```

资产在根目录 `dist/`。实际发布需要维护者单独执行：

```sh
gh release create v2.0.0 dist/looi-s3.bin dist/looi-s3-manifest.json dist/SHA256SUMS \
  --repo xlle-er/LOOI-DF26-Firmware --title 'LOOI ESP32-S3 2.0.0' --notes-file RELEASE_NOTES.md
```

本实现没有固件签名和启动失败自动回滚。请保留 USB 恢复途径。构建、主机模拟和客户端模拟测试不代表已通过真实电机、供电、Wi-Fi/BLE 共存和断电恢复验收。

接口与指令见 [协议说明](protocol/README.md) 和 [指令清单](protocol/commands.json)。不要在仓库中保存个人 Wi-Fi 或服务密钥。

## English

ESP32-S3 chassis firmware with BLE/Wi-Fi control, hotspot provisioning, a built-in panel, persistent motor calibration and dual-slot OTA. The final hardware uses DC motors only: there is no pitch servo or I²C.

Clients: [Web DF26](https://github.com/xlle-er/LOOI_DF26/tree/DF26), [iOS](https://github.com/xlle-er/LOOI-DF26-IOS), [Android](https://github.com/xlle-er/LOOI-DF26-ANDROID).

Use the GPIO table above. Select **ESP32S3 Dev Module, 16MB Flash, OPI PSRAM, app3M_fat9M_16MB partitions**, with Arduino ESP32 3.3.12, ArduinoJson 7.4.3 and NimBLE-Arduino 2.5.1. Incorrect Flash Size/partition combinations prevent boot. Install over USB once; build scripts never flash hardware.

Join `LOOI-Setup-XXXXXX`, open **http://192.168.4.1/**, and sign in as **admin / 12345678**. Change `OTA_PASSWORD` before building if needed. The management password is separate from router credentials. The hotspot starts automatically when unconfigured or offline; holding BOOT for three seconds also opens it. Configure a 2.4 GHz network, including hidden SSIDs. New credentials are persisted only after a successful connection. The hotspot stays available for 60 seconds after connecting. LAN discovery uses `_looi._tcp` and a stable `looi-<MAC>` ID.

Calibrate motor inversion and side mapping from any client. Shortcuts independently reverse forward/back or steering. Each motor test lasts at most 300 ms. Settings survive restarts and normal OTA. Changing calibration, taking control and installing firmware stop the motors and invalidate old motion. A one-second control lease is renewed about every 250 ms. Loss of connectivity or command expiry stops motion.

OTA accepts only ESP32-S3 application `.bin` images, validates size and SHA-256, locks motion and activates the new partition only after verification. Keep power connected. Online updates read the manifest link above from this repository's latest stable Release. Use the build and packaging commands above to prepare `dist/` assets; publishing is a separate maintainer action. Before a release is published, online checks report unavailable and local-file OTA remains usable.

This version does not implement signed firmware or automatic rollback after a failed new application boot. Keep USB recovery available. Host tests and successful builds do not replace physical motor, power, wireless coexistence and interrupted-update testing. See [protocol documentation](protocol/README.md) for API details.
