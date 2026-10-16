from pathlib import Path
root=Path(__file__).resolve().parents[1]
html=(root/'shared/device/panel.html').read_text().replace('/* SHA256 */',(root/'shared/device/sha256.js').read_text().replace('export function','function'))
(root/'firmware/LooiWifiOta/DevicePanel.h').write_text('#pragma once\nconst char DEVICE_PANEL[] PROGMEM = R"LOOIPANEL('+html+')LOOIPANEL";\n')
