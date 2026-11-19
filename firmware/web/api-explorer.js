'use strict';

// BEGIN GENERATED API CATALOG
const apiOperations=[["GET","/api/v1/health","Check readiness and API-key provisioning"],["GET","/api/v1","Discover implemented local capabilities"],["GET","/api/v1/status","Read controller, workflow and front-panel state"],["GET","/api/v1/display","Read the 128×64 appliance display"],["GET","/api/v1/recipes","List persisted local recipes"],["GET","/api/v1/recipes/{id}","Read a complete local recipe",null,[["id","path",true,{"type":"string","minLength":1,"maxLength":20,"pattern":"^[A-Za-z0-9_-]+$"}]]],["PUT","/api/v1/recipes/{id}","Create or replace a persisted recipe while idle",[["Example",{"name":"example","steps":[{"name":"example","location":1,"target_f":0,"minutes":0}]}]],[["id","path",true,{"type":"string","minLength":1,"maxLength":20,"pattern":"^[A-Za-z0-9_-]+$"}]]],["DELETE","/api/v1/recipes/{id}","Remove a recipe while idle",null,[["id","path",true,{"type":"string","minLength":1,"maxLength":20,"pattern":"^[A-Za-z0-9_-]+$"}]]],["POST","/api/v1/commands","Queue a workflow or operate the virtual front panel",[["stop",{"command":"stop"}],["brew",{"command":"brew","recipe_id":"example"}],["circulate",{"command":"circulate"}],["clear-stop",{"command":"clear-stop"}],["cold-brew",{"command":"cold-brew"}],["descale",{"command":"descale"}],["drain-filter",{"command":"drain-filter"}],["drain-reservoir",{"command":"drain-reservoir"}],["encoder",{"command":"encoder","delta":1}],["first-rinse",{"command":"first-rinse"}],["manual",{"command":"manual"}],["rack-beer",{"command":"rack-beer"}],["rinse",{"command":"rinse"}],["settings",{"command":"settings"}],["sous-vide",{"command":"sous-vide"}],["utilities",{"command":"utilities"}]]],["GET","/api/v1/settings","Read nonsecret display preferences"],["PATCH","/api/v1/settings","Persist supported preferences while idle",[["Example",{"temperature_unit":"F"}]]],["GET","/api/v1/events","Read the recent volatile event ring"],["GET","/api/v1/history","List retained brews and recording capacity"],["GET","/api/v1/history/samples/{offset}","Read a page of recorded brew samples",null,[["offset","path",true,{"type":"integer","minimum":0,"maximum":4224}],["id","query",true,{"type":"integer","minimum":1,"maximum":4294967295}]]],["GET","/api/v1/telemetry","Read optional telemetry destination and delivery status"],["PUT","/api/v1/telemetry","Configure optional telemetry forwarding while idle",[["Example",{"enabled":false,"host":"","port":8080}]]],["POST","/api/v1/recipes/restore-defaults","Add missing starter PicoPak recipes while idle"]];
// END GENERATED API CATALOG

function setupApiExplorer({session, enqueue}) {
  const el=id=>document.getElementById(id);
  const endpoint=el('apiEndpoint');
  const example=el('apiExample'), body=el('apiBody');
  const status=el('apiStatus'), response=el('apiResponse');
  let busy=false, parameters=[];
  for (const [method,path] of apiOperations)
    endpoint.add(new Option(method+' '+path,method+' '+path));
  const operation=()=>apiOperations[endpoint.selectedIndex];
  const update=()=>{
    el('apiSend').disabled=busy || !session().connected;
    for (const input of [endpoint,example,body,...parameters.map(item=>item.input)]) input.disabled=busy;
    if (!session().connected) {status.textContent='Connect to send a request.';response.textContent='';}
  };
  const loadExample=()=>{body.value=JSON.stringify(operation()[3][example.selectedIndex][1],null,2);};
  const select=()=>{
    const [method,path,summary,examples,specs=[]]=operation();
    el('apiDescription').textContent=summary;
    const row=el('apiParameterRow');row.hidden=!specs.length;row.replaceChildren();
    parameters=specs.map(([name,place,required,schema],index)=>{
      const input=document.createElement('input'), label=document.createElement('label');
      input.id='apiParameter'+(index || '');input.required=required;input.autocomplete='off';
      if (schema.type==='integer') input.inputMode='numeric';
      if (schema.maxLength) input.maxLength=schema.maxLength;
      input.value=schema.default ?? '';
      label.htmlFor=input.id;label.textContent=name+' ('+place+')';row.append(label,input);
      return {name,place,schema,input};
    });
    el('apiBodyRow').hidden=!examples;
    el('apiExampleRow').hidden=!examples || examples.length<2;
    body.required=!!examples;
    example.replaceChildren();body.value='';
    if (examples) {
      for (const [name] of examples) example.add(new Option(name));
      loadExample();
    }
    status.textContent=method==='GET'?'Read-only request.':'This request changes the brewer or saved data.';
    response.textContent='';update();
  };
  endpoint.addEventListener('change',select);
  example.addEventListener('change',loadExample);
  el('apiForm').addEventListener('submit',async event=>{
    event.preventDefault();
    const connection=session();
    if (busy || !connection.connected) return;
    const current=()=>session().connected && session().generation===connection.generation;
    const [method,template,,examples]=operation();
    let path=template, payload;
    try {
      const query=new URLSearchParams();
      for (const {name,place,schema,input} of parameters) {
        let value=input.value.trim();const number=Number(value);
        if (!value && !input.required) continue;
        if (!value || (schema.pattern && !new RegExp(schema.pattern).test(value)) ||
            value.length<(schema.minLength || 0) || value.length>(schema.maxLength || Infinity) ||
            (schema.type==='integer' && (!/^-?\d+$/.test(value) || !Number.isSafeInteger(number) ||
              number<(schema.minimum ?? -Infinity) || number>(schema.maximum ?? Infinity))))
          throw new Error('Invalid '+name+'. Check the parameter value.');
        if (schema.type==='integer') value=String(number);
        if (place==='path') path=path.replace('{'+name+'}',encodeURIComponent(value));
        else query.set(name,value);
      }
      if (query.size) path+='?'+query;
      if (examples) {
        JSON.parse(body.value);
        payload=body.value;
        if (new TextEncoder().encode(payload).length>4095) throw new Error('JSON body exceeds 4095 bytes.');
      }
      busy=true;update();status.textContent='Sending '+method+' '+path;response.textContent='';
      const result=await enqueue(async()=>{
        if (!current()) throw new Error('Connection changed.');
        const controller=new AbortController(), timer=setTimeout(()=>controller.abort(),6000);
        try {
          const headers={Accept:'application/json',Authorization:'Bearer '+connection.key};
          if (payload!==undefined || ['POST','PUT','PATCH'].includes(method)) headers['Content-Type']='application/json';
          const reply=await fetch(path,{method,headers,body:payload,signal:controller.signal,
            cache:'no-store',credentials:'omit',redirect:'error'});
          return [reply.status,await reply.text()];
        } finally {clearTimeout(timer);}
      });
      if (current()) {
        status.textContent=method+' '+path+' · HTTP '+result[0];
        try {response.textContent=JSON.stringify(JSON.parse(result[1]),null,2);}
        catch (_) {response.textContent=result[1] || '(empty response)';}
      }
    } catch (error) {
      if (current()) status.textContent=error.name==='AbortError'?'Request timed out.':error.message;
    } finally {busy=false;update();}
  });
  select();
  return {update};
}
