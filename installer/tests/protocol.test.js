import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {validateProfile,validateManifest,safePartition,performFlash,verifyState,sha256} from '../protocol.js';
const boards=JSON.parse(await readFile(new URL('../boards.json',import.meta.url)));
const profile={version:1,board:'n16r8',pins:boards.n16r8.default_pins};
test('valid custom wiring and invalid GPIO types/reservations',()=>{
 assert.equal(validateProfile(profile,boards),profile);
 for(const pin of [true,17.5,'17',0,3,19,20,33,37,38,43,44,45,46,48])assert.throws(()=>validateProfile({...profile,pins:{...profile.pins,ena:pin}},boards));
 assert.throws(()=>validateProfile({...profile,pins:{...profile.pins,ena:16}},boards));
});
test('blank/known layouts allowed, unknown or truncated rejected',()=>{
 assert.equal(safePartition(new Uint8Array(4096).fill(255),new Uint8Array([1,2])),true);
 assert.equal(safePartition(new Uint8Array([1,2,255]),new Uint8Array([1,2])),true);
 assert.equal(safePartition(new Uint8Array([1,3]),new Uint8Array([1,2])),false);
 assert.equal(safePartition(new Uint8Array([1]),new Uint8Array([1,2])),false);
});
function fakeRelease(){return {manifest:{version:'2.1.0'},images:[0,32768,57344,65536].map(offset=>({offset,data:new Uint8Array([1,2,3,4])}))};}
test('flash preflight refuses wrong chip/capacity/layout without writing',async()=>{
 for(const reason of ['chip','size','layout']){let wrote=false;const loader={chip:{CHIP_NAME:reason==='chip'?'ESP32':'ESP32-S3'},getFlashSize:async()=>reason==='size'?8192:16384,readFlash:async()=>new Uint8Array(4096).fill(reason==='layout'?3:255),writeFlash:async()=>{wrote=true;}};await assert.rejects(performFlash(loader,boards.n16r8,fakeRelease()));assert.equal(wrote,false);}
});
test('segmented write preserves NVS and verifies every segment',async()=>{
 const release=fakeRelease();for(const image of release.images)image.sha256=await sha256(image.data);
 let wrote=false;const loader={chip:{CHIP_NAME:'ESP32-S3'},getFlashSize:async()=>16384,readFlash:async()=>wrote?new Uint8Array([1,2,3,4]):new Uint8Array(4096).fill(255),writeFlash:async options=>{assert.equal(options.eraseAll,false);assert.deepEqual(options.fileArray.map(p=>p.address),[0,32768,57344,65536]);wrote=true;}};
 await performFlash(loader,boards.n16r8,release);assert.equal(wrote,true);
 loader.readFlash=async()=>new Uint8Array([9,9,9,9]);await assert.rejects(performFlash(loader,boards.n16r8,release));
});
test('success needs version, physical memory, readiness and full pin readback',()=>{
 const state={firmware_version:'2.1.0',hardware_profile:profile,physical_flash_bytes:16777216,psram_bytes:8388608,hardware_ready:true};
 assert.equal(verifyState(state,boards.n16r8,profile,'2.1.0'),state);
 for(const override of [{firmware_version:'2.0.2'},{hardware_ready:false},{psram_bytes:0},{physical_flash_bytes:8388608},{hardware_profile:{...profile,pins:{...profile.pins,ena:4}}}])assert.throws(()=>verifyState({...state,...override},boards.n16r8,profile,'2.1.0'));
});
test('manifest rejects other boards, unsafe URLs and overlap',()=>{
 const m={...boards.n16r8,version:'2.1.0',size:4,sha256:'a'.repeat(64),partition_sha256:'a'.repeat(64),builds:[{chipFamily:'ESP32-S3',parts:[0,32768,57344,65536].map(offset=>({offset,size:4,sha256:'a'.repeat(64),url:'https://github.com/xlle-er/LOOI-DF26-Firmware/releases/download/v2.1.0/a.bin'}))}]};
 validateManifest(m,boards.n16r8);assert.throws(()=>validateManifest(m,boards.n8r8));
 m.builds[0].parts[0].size=40000;assert.throws(()=>validateManifest(m,boards.n16r8));
 m.builds[0].parts[0].size=4;m.builds[0].parts[0].url='https://evil.example/a.bin';assert.throws(()=>validateManifest(m,boards.n16r8));
});
