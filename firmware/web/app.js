'use strict';

const $ = id => document.getElementById(id);
const state = {key:'', connected:false, generation:0, status:null, recipes:[],
  settings:{temperature_unit:'F', language:'en', detailed_status:false}, capabilities:[],
  editorId:null, dirty:false, tab:'overview', polling:false, restoreDefaults:false,
  telemetry:null, telemetryDirty:false, telemetrySaving:false, telemetryAt:0};
let requestQueue = Promise.resolve();
let pollTimer;
let apiExplorer;
let brewHistory;
const programLabels = {'sous-vide':'Sous vide', 'cold-brew':'Cold brew', manual:'Manual brewing',
  rinse:'Rinse', 'first-rinse':'First rinse', descale:'Descale', circulate:'Circulate',
  'drain-reservoir':'Drain reservoir', 'drain-filter':'Drain step filter', 'rack-beer':'Rack beer',
  utilities:'Utilities menu', settings:'Settings menu'};

function notify(message, error=false) {
  $('notice').textContent = message;
  $('notice').classList.toggle('error', error);
  $('notice').hidden = !message;
}

function api(path, method='GET', body) {
  const generation = state.generation;
  const key = state.key;
  const task = async () => {
    if (generation !== state.generation) throw new Error('Connection changed.');
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 6000);
    try {
      const headers = {Accept:'application/json', Authorization:'Bearer '+key};
      if (body !== undefined || ['POST','PUT','PATCH'].includes(method)) headers['Content-Type'] = 'application/json';
      const response = await fetch('/api/v1'+path, {method, headers,
        body:body === undefined ? undefined : JSON.stringify(body),
        signal:controller.signal, cache:'no-store', credentials:'omit', redirect:'error'});
      let data;
      try { data = response.status === 204 ? null : await response.json(); }
      catch (_) { throw new Error('The brewer returned an unreadable response.'); }
      if (!response.ok) {
        const error = new Error(data.message || data.error || 'Request failed ('+response.status+').');
        error.status = response.status;
        throw error;
      }
      if (generation !== state.generation) throw new Error('Connection changed.');
      return data;
    } catch (error) {
      if (error.name === 'AbortError') throw new Error('The brewer did not respond. Check its connection.');
      throw error;
    } finally { clearTimeout(timeout); }
  };
  // One connection at a time matches the Pico's bounded embedded HTTP server.
  const result = requestQueue.catch(() => {}).then(task);
  requestQueue = result;
  return result;
}

function connected(value) {
  state.connected = value;
  $('connectionText').textContent = value ? 'Connected' : 'Disconnected';
  $('connectionDot').classList.toggle('online', value);
  $('connectButton').textContent = value ? 'Device key' : 'Connect';
  updateControls();
}

function updateControls() {
  const ready = state.connected && state.status?.ready === true;
  const busy = !!state.status?.busy;
  const stopped = !!state.status?.stopped;
  document.querySelectorAll('[data-encoder]').forEach(button => button.disabled = !ready);
  $('encoderClick').disabled = !ready;
  $('brewRecipe').disabled = !ready || busy || stopped;
  $('startRecipe').disabled = !ready || busy || stopped || !$('brewRecipe').value;
  $('program').disabled = !ready || busy || stopped;
  $('runProgram').disabled = !ready || busy || stopped || !$('program').value;
  $('stopButton').disabled = !ready || stopped;
  $('clearStop').hidden = !stopped;
  $('clearStop').disabled = !ready || busy;
  $('newRecipe').disabled = !ready || busy;
  $('restoreRecipes').disabled = !ready || busy || !state.restoreDefaults;
  $('saveRecipe').disabled = !ready || busy;
  $('deleteRecipe').disabled = !ready || busy || !state.editorId;
  $('saveSettings').disabled = !ready || busy;
  for (const input of $('telemetryForm').elements) input.disabled = !ready || busy || !state.telemetry || state.telemetrySaving;
  apiExplorer?.update();
  brewHistory?.update();
}

function temperature(value) {
  const number = Number(value);
  if (value === null || value === undefined || !Number.isFinite(number)) return '—';
  const unit = state.settings.temperature_unit;
  return (unit === 'C' ? (number-32)*5/9 : number).toFixed(1)+'°'+unit;
}

function duration(seconds) {
  if (!Number.isFinite(Number(seconds))) return '—';
  const value = Math.max(0, Math.floor(Number(seconds)));
  const minutes = Math.floor(value/60);
  return minutes+':'+String(value%60).padStart(2,'0');
}

function renderStatus(status) {
  state.status = status;
  $('wortTemperature').textContent = temperature(status.wort_f);
  $('blockTemperature').textContent = temperature(status.thermoblock_f);
  $('targetTemperature').textContent = status.target_f === 0 ? 'Unheated' : temperature(status.target_f);
  $('stepTime').textContent = duration(status.step_seconds);
  const phases = ['Heating','Holding temperature','Cooling','Draining','Idle','Circulating','Cooling thermoblock','Pump hold','Heating'];
  $('phaseName').textContent = phases[status.phase] || 'Phase '+status.phase;
  $('operationTitle').textContent = status.stopped ? 'Outputs stopped.' : status.busy ? (status.recipe_name || 'Operation in progress.') : 'Ready for your next brew.';
  $('operationDetail').textContent = status.stopped ? (status.stop_reason === 'filter-removed' ? 'The step filter was removed during pumping. This operation is cancelled. Refit the filter and wait for idle before clearing the stop.' : 'The stop latch is active. Wait for the operation to end before clearing it.') : status.busy ? (status.step_name || 'Follow the brewer’s prompts on the remote front panel.') : 'Choose a saved recipe, open a program, or use the front panel.';
  $('modeBadge').textContent = status.stopped ? 'STOPPED' : status.busy ? 'IN PROGRESS' : 'READY';
  $('modeBadge').classList.toggle('stopped', !!status.stopped);
  $('filterBadge').textContent = status.filter_inserted === null ? 'FILTER UNKNOWN' : status.filter_inserted ? 'FILTER INSERTED' : 'FILTER REMOVED';
  $('filterBadge').classList.toggle('warning', status.filter_inserted === false);
  $('deviceId').textContent = status.device_uid ? 'Device '+status.device_uid : '';
  $('lastUpdated').textContent = 'Updated '+new Date().toLocaleTimeString();
  updateControls();
}

function renderDisplay(frame) {
  if (frame.width !== 128 || frame.height !== 64 || frame.format !== 'page-lsb-hex' || !/^[0-9a-fA-F]{2048}$/.test(frame.data)) return;
  const context = $('display').getContext('2d');
  const image = context.createImageData(128,64);
  for (let y=0;y<64;y++) for (let x=0;x<128;x++) {
    // The API exposes panel-order bytes; the Pico draws with rotation 2.
    const sourceX=127-x, sourceY=63-y;
    const address = (Math.floor(sourceY/8)*128+sourceX)*2;
    const lit = (parseInt(frame.data.slice(address,address+2),16) >> (sourceY%8)) & 1;
    const i=(y*128+x)*4;
    const color = lit ? [199,225,180] : [18,38,32];
    image.data[i]=color[0]; image.data[i+1]=color[1]; image.data[i+2]=color[2]; image.data[i+3]=255;
  }
  context.putImageData(image,0,0);
  $('displayHint').textContent = 'LIVE OLED · 128 × 64';
}

function renderPrograms() {
  $('program').replaceChildren();
  for (const command of state.capabilities) if (programLabels[command]) {
    $('program').add(new Option(programLabels[command],command));
  }
  updateControls();
}

async function refreshRecipes() {
  const data = await api('/recipes');
  state.recipes = data.recipes || [];
  const previous = $('brewRecipe').value;
  $('brewRecipe').replaceChildren(new Option('Choose a recipe',''));
  $('recipeList').replaceChildren();
  for (const recipe of state.recipes) {
    $('brewRecipe').add(new Option(recipe.name,recipe.id));
    const button=document.createElement('button'); button.type='button'; button.className='recipe-item';
    button.classList.toggle('active', recipe.id === state.editorId);
    const name=document.createElement('strong');name.textContent=recipe.name;
    const detail=document.createElement('small');detail.textContent=recipe.steps+' steps · '+recipe.id;
    button.append(name,detail);button.addEventListener('click',()=>loadRecipe(recipe.id));$('recipeList').append(button);
  }
  if (!state.recipes.length) {
    const empty=document.createElement('p');empty.className='empty';empty.textContent='Your collection is empty. Create or import your first recipe.';$('recipeList').append(empty);
  }
  if (state.recipes.some(recipe=>recipe.id===previous)) $('brewRecipe').value=previous;
  else if (state.recipes.length) $('brewRecipe').value=state.recipes[0].id;
  $('recipeCount').textContent=state.recipes.length;
  $('recipeCapacity').textContent=state.recipes.length+' / '+data.capacity;
  updateControls();
}

function addStep(step={name:'New step',location:1,target_f:152,minutes:20,drain_minutes:0}) {
  if ($('steps').children.length>=15) {notify('A recipe can contain up to 15 steps.',true);return;}
  const row=document.createElement('tr');
  const definitions=[['name','text',null,null],['location','select',1,6],['target_f','number',0,212],['minutes','number',0,599],['drain_minutes','number',0,120]];
  for (const [field,type,min,max] of definitions) {
    const cell=document.createElement('td');let input;
    if(type==='select') {input=document.createElement('select');for(let i=1;i<=6;i++)input.add(new Option(String(i),String(i)));}
    else {input=document.createElement('input');input.type=type;if(type==='text')input.maxLength=21;else{input.min=min;input.max=max;input.step=1;}}
    input.dataset.field=field;input.required=true;input.value=step[field] ?? '';
    input.setAttribute('aria-label',field.replaceAll('_',' '));cell.append(input);row.append(cell);
  }
  const cell=document.createElement('td');const remove=document.createElement('button');remove.type='button';remove.className='remove';remove.textContent='×';remove.setAttribute('aria-label','Remove step');
  remove.addEventListener('click',()=>{row.remove();state.dirty=true;});cell.append(remove);row.append(cell);$('steps').append(row);
}

function editRecipe(recipe, existing=false) {
  state.editorId=existing ? recipe.id : null;state.dirty=false;
  $('recipeName').value=recipe.name || '';$('recipeId').value=recipe.id || '';$('recipeId').readOnly=existing;
  $('recipeId').dataset.auto=existing ? 'false' : 'true';
  $('editorHeading').textContent=existing ? 'Edit recipe' : 'New recipe';$('steps').replaceChildren();
  for(const step of recipe.steps || [{name:'Mash',location:1,target_f:152,minutes:60,drain_minutes:0}])addStep(step);
  updateControls();
}

async function loadRecipe(id) {
  if(state.dirty && !confirm('Discard the unsaved recipe changes?'))return;
  try {const recipe=await api('/recipes/'+encodeURIComponent(id));editRecipe({...recipe,id},true);notify('');}
  catch(error){notify(error.message,true);}
}

function recipeFromForm() {
  if(!$('recipeForm').reportValidity())return null;
  const steps=Array.from($('steps').children).map(row=>{
    const step={};row.querySelectorAll('[data-field]').forEach(input=>step[input.dataset.field]=input.dataset.field==='name'?input.value:Number(input.value));return step;
  });
  if(!steps.length){notify('Add at least one brewing step.',true);return null;}
  if(steps.some(step=>step.target_f!==0 && step.target_f<60)){notify('Targets must be 0 for unheated circulation, or between 60 and 212 °F.',true);return null;}
  return {id:$('recipeId').value,name:$('recipeName').value,steps};
}

async function sendCommand(body) {
  try {await api('/commands','POST',body);notify(body.command==='stop'?'Stop requested.':'Command accepted.');renderStatus(await api('/status'));}
  catch(error){notify(error.message,true);}
}

function download(name,data) {
  const blob=new Blob([JSON.stringify(data,null,2)+'\n'],{type:'application/json'});
  const url=URL.createObjectURL(blob);const link=document.createElement('a');link.href=url;link.download=name;link.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
}

function renderSettings(settings) {
  state.settings=settings;$('temperatureUnit').value=settings.temperature_unit;$('language').value=settings.language;$('detailedStatus').checked=!!settings.detailed_status;
}

function renderTelemetry(config) {
  state.telemetry=config;
  if (!state.telemetryDirty) {
    $('telemetryEnabled').checked=!!config.enabled;$('telemetryHost').value=config.host || '';
    $('telemetryPort').value=config.port;$('telemetryHost').required=!!config.enabled;
  }
  const status=config.status || {};
  const labels={disabled:'Forwarding is off.',offline:'Waiting for the Pico’s network connection.',
    'waiting-idle':'Waiting for the brewer to be idle to look up the receiver.',ready:'Ready to send readings.',
    connecting:'Connecting to the receiver…',sending:'Sending readings…',receiving:'Waiting for the receiver’s reply…',
    backoff:'Receiver unavailable. The Pico will retry.'};
  $('telemetryStatus').textContent=(labels[status.state] || 'Waiting for forwarding status.')+
    ' Sent: '+(status.sent || 0)+' · Failed: '+(status.failed || 0)+' · Skipped: '+(status.dropped || 0)+
    (status.last_http_status?' · Last reply: HTTP '+status.last_http_status:'')+
    (status.storage_error?' · Destination settings could not be saved.':'');
  updateControls();
}

async function refreshTelemetry() {
  const generation=state.generation;
  state.telemetryAt=Date.now();
  try {renderTelemetry(await api('/telemetry'));}
  catch(error) {if(generation===state.generation)$('telemetryStatus').textContent=error.message;}
}

async function poll() {
  clearTimeout(pollTimer);
  if(!state.key || state.polling)return;
  state.polling=true;const generation=state.generation;
  try {
    if(!document.hidden) {
      renderStatus(await api('/status'));connected(true);
      if(state.tab==='overview')renderDisplay(await api('/display'));
      if(state.tab==='settings' && Date.now()-state.telemetryAt>=5000)await refreshTelemetry();
    }
  } catch(error) {
    if(generation===state.generation){connected(false);notify(error.message,true);if(error.status===401){state.key='';try{sessionStorage.removeItem('pico-api-key');}catch(_){}}}
  } finally {state.polling=false;if(state.key)pollTimer=setTimeout(poll,state.connected?1200:3000);}
}

function selectTab(tab) {
  state.tab=tab;document.querySelectorAll('.tab-panel').forEach(panel=>panel.hidden=panel.id!==tab);
  document.querySelectorAll('[data-tab]').forEach(button=>{const active=button.dataset.tab===tab;button.classList.toggle('selected',active);if(active)button.setAttribute('aria-current','page');else button.removeAttribute('aria-current');});
  if(state.connected && tab==='recipes')refreshRecipes().catch(error=>notify(error.message,true));
  if(state.connected && tab==='settings')refreshTelemetry();
  if(tab==='history')brewHistory?.activate();
}

async function connect(key) {
  state.key=key;state.generation++;clearTimeout(pollTimer);
  state.telemetry=null;state.telemetryDirty=false;state.telemetrySaving=false;state.telemetryAt=0;
  const info=await api('');state.capabilities=info.commands || [];renderPrograms();
  state.restoreDefaults=info.restore_default_recipes === true;
  $('firmwareVersion').textContent=info.firmware_version ? 'Firmware '+info.firmware_version : 'Standalone firmware';
  renderSettings(await api('/settings'));renderStatus(await api('/status'));await refreshRecipes();
  try{sessionStorage.setItem('pico-api-key',key);}catch(_){}
  connected(true);notify('');$('connectDialog').close();$('apiKey').value='';poll();
}

function disconnect() {
  state.generation++;state.key='';state.status=null;clearTimeout(pollTimer);connected(false);
  state.telemetry=null;state.telemetryDirty=false;$('telemetryStatus').textContent='Connect to load forwarding settings.';
  try{sessionStorage.removeItem('pico-api-key');}catch(_){}
  $('operationTitle').textContent='Disconnected from your brewer.';$('operationDetail').textContent='A running program continues on the Pico. Reconnect to see its status.';
  $('apiKey').value='';notify('Device key removed from this tab.');
}

document.querySelectorAll('[data-tab]').forEach(button=>button.addEventListener('click',()=>selectTab(button.dataset.tab)));
document.querySelectorAll('[data-encoder]').forEach(button=>button.addEventListener('click',()=>sendCommand({command:'encoder',delta:Number(button.dataset.encoder),click:false})));
$('encoderClick').addEventListener('click',()=>sendCommand({command:'encoder',delta:0,click:true}));
$('brewRecipe').addEventListener('change',updateControls);
$('startRecipe').addEventListener('click',()=>sendCommand({command:'brew',recipe_id:$('brewRecipe').value}));
$('runProgram').addEventListener('click',()=>sendCommand({command:$('program').value}));
$('stopButton').addEventListener('click',()=>sendCommand({command:'stop'}));
$('clearStop').addEventListener('click',()=>sendCommand({command:'clear-stop'}));
$('connectButton').addEventListener('click',()=>{$('connectError').textContent='';$('connectDialog').showModal();});
$('cancelConnect').addEventListener('click',()=>$('connectDialog').close());
$('disconnectButton').addEventListener('click',disconnect);
$('connectForm').addEventListener('submit',async event=>{
  event.preventDefault();$('submitConnect').disabled=true;$('connectError').textContent='';
  try{await connect($('apiKey').value.trim());}catch(error){connected(false);$('connectError').textContent=error.message;}finally{$('submitConnect').disabled=false;}
});
$('newRecipe').addEventListener('click',()=>{if(!state.dirty || confirm('Discard the unsaved recipe changes?')){editRecipe({});$('recipeName').focus();}});
$('restoreRecipes').addEventListener('click',async()=>{
  if(!confirm('Add missing starter recipes? Recipes already on the Pico, including your edits, will be preserved.'))return;
  $('restoreRecipes').disabled=true;
  try{const result=await api('/recipes/restore-defaults','POST');await refreshRecipes();notify(result.added ? result.added+' starter recipes added. Existing recipes preserved.' : 'All starter recipes are already on the Pico.');}
  catch(error){notify(error.message,true);}finally{updateControls();}
});
$('addStep').addEventListener('click',()=>{addStep();state.dirty=true;});
$('recipeForm').addEventListener('input',()=>{state.dirty=true;});
$('recipeName').addEventListener('input',()=>{if($('recipeId').dataset.auto==='true')$('recipeId').value=$('recipeName').value.toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'').slice(0,20);});
$('recipeId').addEventListener('input',()=>{$('recipeId').dataset.auto='false';});
$('recipeForm').addEventListener('submit',async event=>{
  event.preventDefault();const recipe=recipeFromForm();if(!recipe)return;
  try{await api('/recipes/'+encodeURIComponent(recipe.id),'PUT',{name:recipe.name,steps:recipe.steps});editRecipe(recipe,true);await refreshRecipes();notify('Recipe saved on the Pico.');}
  catch(error){notify(error.message,true);}
});
$('deleteRecipe').addEventListener('click',async()=>{
  if(!state.editorId || !confirm('Delete this recipe from the Pico?'))return;
  try{await api('/recipes/'+encodeURIComponent(state.editorId),'DELETE');editRecipe({});await refreshRecipes();notify('Recipe deleted.');}
  catch(error){notify(error.message,true);}
});
$('importRecipe').addEventListener('click',()=>$('importFile').click());
$('importFile').addEventListener('change',async event=>{
  const file=event.target.files[0];event.target.value='';if(!file)return;
  try{
    if(file.size>16384)throw new Error('Recipe file is too large.');
    const recipe=JSON.parse(await file.text());
    if(!recipe || !Array.isArray(recipe.steps) || recipe.steps.length<1 || recipe.steps.length>15 || typeof recipe.name!=='string' || typeof recipe.id!=='string' || !/^[A-Za-z0-9_-]{1,20}$/.test(recipe.id) || recipe.steps.some(step=>!step || typeof step!=='object'))throw new Error('Choose a recipe JSON file exported by Pico Local.');
    if(state.dirty && !confirm('Discard the unsaved recipe changes?'))return;
    editRecipe(recipe,false);state.dirty=true;notify('Recipe imported into the editor. Review it, then save to the Pico.');
  }catch(error){notify(error instanceof SyntaxError?'The file is not valid JSON.':error.message,true);}
});
$('exportRecipe').addEventListener('click',()=>{const recipe=recipeFromForm();if(recipe)download(recipe.id+'.json',recipe);});
$('settingsForm').addEventListener('submit',async event=>{
  event.preventDefault();try{const settings=await api('/settings','PATCH',{temperature_unit:$('temperatureUnit').value,language:$('language').value,detailed_status:$('detailedStatus').checked});renderSettings(settings);if(state.status)renderStatus(state.status);notify('Preferences saved on the Pico.');}catch(error){notify(error.message,true);}
});
$('telemetryForm').addEventListener('input',()=>{
  state.telemetryDirty=true;$('telemetryHost').required=$('telemetryEnabled').checked;
});
$('telemetryForm').addEventListener('submit',async event=>{
  event.preventDefault();if(!state.telemetry || state.telemetrySaving)return;
  const generation=state.generation;
  state.telemetrySaving=true;updateControls();
  try {
    const config=await api('/telemetry','PUT',{enabled:$('telemetryEnabled').checked,host:$('telemetryHost').value.trim(),port:Number($('telemetryPort').value)});
    state.telemetryDirty=false;renderTelemetry(config);notify('Forwarding settings saved on the Pico.');
  } catch(error) {if(generation===state.generation)notify(error.message,true);}
  finally {if(generation===state.generation){state.telemetrySaving=false;updateControls();}}
});
document.addEventListener('visibilitychange',()=>{if(!document.hidden && state.key)poll();});
apiExplorer=setupApiExplorer({session:()=>({key:state.key,connected:state.connected,generation:state.generation}),enqueue:task=>{
  const result=requestQueue.catch(()=>{}).then(task);requestQueue=result;return result;
}});
brewHistory=setupHistory({session:()=>({key:state.key,connected:state.connected,generation:state.generation,temperature_unit:state.settings.temperature_unit}),enqueue:task=>{
  const result=requestQueue.catch(()=>{}).then(task);requestQueue=result;return result;
}});
editRecipe({});
try{state.key=sessionStorage.getItem('pico-api-key') || '';}catch(_){}
if(/^[0-9a-f]{64}$/.test(state.key))connect(state.key).catch(error=>{connected(false);notify(error.message,true);$('connectDialog').showModal();});
else $('connectDialog').showModal();
