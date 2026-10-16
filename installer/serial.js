// One request per open session; logs and partial lines are ignored, replies are correlated.
export async function request(port, values, timeout=6000) {
  const id=crypto.randomUUID();let reader,writer,timer;
  try {
    await port.open({baudRate:115200});
    await port.setSignals({dataTerminalReady:false,requestToSend:false});
    reader=port.readable.getReader();writer=port.writable.getWriter();
    await writer.write(new TextEncoder().encode(JSON.stringify({...values,id})+'\n'));
    writer.releaseLock();writer=null;
    const read=async()=>{let buffer='';const decoder=new TextDecoder();for(;;){const {value,done}=await reader.read();if(done)throw Error('USB disconnected');buffer+=decoder.decode(value,{stream:true});if(buffer.length>32768)throw Error('USB response too long');let end;while((end=buffer.indexOf('\n'))>=0){const line=buffer.slice(0,end);buffer=buffer.slice(end+1);let reply;try{reply=JSON.parse(line);}catch{continue;}if(reply?.id!==id)continue;if(reply.accepted===false||reply.type==='error')throw Error(reply.message||'Device rejected request');return reply;}}};
    return await Promise.race([read(),new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error('USB timeout / USB 无响应')),timeout);})]);
  } finally {
    clearTimeout(timer);if(writer)writer.releaseLock();
    if(reader){await reader.cancel().catch(()=>{});reader.releaseLock();}
    if(port.readable)await port.close().catch(()=>{});
  }
}
export async function waitForDevice(port) {
  let last;
  for(let i=0;i<10;i++){try{return await request(port,{type:'hardware_get'},2000);}catch(e){last=e;await new Promise(r=>setTimeout(r,500));}}
  throw Error('Reconnect the same USB device and select Configure / 重新选择同一块开发板，点击保存配置。'+last.message);
}
