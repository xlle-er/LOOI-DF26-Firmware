from pathlib import Path
import json,re,subprocess,sys,tempfile
root=Path(__file__).resolve().parents[1]
commands=json.loads((root/'protocol/commands.json').read_text())['commands']
ino=(root/'firmware/LooiWifiOta/LooiWifiOta.ino').read_text()
header=(root/'firmware/LooiWifiOta/LooiWifiOta.h').read_text()
assert not any(x in ino for x in ['#include <Wire.h>','Adafruit_PWM','HEAD_PITCH','head_pitch'])
assert len(commands)==len(set(commands)) and 'head_pitch' not in commands
for command in commands:assert '"'+command+'"' in ino+header,command
assert 'LOOI-DF26-Firmware/releases/latest/download/looi-s3-manifest.json' in header
# Keep cross-repository contracts aligned when the companion checkouts are available.
workspace=root.parent
for relative in ['LOOI-DF26-Android/app/src/main/java/app/looi/android/Core.kt','LOOI-DF26-IOS/Native/Sources/LooiNative/LooiBodyProtocol.swift','LOOI_DF26-WiFi/looi-life-robot/frontend/js/device/commands.js']:
 p=workspace/relative
 if p.exists():
  source=p.read_text()
  for c in commands:assert '"'+c+'"' in source,(relative,c)
for folder in ['LOOI-DF26-Android/app/src/main','LOOI-DF26-Android/FaceSource','LOOI-DF26-IOS/Native/Sources','LOOI-DF26-IOS/FaceSource','LOOI_DF26-WiFi/looi-life-robot/frontend/js']:
 p=workspace/folder
 if p.exists():
  for f in p.rglob('*'):
   if f.suffix in ['.kt','.swift','.js','.json']:assert 'head_pitch' not in f.read_text(),str(f)
with tempfile.TemporaryDirectory() as tmp:
 binary=Path(tmp)/'policy-test'
 subprocess.run(['clang++','-std=c++17','-Wall','-Wextra','-Werror',str(root/'firmware/LooiWifiOta/tests/policy_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
print('PASS: firmware/client command parity, no obsolete actuator paths, release destination and policy')
