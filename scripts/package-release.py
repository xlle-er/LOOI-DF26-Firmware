#!/usr/bin/env python3
"""Prepare release assets; never creates a GitHub release or flashes a device."""
from pathlib import Path
import hashlib,json,re,shutil,sys
root=Path(__file__).resolve().parents[1]
version=sys.argv[1]
assert re.fullmatch(r'\d+\.\d+\.\d+(?:-[A-Za-z0-9.-]+)?',version),'Invalid version'
source=Path(sys.argv[2]).resolve();dest=root/'dist';dest.mkdir(exist_ok=True)
image=source/'LooiWifiOta.ino.bin';data=image.read_bytes()
assert 32<=len(data)<=3145728 and data[0]==0xe9,'Invalid application binary'
assert f'"{version}"' in (root/'firmware/LooiWifiOta/LooiWifiOta.h').read_text(),'Version must match firmware'
sha=hashlib.sha256(data).hexdigest();(dest/'looi-s3.bin').write_bytes(data)
manifest={'version':version,'chip':'ESP32-S3','partition':'app3M_fat9M_16MB','size':len(data),'sha256':sha,'url':f'https://github.com/xlle-er/LOOI-DF26-Firmware/releases/download/v{version}/looi-s3.bin'}
(dest/'looi-s3-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
(dest/'SHA256SUMS').write_text(sha+'  looi-s3.bin\n')
info={'version':version,'esp32_arduino':'3.3.12','fqbn':'esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,USBMode=hwcdc,CDCOnBoot=default','source_sha256':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'firmware/LooiWifiOta').glob('*') if p.suffix in ['.ino','.h']},'binary_sha256':sha,'flashed':False}
(dest/'build-info.json').write_text(json.dumps(info,indent=2)+'\n')
print(f'Prepared v{version}: {len(data)} bytes; no release published')
