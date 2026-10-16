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
