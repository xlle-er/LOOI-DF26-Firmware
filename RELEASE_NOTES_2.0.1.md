# 2.0.1 · Hotspot recovery

- Adds `close_ap_after_update` (default true) and capability reporting.
- Persists a one-boot choice bound to the verified OTA target partition; a failed OTA cannot suppress the old partition's recovery hotspot.
- Retains BLE, saved network and calibration; hold BOOT 3 seconds to reopen the hotspot.
- The device panel explains downloading the file before joining a hotspot and restoring internet before AI companionship.

首次从旧固件升级后仍需手动断开热点；开关对安装 2.0.1 后发起的 OTA 生效。关闭设备热点不保证手机自动恢复互联网。本次为构建和模拟测试，真机验证待完成。
The first upgrade from older firmware still requires manually leaving the hotspot. This option applies to OTA started after 2.0.1 is installed. Phone reconnection remains OS-controlled. Hardware acceptance is pending.
