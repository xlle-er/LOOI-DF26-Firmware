import test from 'node:test';
import assert from 'node:assert/strict';
import {request} from '../serial.js';
function fakePort(mode='ok') {
 const port={closed:false,locked:false};let controller;
 port.open=async()=>{port.readable=new ReadableStream({start(c){controller=c;}});port.writable=new WritableStream({write(data){const req=JSON.parse(new TextDecoder().decode(data));if(mode==='timeout')return;const response=JSON.stringify({id:req.id,type:mode==='reject'?'error':'ack',accepted:mode!=='reject',message:mode==='reject'?'Administrator password required':'',hardware_ready:true});controller.enqueue(new TextEncoder().encode('[BOOT] log\n{"id":"other","accepted":true}\n'+response.slice(0,20)));controller.enqueue(new TextEncoder().encode(response.slice(20)+'\n'));}});};
 port.setSignals=async values=>{assert.equal(values.dataTerminalReady,false);assert.equal(values.requestToSend,false);};
 port.close=async()=>{port.closed=true;port.readable=null;};return port;
}
test('USB correlates IDs across split packets and ignores boot logs',async()=>{const port=fakePort();const result=await request(port,{type:'hardware_get'});assert.equal(result.hardware_ready,true);assert.equal(port.closed,true);});
test('USB propagates rejection and closes the port',async()=>{const port=fakePort('reject');await assert.rejects(request(port,{type:'hardware_set',admin_password:'never-log-this'}),/Administrator/);assert.equal(port.closed,true);});
test('USB timeout releases read lock and closes the port',async()=>{const port=fakePort('timeout');await assert.rejects(request(port,{type:'hardware_get'},10),/timeout/);assert.equal(port.closed,true);});
