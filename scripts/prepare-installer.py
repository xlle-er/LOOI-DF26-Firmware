#!/usr/bin/env python3
"""Stage verified release assets on the installer's own origin (GitHub downloads lack browser CORS)."""
from pathlib import Path
import argparse,hashlib,json,shutil,tempfile
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--assets',type=Path,default=ROOT/'dist');p.add_argument('--require-all',action='store_true');args=p.parse_args()
specs=json.loads((ROOT/'hardware/boards.json').read_text());public=ROOT/'installer/public';public.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(dir=public,prefix='firmware-stage-') as tmp:
 staged=Path(tmp);count=0
 for board,spec in specs.items():
  prefix='looi-s3' if board=='n16r8' else 'looi-s3-n8r8';name=prefix+'-usb.json';meta=args.assets/name
  if not meta.exists():
   if args.require_all:raise ValueError('Missing release '+name)
   continue
  manifest=json.loads(meta.read_text())
  if any(manifest.get(k)!=spec[k] for k in ('board','chip','flash_mb','psram','psram_mb','partition','hardware_protocol')):raise ValueError('Manifest/profile mismatch')
  builds=manifest['builds']
  if len(builds)!=1 or builds[0]['chipFamily']!='ESP32-S3':raise ValueError('Invalid USB builds')
  parts=builds[0]['parts']
  if len(parts)!=4 or {p['offset'] for p in parts}!={0,0x8000,0xe000,0x10000}:raise ValueError('Invalid USB layout')
  for part in parts:
   filename=part['path']
   if Path(filename).name!=filename or not filename.endswith('.bin'):raise ValueError('Invalid asset name')
   data=(args.assets/filename).read_bytes()
   if len(data)!=part['size'] or hashlib.sha256(data).hexdigest()!=part['sha256']:raise ValueError('Corrupt release segment')
   if part['offset']==0x10000 and f"LOOI_HW_V1|{board}|1|{spec['partition']}|END".encode() not in data:raise ValueError('Missing hardware descriptor')
   (staged/filename).write_bytes(data)
  (staged/name).write_text(json.dumps(manifest,indent=2)+'\n');count+=1
 if not count:raise ValueError('No compatible release assets; build firmware or download a validated release first')
 target=public/'firmware'
 if target.exists():shutil.rmtree(target)
 shutil.copytree(staged,target)
 print(f'Staged {count} board profiles for same-origin installation; nothing published')
