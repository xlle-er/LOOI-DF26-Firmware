# LOOI DF26 Firmware

> **2.1.0 预发布版**：新增持久化接线、N16R8/N8R8 和 USB 一键部署。首次升级后必须确认接线，否则电机保持禁用。兼容官方 OTA 保留接线；旧 2.0.x OTA 包将被拒绝。下面 2.0.2 说明保留作稳定版参考，2.1.0 的构建和安装以本节为准。

2.1.0 下载：[预发布版本](https://github.com/xlle-er/LOOI-DF26-Firmware/releases/tag/v2.1.0)。尚未完成真机验收，不替换稳定版推荐。Agent 安装时显式添加 `--release v2.1.0`。

使用 [LOOI-DF26-Skill](https://github.com/xlle-er/LOOI-DF26-Skill) 完成 Agent 部署。网页安装源码位于 `installer/`，本次尚未部署上线。`npm ci && npm run build` 生成 `installer/dist`，供后续静态托管；预定入口为 https://xlle-er.github.io/LOOI-DF26-Firmware/。

```sh
# 两种板型分别构建，输出 dist/ 中的 USB 分段清单和 OTA 应用包；不会烧录或发布
./scripts/build.sh
# 也可 --board n16r8 或 --board n8r8
python3 tests/contracts.py
```

工具链：Arduino ESP32 3.3.12、ArduinoJson 7.4.3、NimBLE-Arduino 2.5.1。2.1.0 的 USB 配置通道需要 USBMode=hwcdc、CDCOnBoot=cdc，同时启用 UART0；多规格构建由 scripts/build.py 固定参数。GPIO 定义保存在独立 NVS blob 中，不通过源码构建切换。首次配置前断开电机电源；未配置时 GPIO 不输出，驱动输入若无下拉需用适当的使能下拉保证电气状态。

安装包使用 Arduino 生成的 flash_args 提取地址，保留 NVS；N8R8 采用双 3MB 应用槽，NVS/otadata/app 地址与 N16R8 相同。新版本与旧标准接线迁移在设备面板中确认一次；旧自定义接线须重新填写。真实 USB、电机、供电和断电验收状态见 VERIFICATION.md。


> **请使用 2.0.2 或更新版本。2.0.0 / 2.0.1 存在启动循环重启问题，已撤回推荐。若设备已无法启动，请通过 USB 恢复应用固件，不要擦除 NVS 或整片 Flash。**
>
> **Use 2.0.2 or newer. Versions 2.0.0 / 2.0.1 have a startup boot-loop defect and are withdrawn from recommendation. Recover affected devices over USB while preserving NVS and the partition table.**

[简体中文](#简体中文) · [English](#english)

## 简体中文

ESP32-S3 底盘固件：BLE / Wi-Fi 控制、热点配网、设备面板、电机方向校准、双分区 OTA。

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
python3 scripts/package-release.py 2.0.2 /path/to/build-output
```

资产在根目录 `dist/`。实际发布需要维护者单独执行：

```sh
gh release create v2.0.2 dist/looi-s3.bin dist/looi-s3-manifest.json dist/SHA256SUMS \
  --repo xlle-er/LOOI-DF26-Firmware --title 'LOOI ESP32-S3 2.0.2' --notes-file RELEASE_NOTES_2.0.2.md
```

本实现没有固件签名和启动失败自动回滚。请保留 USB 恢复途径。构建、主机模拟和客户端模拟测试不代表已通过真实电机、供电、Wi-Fi/BLE 共存和断电恢复验收。

接口与指令见 [协议说明](protocol/README.md) 和 [指令清单](protocol/commands.json)。不要在仓库中保存个人 Wi-Fi 或服务密钥。

## English

ESP32-S3 chassis firmware with BLE/Wi-Fi control, hotspot provisioning, a built-in panel, persistent motor calibration and dual-slot OTA. 

Clients: [Web DF26](https://github.com/xlle-er/LOOI_DF26/tree/DF26), [iOS](https://github.com/xlle-er/LOOI-DF26-IOS), [Android](https://github.com/xlle-er/LOOI-DF26-ANDROID).

Use the GPIO table above. Select **ESP32S3 Dev Module, 16MB Flash, OPI PSRAM, app3M_fat9M_16MB partitions**, with Arduino ESP32 3.3.12, ArduinoJson 7.4.3 and NimBLE-Arduino 2.5.1. Incorrect Flash Size/partition combinations prevent boot. Install over USB once; build scripts never flash hardware.

Join `LOOI-Setup-XXXXXX`, open **http://192.168.4.1/**, and sign in as **admin / 12345678**. Change `OTA_PASSWORD` before building if needed. The management password is separate from router credentials. The hotspot starts automatically when unconfigured or offline; holding BOOT for three seconds also opens it. Configure a 2.4 GHz network, including hidden SSIDs. New credentials are persisted only after a successful connection. The hotspot stays available for 60 seconds after connecting. LAN discovery uses `_looi._tcp` and a stable `looi-<MAC>` ID.

Calibrate motor inversion and side mapping from any client. Shortcuts independently reverse forward/back or steering. Each motor test lasts at most 300 ms. Settings survive restarts and normal OTA. Changing calibration, taking control and installing firmware stop the motors and invalidate old motion. A one-second control lease is renewed about every 250 ms. Loss of connectivity or command expiry stops motion.

OTA accepts only ESP32-S3 application `.bin` images, validates size and SHA-256, locks motion and activates the new partition only after verification. Keep power connected. Online updates read the manifest link above from this repository's latest stable Release. Use the build and packaging commands above to prepare `dist/` assets; publishing is a separate maintainer action. Before a release is published, online checks report unavailable and local-file OTA remains usable.

This version does not implement signed firmware or automatic rollback after a failed new application boot. Keep USB recovery available. Host tests and successful builds do not replace physical motor, power, wireless coexistence and interrupted-update testing. See [protocol documentation](protocol/README.md) for API details.

## 热点升级与联网恢复 / Hotspot update and internet recovery

先在有互联网时检查版本、下载并校验固件，再连接 ESP 热点上传。网页使用本机浏览器存储，App 使用自己的文件存储；已下载文件可在页面或 App 重启后继续使用。重新上传前再次校验，失败下载不会替换上一个完整包。

“升级后关闭 ESP 热点”默认开启，需要设备固件 2.0.2 或更新版本支持。只有成功校验并切换到 OTA 目标分区后，下次启动才跳过自动开启热点；不会删除网络或方向设置，BLE 仍可用，长按 BOOT 3 秒可重新打开热点。该设置只影响升级后的这次启动，普通断电重启仍按原配网策略工作。关闭设备热点不保证手机自动恢复互联网，启动 AI 陪伴前应检查手机流量或可上网的 Wi-Fi。

从仅热点 OTA 初版或 2.0.0 首次升级时，旧固件无法保存这个选项：先下载 `.bin`，再通过 `http://192.168.4.1/` 的设备面板上传，完成后手动断开热点。App/网页管理接口不保证兼容仅热点 OTA 初版。2.0.1 的关闭开关对安装该版本之后发起的 OTA 生效。

Download and verify firmware while online, then join the ESP hotspot to upload. Prepared packages survive page/app restarts and are verified again before upload. Failed downloads preserve the last complete package.

Close ESP hotspot after update is enabled by default and requires device firmware 2.0.2+. After a verified OTA activates its target partition, the next boot skips automatic hotspot startup. Network and calibration settings remain intact; BLE stays available and holding BOOT for three seconds reopens the hotspot. This is a one-boot choice; ordinary subsequent power cycles use normal provisioning behavior. Your phone decides whether internet reconnects automatically. Restore mobile data or internet Wi-Fi before AI companionship.

The initial hotspot-only firmware and 2.0.0 cannot store this option. For the first upgrade, download the application `.bin`, join the hotspot and upload it through the device's own panel at `http://192.168.4.1/`, then leave the hotspot manually. Modern management APIs are not guaranteed on the initial firmware. The 2.0.1 option applies to OTA operations initiated after that firmware is installed.

### 安装页构建与发布准备（2.1.0）

GitHub Release 下载地址不提供完整的浏览器跨域读取支持，安装页因此将校验过的 USB 文件与页面同源托管，Agent 工具继续直接读取官方 Release。`scripts/build.py` 会生成并暂存安装页资源；只构建 UI 时不会包含可安装固件。

```sh
# 已完成本地固件构建，或已下载某个通过真机验收的 Release 资产：
python3 scripts/prepare-installer.py --assets dist --require-all
cd installer
npm ci
npm test
npm run build
# installer/dist 为完整静态站点，包括两个板型的 USB 资源。
```

`Prepare installer site` workflow 仅手动准备站点 artifact，不部署。输入已验收的 release_tag 时下载并校验该 Release 的两套资源；留空只构建 UI。正式上线需单独配置 Pages/静态托管并发布对应 Release；本次不执行。


### 预览版通道维护

客户端和设备面板的“预览版更新”默认关闭；开启后读取 `releases/download/preview/` 的对应板型清单。每次已校验的版本正式公开为 prerelease 后运行 `python3 scripts/prepare-preview-channel.py`，将生成的 `dist/preview/*-manifest.json` 上传到独立、标记为 prerelease 的 `preview` Release。只更新清单指针，应用 URL 保持指向不可变版本标签；不要将 preview 设为 latest。发布版本和更新 preview 指针是两个独立步骤。
