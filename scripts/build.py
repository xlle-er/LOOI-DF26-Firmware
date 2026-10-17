#!/usr/bin/env python3
"""Build both supported profiles without modifying the source sketch or flashing hardware."""
import argparse,hashlib,json,os,re,shutil,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--board',choices=['all','n16r8','n8r8'],default='all');args=p.parse_args()
cli=os.environ.get('ARDUINO_CLI') or shutil.which('arduino-cli') or '/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli'
subprocess.run([os.sys.executable,str(ROOT/'scripts/sync-hardware.py')],check=True)
boards=json.loads((ROOT/'hardware/boards.json').read_text())
for name in (boards if args.board=='all' else [args.board]):
 b=boards[name];build=ROOT/'build'/name;build.mkdir(parents=True,exist_ok=True)
 with tempfile.TemporaryDirectory(prefix='looi-build-') as tmp:
  sketch=Path(tmp)/'LooiWifiOta';shutil.copytree(ROOT/'firmware/LooiWifiOta',sketch,ignore=shutil.ignore_patterns('tests'))
  if name=='n8r8':shutil.copyfile(ROOT/'hardware/looi_8m.csv',sketch/'partitions.csv')
  fqbn=f"esp32:esp32:esp32s3:FlashSize={b['flash_mb']}M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,USBMode=hwcdc,CDCOnBoot=cdc"
  snapshot={'fqbn':fqbn,'source_sha256':{'firmware/LooiWifiOta/'+p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sketch.iterdir() if p.suffix in ('.ino','.h')}}
  subprocess.run([cli,'compile','--fqbn',fqbn,'--build-property',f"compiler.cpp.extra_flags=-DLOOI_FLASH_MB={b['flash_mb']}",'--build-path',str(build),str(sketch)],check=True)
  (build/'build-source-info.json').write_text(json.dumps(snapshot,indent=2)+'\n')
  version=re.search(r'DEVICE_VERSION\[\] = "([^"]+)"',(ROOT/'firmware/LooiWifiOta/LooiWifiOta.h').read_text()).group(1)
  subprocess.run([os.sys.executable,str(ROOT/'scripts/package-release.py'),version,str(build),name],check=True)
shutil.copyfile(ROOT/'hardware/boards.json',ROOT/'installer/boards.json')

subprocess.run([os.sys.executable,str(ROOT/'scripts/prepare-installer.py')],check=True)
