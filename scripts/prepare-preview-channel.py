#!/usr/bin/env python3
"""Prepare rolling preview feed pointers to versioned, already verified release assets."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parents[1]
output=ROOT/'dist/preview';output.mkdir(exist_ok=True)
for prefix,board in [('looi-s3','n16r8'),('looi-s3-n8r8','n8r8')]:
 manifest=json.loads((ROOT/'dist'/f'{prefix}-manifest.json').read_text())
 image=(ROOT/'dist'/f'{prefix}.bin').read_bytes()
 assert manifest['board']==board and len(image)==manifest['size']
 assert hashlib.sha256(image).hexdigest()==manifest['sha256']
 assert manifest['url'].startswith(f'https://github.com/xlle-er/LOOI-DF26-Firmware/releases/download/v{manifest["version"]}/')
 manifest['channel']='preview'
 (output/f'{prefix}-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 print(f'Prepared preview pointer for {board} -> {manifest["version"]}; no publication performed')
