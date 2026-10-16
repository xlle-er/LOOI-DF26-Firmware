# 2.0.2 · Startup crash recovery

修复 Wi-Fi 网络栈初始化之前启动 WebServer 导致的启动崩溃与循环重启。设备会短暂广播 LOOI Body，随后重启，表现为手机蓝牙连接超时。启动日志增加网络栈与 HTTP 服务就绪标记。

Fixes a startup crash caused by opening the HTTP listener before Wi-Fi initializes lwIP. The device briefly advertises BLE and then reboots, resulting in client connection timeouts. Network and HTTP startup now have explicit serial markers.

受影响的 2.0.0 / 2.0.1 若已循环重启，需要通过 USB 恢复应用固件；此时设备无法正常提供 OTA 服务。保留 NVS、网络与方向设置，无需擦除整片 Flash。

If 2.0.0/2.0.1 is already rebooting, recover the application over USB; OTA cannot run during the boot loop. Preserve NVS and the partition table rather than erasing the entire flash.
