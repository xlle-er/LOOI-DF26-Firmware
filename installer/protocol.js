export const pinNames = ['ena', 'in1', 'in2', 'in3', 'in4', 'enb'];
export const releaseBase = 'https://github.com/xlle-er/LOOI-DF26-Firmware/releases/';
export function validateProfile(profile, boards) {
  const spec = boards[profile?.board];
  if (!spec || profile.version !== 1) throw Error('Unsupported hardware profile / 不支持的硬件配置');
  const pins = profile.pins;
  if (!pins || Object.keys(pins).length !== 6 || pinNames.some(k => !Number.isInteger(pins[k]) || !spec.allowed_gpio.includes(pins[k])))
    throw Error('Invalid/reserved GPIO / GPIO 无效或已被保留');
  if (new Set(Object.values(pins)).size !== 6) throw Error('Duplicate GPIO / GPIO 不能重复');
  return profile;
}
export async function sha256(data) {
  return [...new Uint8Array(await crypto.subtle.digest('SHA-256', data))].map(v => v.toString(16).padStart(2, '0')).join('');
}
export function trustedUrl(value) {
  const u = new URL(value);
  return u.protocol === 'https:' && !u.username && !u.password &&
    ((u.hostname === 'github.com' && u.pathname.startsWith('/xlle-er/LOOI-DF26-Firmware/releases/')) ||
      ['release-assets.githubusercontent.com', 'objects.githubusercontent.com'].includes(u.hostname));
}
export function validateManifest(m, spec) {
  for (const k of ['board', 'chip', 'flash_mb', 'psram', 'psram_mb', 'partition', 'hardware_protocol'])
    if (m[k] !== spec[k]) throw Error(`Incompatible manifest / 安装包不匹配: ${k}`);
  if (!/^\d+\.\d+\.\d+(?:-[A-Za-z0-9.-]+)?$/.test(m.version)) throw Error('Invalid version');
  const builds = m.builds;
  if (builds?.length !== 1 || builds[0].chipFamily !== 'ESP32-S3') throw Error('Invalid USB manifest');
  const parts = builds[0].parts;
  if (parts?.length !== 4 || JSON.stringify(parts.map(p => p.offset).sort((a,b)=>a-b)) !== JSON.stringify([0,32768,57344,65536])) throw Error('Invalid flash layout');
  const ordered = [...parts].sort((a,b)=>a.offset-b.offset);
  for (const [i,p] of ordered.entries()) {
    if (!Number.isInteger(p.size) || p.size <= 0 || p.size > 3145728 || !/^[a-f0-9]{64}$/.test(p.sha256) || !trustedUrl(p.url)) throw Error('Invalid USB segment');
    if (i && ordered[i-1].offset + ordered[i-1].size > p.offset) throw Error('Overlapping segments');
  }
  if (parts.find(p=>p.offset===32768).sha256 !== m.partition_sha256) throw Error('Partition digest mismatch');
  const app=parts.find(p=>p.offset===65536);
  if(app.size!==m.size || app.sha256!==m.sha256)throw Error('Application metadata mismatch');
  return parts;
}
async function getAsset(url, limit) {
  const sameOrigin=value=>new URL(value,location.href).origin===location.origin;
  if (!sameOrigin(url)) throw Error('Installer assets must share the page origin');
  const response = await fetch(url, {cache:'no-store', signal:AbortSignal.timeout(60000)});
  if (!response.ok || !sameOrigin(response.url)) throw Error('Compatible release unavailable / 兼容版本尚未发布');
  const chunks = []; let size=0;
  const reader=response.body.getReader();
  try {
    for (;;) { const {value,done}=await reader.read(); if(done)break; size+=value.length; if(size>limit)throw Error('Asset too large'); chunks.push(value); }
  } finally { await reader.cancel(); reader.releaseLock(); }
  const data=new Uint8Array(size);let offset=0;for(const chunk of chunks){data.set(chunk,offset);offset+=chunk.length;}return data;
}
export async function loadRelease(spec) {
  const prefix=spec.board==='n16r8'?'looi-s3':'looi-s3-n8r8';
  const base=new URL('./firmware/',document.baseURI);
  const manifest=JSON.parse(new TextDecoder().decode(await getAsset(new URL(prefix+'-usb.json',base).href,65536)));
  const parts=validateManifest(manifest,spec),images=[];
  for(const part of parts){if(!/^[A-Za-z0-9_.-]+\.bin$/.test(part.path))throw Error('Invalid asset name');const data=await getAsset(new URL(part.path,base).href,part.size);if(data.length!==part.size || await sha256(data)!==part.sha256)throw Error('Corrupt release asset / 安装包校验失败');images.push({...part,data});}
  const marker=`LOOI_HW_V1|${spec.board}|1|${spec.partition}|END`;
  if(!new TextDecoder().decode(images.find(p=>p.offset===65536).data).includes(marker))throw Error('Missing hardware descriptor');
  return {manifest,images};
}
export function safePartition(current, expected) {
  return current.every(v=>v===255) || (current.length>=expected.length && expected.every((v,i)=>current[i]===v));
}
export async function performFlash(loader,spec,release,onProgress=()=>{}) {
  if(loader.chip.CHIP_NAME!=='ESP32-S3')throw Error('ESP32-S3 required / 芯片型号不符');
  if(await loader.getFlashSize()!==spec.flash_mb*1024)throw Error('Physical Flash mismatch / Flash 容量不符');
  const current=await loader.readFlash(32768,4096);
  if(!safePartition(current,release.images.find(p=>p.offset===32768).data))throw Error('Unknown partition layout. Use Agent inspection / 未知分区，请使用 Agent 检查，未写入固件');
  const binary=data=>{let s='';for(let i=0;i<data.length;i+=8192)s+=String.fromCharCode(...data.subarray(i,i+8192));return s;};
  await loader.writeFlash({fileArray:release.images.map(p=>({address:p.offset,data:binary(p.data)})),flashSize:'keep',flashMode:'keep',flashFreq:'keep',eraseAll:false,compress:true,reportProgress:onProgress});
  for(const part of release.images){const written=await loader.readFlash(part.offset,part.size);if(await sha256(written)!==part.sha256)throw Error('USB write verification failed / 写入校验失败');}
}
export function verifyState(state,spec,profile,version) {
  if(state.firmware_version!==version || state.hardware_profile?.board!==spec.board || state.physical_flash_bytes!==spec.flash_mb*1048576 || state.psram_bytes!==8388608)throw Error('Device version/memory mismatch / 版本或内存不匹配');
  if(!state.hardware_ready)throw Error(state.hardware_error||'Hardware not ready');
  if(state.hardware_profile?.version!==profile.version || pinNames.some(k=>state.hardware_profile.pins[k]!==profile.pins[k]))throw Error('Wiring readback mismatch / 接线回读不匹配');
  return state;
}
