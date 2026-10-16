# 验证记录 / Verification

日期 / Date: 2026-10-16

## 已执行 / Completed

| 检查 / Check | 结果 / Result |
| --- | --- |
| ESP32-S3 16 MB Flash、8 MB OPI PSRAM、双 3 MB OTA 配置构建 | PASS；应用镜像 1,429,888 bytes |
| 主机端方向组合、独立方向快捷操作、控制权接管、旧令牌拒绝、租约到期和时钟回绕 | PASS |
| 固件与三个客户端指令清单一致、无废弃俯仰执行路径 | PASS |
| 网页设备模块自动化测试 | 7 / 7 PASS |
| 浏览器模拟设备管理，桌面 1440×1000 / 手机 390×844 | PASS；中英文持久化、连接、扫描、方向保存、限时点动、停止、错误固件拒绝；无浏览器错误 |
| Android 单元测试与 Debug APK 构建 | PASS |
| iOS 模拟器构建 | PASS |
| iOS 本机 Swift 检查 / 远程控制检查 | 77 / 24 PASS |
| iOS 本地化检查 | 265 keys，2 languages，PASS |

浏览器设备接口为模拟响应，没有通过测试操作真实电机。Android/iOS 管理源码位于配套本地仓库，尚未上传到各自远程仓库。网页源码已推送 DF26 分支，固件源码已推送独立仓库。未烧录设备，未发布 Release。

Browser device responses were mocked; no physical motors were operated. Android/iOS changes remain in their local companion checkouts. Web source is on DF26 and firmware source is in its separate repository. No hardware was flashed and no Release was published.

## 待真机验收 / Physical acceptance pending

以下均未执行，不能据此认定硬件已验收：无舵机/I²C 启动，BLE/Wi-Fi 共存与切换，错误凭据恢复与隐藏网络，方向断电保存，多客户端争用与失联停车，实际 OTA 成功重连确认版本、传输中断重试及升级后设置保留。

Not yet tested on hardware: motor-only boot, BLE/Wi-Fi coexistence and switching, provisioning failure recovery and hidden SSIDs, calibration across power loss, competing clients and disconnect stops, successful OTA/version confirmation, interrupted transfers/retry, and settings retention after an update.

## 本地产物 / Local artifacts

`dist/looi-s3.bin`, `dist/looi-s3-manifest.json`, `dist/SHA256SUMS`, `dist/build-info.json`。构建信息记录源码和镜像校验值；dist 不提交到 Git。在线 OTA 必须在单独发布稳定 Release 并上传资产后才能使用。

Build metadata records source and image checksums. The dist folder is excluded from Git. Online OTA becomes available after a maintainer separately publishes a stable Release with these assets.

## 2.0.1 / Offline hotspot workflow verification

2026-10-16: 已重新构建 16 MB / OPI / 双 3 MB 固件。新增目标分区绑定、默认关闭、用户选择保留热点和错误分区标记拒绝的策略检查。网页端 8 项测试通过；浏览器验证了无设备预下载、损坏包不覆盖缓存、刷新后持久化、离线上传期间零在线下载、默认开启开关与关闭选项、升级后恢复互联网提示及桌面/手机双语布局。Android 59 项测试与 Lint/APK 构建通过；iOS 81 项本机检查、24 项遥控检查和原签名项目模拟器构建通过。本轮均未进行真机 OTA 或真实手机切网。

The package cache, offline upload, hotspot selection and AI internet reminders were verified with host tests and mocked browser device responses. Android/iOS builds passed. Physical OTA, hotspot shutdown and phone network recovery remain pending. The earlier 2.0.0 Release was published after the initial record above; 2.0.1 is authorized for publication in this update.

## 2.0.2 · Physical startup crash diagnosis and USB recovery

2026-10-16：真实设备串口确认 2.0.1 在广播后发生 `xQueueSemaphoreTake` 断言并持续重启。运行镜像 ELF SHA 前缀 `cb4dcadf6` 与已发布的 2.0.1 完全对应。解析回溯定位到 `LooiWifiOta::begin → WebServer::begin → NetworkServer::begin → lwip_socket → sys_mutex_lock`，即在 `WiFi.mode(WIFI_STA)` 初始化 lwIP 之前打开监听 socket。

2.0.2 将网络初始化移到 WebServer 启动之前，并在初始化失败时保留 BLE 而不启动监听。已按用户明确授权，通过 USB 只写当前 OTA 应用分区；实际分区表确认为 16 MB、双 3 MB，NVS、分区表及另一个应用区未写入。写入校验通过，复位后日志正常出现 2.0.2、NET 和 HTTP 就绪标记，观察期内未再出现断言或重复启动。蓝牙实际连接结果将在用户测试后补充；没有发送电机运动指令。

Physical serial logs reproduced the 2.0.1 boot loop and matched its published ELF hash. Symbolication identified an HTTP socket opened before lwIP initialization. With user authorization, 2.0.2 was written to the currently selected OTA application partition only. Flash data verification passed; NVS, the partition table and the other app partition were preserved. Startup reached the network/HTTP ready markers without a new panic. Physical BLE connection is pending the user's test; motors were not exercised.

### 2.0.2 BLE confirmation

修复后串口记录到 `[BLE] Client connected`，用户随后明确确认 iPhone App 已连接成功。启动与蓝牙连接问题已真机复验通过；未执行电机运动或真实 OTA 升级流程。

Serial logs recorded a BLE client connection, and the user confirmed successful connection in the iPhone app. Physical startup and BLE reconnection passed; motor motion and an actual OTA transfer were not tested in this recovery run.
