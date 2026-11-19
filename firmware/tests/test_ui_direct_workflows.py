#!/usr/bin/env python3
"""Direct original/rebuilt ARM comparisons for UI workflow and adapter entries.

Actual drawing runs through the original and rebuilt graphics code. User input,
network/recipe operations and actuator services are explicit observed boundaries;
the function under test is always executed directly and is never intercepted.
"""
from ui_test_support import *
from test_native_startup import Layout
import os

layout = Layout(ELF)
results, failures = [], []
FILTER = os.environ.get('PICO_UI_CASE_FILTER', '')
OWNED = {e['replacement_function']: e['original_address'] for e in
         json.loads((PROJECT/'analysis/recovery/ui.json').read_text())['entries']
         if e['replacement_file'] in ('src/ui/ui_workflows.c','src/ui/workflow_adapters.c','src/ui/ui_screens.c')
         and e['replacement_function'] in {
'pico_menu_populate_from_menu','pico_ui_wait_for_resume','pico_ui_wait_for_footer_press',
'pico_ui_prompt_deep_clean_setup','pico_ui_draw_instruction_with_header','pico_ui_draw_instruction_with_context',
'pico_ui_wait_for_filter_insertion','pico_ui_enter_wifi_password','pico_ui_select_scanned_network',
'pico_ui_choose_language','pico_ui_choose_units','pico_ui_choose_program','pico_ui_walk_program_steps',
'pico_ui_require_first_rinse','pico_ui_prompt_vacuum','pico_ui_wait_for_continue',
'pico_prompt_insert_filter_with_alarm','pico_run_drain_instructions','pico_ui_manual_network_step',
'pico_ui_prompt_picopak_association','pico_ui_show_rinse_finished','pico_ui_first_rinse_step',
'pico_ui_wait_for_stop','pico_ui_rack_beer_step','pico_ui_descale_step','pico_ui_confirm_screen',
'pico_ui_confirm_thermal_recovery','pico_ui_confirm_continue_or_abort','pico_ui_enter_manual_network',
'pico_ui_scan_and_connect_wifi','pico_ui_network_menu','pico_settings_menu','pico_ui_adjust_recipe',
'pico_ui_start_descale','pico_ui_select_recipe'}}


def offset(record, member):
    return layout.offset(record, member)


def case(name, label, run, kind='original_vs_rebuilt_arm'):
    if FILTER and FILTER not in name:
        return
    try:
        run()
        results.append({'function':name,'original_address':OWNED[name], 'case':label,'kind':kind})
    except Exception as error:
        failures.append({'function':name,'case':label,'error':str(error)})
        print('FAIL',name,label,repr(error),flush=True)


class WorkflowPair(Pair):
    def __init__(self, lang=0):
        super().__init__(lang)
        self.events={'old':[],'new':[]}
        self.service_table=self.new.SCRATCH+0x7000
        original_table=self.new.read_u32(self.ui+offset('PicoUi','services'))
        self.new.write(self.service_table,self.new.read(original_table,92))
        self.new.write_u32(self.ui+offset('PicoUi','services'),self.service_table)
        self.set_setting('wifi_ssid',b'Old Network')
        self.set_setting('wifi_password',b'Old Password')
        self.set_setting('wifi_security',2)
        self.old.write(0x2002da9c,b'\x02')
        self.new.write(self.app+offset('PicoAppState','device_state'),b'\x02')
        for field,original in self.progress_fields():
            self.old.write(original,self.new.read(self.app+offset('PicoAppState','control_signals')+offset('PicoControlSignals',field),1))
        self.menu([0]*100)

    @staticmethod
    def progress_fields():
        return [('progress_row',0x2002bf10),('progress_start_column',0x2002bf11),
                ('progress_end_column',0x2002bf12),('progress_glyph_index',0x2002bf13),
                ('progress_active',0x2002bf20)]

    def check_progress(self):
        for field,original in self.progress_fields():
            assert self.old.read(original,1)==self.new.read(self.app+offset('PicoAppState','control_signals')+offset('PicoControlSignals',field),1),field

    def hook(self, old, new, callback):
        self.old.hook(MAP[old] if isinstance(old,str) else old,lambda f:callback(f,'old'))
        self.new.hook_symbol(new,lambda f:callback(f,'new'))

    def service(self, field, old, callback):
        address=self.new.SCRATCH+0x7200+offset('PicoUiServices',field)
        self.new.write(address,b'\x70\x47')
        self.new.write_u32(self.service_table+offset('PicoUiServices',field),address|1)
        self.old.hook(MAP[old] if isinstance(old,str) else old,lambda f:callback(f,'old'))
        self.new.hook(address,lambda f:callback(f,'new'))

    def note(self, side, name, *values):
        self.events[side].append((name,*values))

    def stub(self, old, new=None, result=0, label=None):
        name=label or str(old)
        self.hook(old,new or old,lambda f,side:self.note(side,name) or result)

    def menu(self, choices):
        counters={'old':0,'new':0}
        def choose(f,side):
            index=counters[side];counters[side]+=1
            assert index<len(choices),'menu choices exhausted'
            labels=0x2002bf24 if side=='old' else self.labels
            self.note(side,'menu',f.reg(0),f.reg(1),f.reg(2),tuple(f.cstring(labels+22*i) for i in range(f.reg(1))))
            return choices[index]
        self.hook('pico_menu_choose','pico_menu_choose',choose)

    def clicks(self, values):
        counts={'old':0,'new':0}
        def click(f,side):
            index=counts[side];counts[side]+=1
            assert index<len(values),'click sequence exhausted'
            self.note(side,'click',values[index])
            return values[index]
        self.hook('pico_encoder_take_click','pico_encoder_take_click',click)

    def set_setting(self,field,value):
        addresses={'wifi_ssid':0x2002c2d0,'wifi_password':0x2002c290,'wifi_security':0x2002c2f1,
                   'language':0x2002c164,'temperature_unit_celsius':0x2002d9c9}
        address=self.settings+offset('PicoSettings',field)
        data=value+b'\0' if isinstance(value,bytes) else bytes([value])
        self.old.write(addresses[field],data);self.new.write(address,data)

    def check_settings(self):
        for field,old in [('wifi_ssid',0x2002c2d0),('wifi_password',0x2002c290)]:
            assert self.old.cstring(old)==self.new.cstring(self.settings+offset('PicoSettings',field)),field
        for field,old in [('wifi_security',0x2002c2f1),('language',0x2002c164),('temperature_unit_celsius',0x2002d9c9)]:
            assert self.old.read(old,1)==self.new.read(self.settings+offset('PicoSettings',field),1),field

    def call_direct(self,name,args=(),new_args=None,returns=False):
        # Pair's baseline convenience hooks must never intercept the tested entry.
        self.old.unhook(MAP[name]);self.new.unhook(self.new.address(name))
        a,b=self.call(name,args,new_args=new_args,max_instructions=20000000)
        if returns:assert a['r0']==b['r0'],('return',a['r0'],b['r0'])
        self.check_display();self.check_labels();self.check_settings()
        assert self.events['old']==self.events['new'],('events',self.events)
        assert self.old.read(0x2002da9c,1)==self.new.read(self.app+offset('PicoAppState','device_state'),1),'device state'
        return a['r0'],b['r0']


def simple_screen(name,arguments=(),choices=(0,),lang=0):
    p=WorkflowPair(lang);p.menu(list(choices))
    p.call_direct(name,arguments,returns=name in ('pico_ui_wait_for_continue','pico_ui_choose_program'))


# Display-producing wrappers and their observable input boundary.
for name in ['pico_ui_require_first_rinse','pico_ui_prompt_picopak_association','pico_ui_show_rinse_finished',
             'pico_ui_wait_for_continue','pico_ui_wait_for_stop','pico_ui_choose_program']:
    for lang in range(3):
        case(name,f'language={lang}',lambda name=name,lang=lang:simple_screen(name,lang=lang))

for name in ['pico_ui_draw_instruction_with_header','pico_ui_draw_instruction_with_context']:
    for screen in range(25):
        for lang in range(3):
            def run(name=name,screen=screen,lang=lang):
                p=WorkflowPair(lang)
                p.call_direct(name,[screen,lang,p.old.SCRATCH])
            case(name,f'instruction={screen},language={lang}',run)

for initial in range(3):
    for selected in [0,1,2,3,9]:
        def run(initial=initial,selected=selected):
            p=WorkflowPair(initial);p.menu([selected])
            p.call_direct('pico_ui_choose_language',returns=True)
        case('pico_ui_choose_language',f'initial={initial},selection={selected}',run)
for selected in [0,1,2,0xffffffff]:
    for lang in range(3):
        def run(selected=selected,lang=lang):
            p=WorkflowPair(lang);p.menu([selected]);p.call_direct('pico_ui_choose_units',returns=True)
        case('pico_ui_choose_units',f'language={lang},selection={selected}',run)

for name,stages in [('pico_ui_manual_network_step',range(4)),('pico_ui_first_rinse_step',range(10)),
                    ('pico_ui_rack_beer_step',range(10)),('pico_ui_descale_step',range(8))]:
    for stage in stages:
        for lang in range(3):
            def run(name=name,stage=stage,lang=lang):
                p=WorkflowPair(lang)
                p.hook('pico_ui_wait_for_footer_press','pico_ui_wait_for_footer_press',lambda f,side:p.note(side,'footer_wait',f.cstring(f.reg(0))))
                p.hook('pico_api_report_device_state','pico_api_report_state',lambda f,side:p.note(side,'report_state',f.read(0x2002da9c if side=='old' else p.app+offset('PicoAppState','device_state'),1)))
                p.hook('pico_ui_confirm_at_row','pico_ui_confirm_at_row',lambda f,side:p.note(side,'confirm_row',f.reg(0)) or 1)
                p.call_direct(name,[stage],returns=name=='pico_ui_descale_step')
            case(name,f'stage={stage},language={lang}',run)

for selected in [0,1,2]:
    for stage in [0,1,2]:
        for lang in range(3):
            def run(stage=stage,selected=selected,lang=lang):
                p=WorkflowPair(lang);p.menu([selected]);p.call_direct('pico_ui_prompt_vacuum',[stage],returns=True)
            case('pico_ui_prompt_vacuum',f'stage={stage},selection={selected},language={lang}',run)
for selected in [0,1,2,0xffffffff]:
    def run(selected=selected):
        p=WorkflowPair();p.menu([selected]);p.call_direct('pico_ui_confirm_continue_or_abort',returns=True)
    case('pico_ui_confirm_continue_or_abort',f'selection={selected}',run)

for lang in range(3):
    def run(lang=lang):
        p=WorkflowPair(lang)
        p.hook('pico_ui_wait_for_footer_press','pico_ui_wait_for_footer_press',lambda f,side:p.note(side,'footer_wait',f.cstring(f.reg(0))))
        p.call_direct('pico_ui_prompt_deep_clean_setup')
    case('pico_ui_prompt_deep_clean_setup',f'language={lang}',run)
for clicks in [[255],[0,255],[0,0,255]]:
    def run(clicks=clicks):
        p=WorkflowPair();p.clicks(clicks);p.call_direct('pico_ui_wait_for_footer_press',[p.old.SCRATCH])
        assert p.old.read(0x2002bc28+12,1)==p.new.read(p.encoder+12,1)
    case('pico_ui_wait_for_footer_press',str(clicks),run)
for clicks,filters in [([255],[1]),([0,255],[1,1]),([255,255],[0,1]),([0,255,0,255],[0,0,1,1])]:
    def run(clicks=clicks,filters=filters):
        p=WorkflowPair();p.clicks(clicks);indexes={'old':0,'new':0}
        def inserted(f,side):
            i=indexes[side];indexes[side]+=1;assert i<len(filters)
            p.note(side,'filter',filters[i]);return filters[i]
        p.hook('pico_step_filter_is_inserted','pico_filter_is_inserted',inserted)
        p.call_direct('pico_ui_wait_for_resume')
    case('pico_ui_wait_for_resume',f'clicks={clicks},filters={filters}',run)
for inserted in [0,1]:
    for lang in range(3):
        def run(inserted=inserted,lang=lang):
            p=WorkflowPair(lang)
            p.hook('pico_step_filter_is_inserted','pico_filter_is_inserted',lambda f,side:p.note(side,'filter',inserted) or inserted)
            p.service('wait_for_filter','pico_wait_for_step_filter',lambda f,side:p.note(side,'wait_for_filter'))
            p.call_direct('pico_ui_wait_for_filter_insertion')
        case('pico_ui_wait_for_filter_insertion',f'inserted={inserted},language={lang}',run)
for lang in range(3):
    def run(lang=lang):
        p=WorkflowPair(lang)
        p.hook('pico_buzzer_on','ui_fixture_buzzer',lambda f,side:p.note(side,'buzzer',True) or 1)
        p.service('alarm_filter_missing','pico_alarm_while_filter_missing',lambda f,side:p.note(side,'filter_alarm'))
        p.call_direct('pico_prompt_insert_filter_with_alarm')
    case('pico_prompt_insert_filter_with_alarm',f'language={lang}',run)


def finish():
    tested=sorted({r['function'] for r in results})
    missing=sorted(set(OWNED)-set(tested)) if not FILTER else []
    report={'elf_sha256':hashlib.sha256(ELF.read_bytes()).hexdigest(),
            'original_image_sha256':hashlib.sha256((PROJECT/'reference/original-0.1.34.bin').read_bytes()).hexdigest(),
            'passed_cases':len(results),'failed_cases':len(failures),'tested_functions':tested,
            'missing_owned_entries':missing,'failures':failures,'cases':results,
            'entry_evidence':[{'original_address':OWNED[name],'function':name,
                              'direct_cases':sum(r['function']==name for r in results)} for name in tested],
            'scope':'Direct original/rebuilt ARM entry execution. Actual framebuffers, flushed frames, menu labels, settings, returns and ordered service requests are compared; hardware/network/recipe services and user input are explicitly controlled. Not physical or exhaustive path validation.'}
    write_report(report,'unused-staging.json','ui-direct-workflows-tests.json')
    print(json.dumps({k:report[k] for k in ('passed_cases','failed_cases','missing_owned_entries','failures')},indent=2))
    if failures or missing:raise SystemExit(1)

# Additional adapter scenarios are declared below before finish().

for actions in [[],[0],[23,34],[12,13,14,15,16,17],[*range(22)]]:
    for lang in range(3):
        for status in [0,1]:
            def run(actions=actions,lang=lang,status=status):
                p=WorkflowPair(lang)
                data=struct.pack('<'+'I'*22,*((actions+[0]*22)[:22]))+struct.pack('<I',len(actions))
                for f in (p.old,p.new):f.write(f.SCRATCH,data)
                p.call_direct('pico_menu_populate_from_menu',[p.old.SCRATCH,lang,status])
                assert p.old.read(0x2002bc74,92)==p.new.read(p.app+offset('PicoAppState','menu'),92)
            case('pico_menu_populate_from_menu',f'actions={actions},language={lang},status={status}',run)

for lang in range(3):
    for password in [b'',b'secret',b'P'*63]:
        def run(lang=lang,password=password):
            p=WorkflowPair(lang)
            def edit(f,side):
                p.note(side,'edit',f.cstring(f.reg(0)),f.reg(1))
                f.write(f.reg(0),password+b'\0')
            p.hook('pico_encoder_edit_text','pico_encoder_edit_text',edit)
            p.call_direct('pico_ui_enter_wifi_password')
            assert p.new.cstring(p.settings+offset('PicoSettings','wifi_password'))==password
        case('pico_ui_enter_wifi_password',f'language={lang},length={len(password)}',run)

for selection in [0,1,2,7]:
    for lang in range(3):
        def run(selection=selection,lang=lang):
            p=WorkflowPair(lang);p.menu([selection]);edits={'old':0,'new':0}
            def edit(f,side):
                n=edits[side];edits[side]+=1
                p.note(side,'edit',f.cstring(f.reg(0)),f.reg(1))
                f.write(f.reg(0),(b'Manual Network' if n==0 else b'passphrase')+b'\0')
            p.hook('pico_encoder_edit_text','pico_encoder_edit_text',edit)
            p.call_direct('pico_ui_enter_manual_network')
        case('pico_ui_enter_manual_network',f'language={lang},security_selection={selection}',run)

for security in [2,5,7]:
    for index,ssid in [(0,b'Cafe'),(3,b'X'*32),(19,b'Last Network')]:
        def run(security=security,index=index,ssid=ssid):
            p=WorkflowPair()
            def scan_security(f,side):
                p.note(side,'security',f.reg(1));return security
            def scan_ssid(f,side):
                p.note(side,'ssid',f.reg(1));f.write(f.SCRATCH+2048,ssid+b'\0');return f.SCRATCH+2048
            def password(f,side):
                p.note(side,'password_prompt')
                address=0x2002c290 if side=='old' else p.settings+offset('PicoSettings','wifi_password')
                assert f.cstring(address)==b''
                f.write(address,b'updated-password\0')
            p.hook('WiFi_scan_result_security','fixture_security',scan_security)
            p.hook('WiFi_scan_result_ssid','fixture_ssid',scan_ssid)
            p.hook('pico_ui_enter_wifi_password','pico_ui_enter_wifi_password',password)
            p.call_direct('pico_ui_select_scanned_network',[index])
        case('pico_ui_select_scanned_network',f'index={index},security={security},ssid_length={len(ssid)}',run)

for program in [0,2,3,4]:
    for choices in [[0,0,0,0],[1],[0,1],[0,0,2],[0,0,0,3]]:
        def run(program=program,choices=choices):
            p=WorkflowPair();p.menu(choices);p.call_direct('pico_ui_walk_program_steps',[program],returns=True)
        case('pico_ui_walk_program_steps',f'program={program},choices={choices}',run)

for selected in [0,1]:
    for name in ['pico_ui_draw_descale_preparation','pico_ui_draw_deep_clean_heading']:
        def run(selected=selected,name=name):
            p=WorkflowPair();p.menu([selected])
            p.call_direct('pico_ui_confirm_screen',[MAP[name]|1],new_args=[p.new.address(name)|1],returns=True)
        case('pico_ui_confirm_screen',f'callback={name},selection={selected}',run)
for status in [0,5,6,7]:
    for selected in [0,1,2]:
        for lang in range(3):
            def run(status=status,selected=selected,lang=lang):
                p=WorkflowPair(lang);p.menu([selected])
                p.hook('pico_api_report_error','pico_api_error',lambda f,side:p.note(side,'report_error',f.reg(0 if side=='old' else 1)))
                p.service('alarm_until_press','pico_alarm_until_encoder_press',lambda f,side:p.note(side,'alarm'))
                p.call_direct('pico_ui_confirm_thermal_recovery',[status],returns=True)
            case('pico_ui_confirm_thermal_recovery',f'status={status},selection={selected},language={lang}',run)
for selected in [False,True]:
    for lang in range(3):
        def run(selected=selected,lang=lang):
            p=WorkflowPair(lang)
            def confirm(f,side):
                expected=MAP['pico_ui_draw_descale_preparation']|1 if side=='old' else p.new.address('pico_ui_draw_descale_preparation')|1
                assert f.reg(0)==expected
                p.note(side,'confirm_descale');return selected
            p.hook('pico_ui_confirm_screen','pico_ui_confirm_screen',confirm)
            p.service('run_descale','pico_run_descale_program',lambda f,side:p.note(side,'run_descale'))
            p.call_direct('pico_ui_start_descale')
        case('pico_ui_start_descale',f'confirmed={selected},language={lang}',run)

for setup_action in [23,34]:
    for initially_running,actions in [(False,[30]),(True,[30]),(False,[31,32,30]),(True,[32,31,34])]:
        def run(setup_action=setup_action,initially_running=initially_running,actions=actions):
            p=WorkflowPair();running={'old':initially_running,'new':initially_running};indexes={'old':0,'new':0}
            def sequence(f,side):
                p.note(side,'instruction_sequence',words(f.read(f.reg(0),8)),f.reg(1),f.reg(2),f.reg(3),f.cstring(f.read_u32(f.reg('sp'))),f.read_u32(f.reg('sp')+4))
                return setup_action
            def drain_status(f,side):p.note(side,'drain_status',running[side]);return running[side]
            def drain_on(f,side):p.note(side,'drain_on');running[side]=True
            def drain_off(f,side):p.note(side,'drain_off');running[side]=False
            def instruction(f,side):
                index=indexes[side];indexes[side]+=1;assert index<len(actions)
                p.note(side,'instruction',f.reg(0 if side=='old' else 1),f.reg(1 if side=='old' else 2));return actions[index]
            def read(f,side):p.note(side,'read',f.reg(0 if side=='old' else 1));return int(running[side])
            def write(f,side):
                pin,value=f.reg(0 if side=='old' else 1),f.reg(1 if side=='old' else 2)
                p.note(side,'write',pin,value);running[side]=bool(value)
            p.hook('pico_ui_run_instruction_sequence','pico_ui_run_instruction_sequence',sequence)
            p.hook('pico_drain_pump_is_on','pico_actuator_drain_is_on',drain_status)
            p.hook('pico_drain_pump_on','pico_actuator_drain_on',drain_on)
            p.hook('pico_drain_pump_off','pico_actuator_drain_off',drain_off)
            p.service('execute_instruction','pico_execute_current_program_instruction',instruction)
            p.hook('digitalRead','ui_fixture_read',read);p.hook('digitalWrite','ui_fixture_write',write)
            p.call_direct('pico_run_drain_instructions',[15],returns=True)
            assert running['old']==running['new']
        case('pico_run_drain_instructions',f'setup={setup_action},initial={initially_running},actions={actions}',run)

# Menus execute the original/rebuilt dispatch and actual redraws; child workflows
# are bounded observation points so every menu branch can be exercised separately.
for choices in [[4],[0,4],[1,4],[2,4],[3,4],[0,1,2,3,4]]:
    def run(choices=choices):
        p=WorkflowPair();p.menu(choices)
        for name in ['pico_ui_choose_language','pico_ui_choose_units','pico_ui_network_menu','pico_screen_device_information']:
            p.stub(name)
        p.stub('pico_settings_save_display','pico_storage_save_display')
        p.call_direct('pico_settings_menu')
    case('pico_settings_menu',str(choices),run)
for connected in [False,True]:
    for choices in [[3],[0,3],[1,3],[2,3],[0,1,2,3]]:
        def run(connected=connected,choices=choices):
            p=WorkflowPair();p.menu(choices)
            p.stub('WiFi_disconnect','fixture_disconnect')
            p.stub('pico_ui_scan_and_connect_wifi',result=connected)
            p.stub('pico_diagnostic_wifi');p.stub('pico_ui_show_network_addresses')
            p.stub('pico_settings_save_wifi','pico_storage_save_wifi')
            p.hook('delay','ui_fixture_delay',lambda f,side:p.note(side,'delay',f.reg(0 if side=='old' else 1)))
            p.call_direct('pico_ui_network_menu')
        case('pico_ui_network_menu',f'connected={connected},choices={choices}',run)

for initialized,init_result,count,choices,connected,resolved in [
 (False,False,0,[],False,False),(False,True,0,[],False,False),
 (True,True,0,[],False,False),(True,True,1,[0],True,True),
 (True,True,2,[0],True,False),(True,True,2,[1],False,False),
 (True,True,2,[2],True,True),(True,True,2,[3,0],True,True),
 (True,True,20,[19],True,True)]:
    for lang in range(3):
        def run(initialized=initialized,init_result=init_result,count=count,choices=choices,connected=connected,resolved=resolved,lang=lang):
            p=WorkflowPair(lang);p.menu(choices)
            p.old.write(0x2002daff,bytes([initialized]));p.new.hook_symbol('fixture_initialized',lambda f:initialized)
            p.hook('WiFi_initialize','fixture_initialize',lambda f,side:p.note(side,'wifi_initialize') or init_result)
            p.hook('WiFi_scan_networks','fixture_scan',lambda f,side:p.note(side,'scan') or count)
            def ssid(f,side):
                index=f.reg(1);p.note(side,'ssid',index)
                f.write(f.SCRATCH+2048,('Network '+str(index)).encode()+b'\0');return f.SCRATCH+2048
            p.hook('WiFi_scan_result_ssid','fixture_ssid',ssid)
            p.hook('pico_ui_select_scanned_network','pico_ui_select_scanned_network',lambda f,side:p.note(side,'selected_network',f.reg(0)))
            p.stub('pico_ui_enter_manual_network')
            p.stub('pico_wifi_connect_with_screen','pico_network_connect_with_screen',result=connected)
            p.stub('pico_wifi_resolve_server','pico_network_resolve_server',result=resolved)
            p.stub('pico_diagnostic_wifi')
            p.hook('delay','ui_fixture_delay',lambda f,side:p.note(side,'delay',f.reg(0 if side=='old' else 1)))
            p.call_direct('pico_ui_scan_and_connect_wifi',returns=True)
            p.check_progress()
        case('pico_ui_scan_and_connect_wifi',f'initialized={initialized},init={init_result},count={count},choices={choices},connected={connected},resolved={resolved},language={lang}',run)

for abv,ibu,choices,changed,fetch_result in [
 (-1,-1,[0],False,0),(-1,-1,[1],False,0),(-1,-1,[2],False,0),
 (2,4,[0],False,0),(2,4,[2],False,0),(2,-1,[1,0],False,0),
 (-1,4,[1,0],True,0),(2,4,[1,0],True,0),(2,4,[1],True,1)]:
    def run(abv=abv,ibu=ibu,choices=choices,changed=changed,fetch_result=fetch_result):
        p=WorkflowPair();p.menu(choices)
        old_recipe=0x2002bcd8;new_recipe=p.app+offset('PicoAppState','recipe')
        for f,address in [(p.old,old_recipe),(p.new,new_recipe)]:
            f.write_u32(address+offset('PicoRecipe','abv_adjustment_reference'),u32(abv))
            f.write_u32(address+offset('PicoRecipe','ibu_adjustment_reference'),u32(ibu))
            f.write(address+offset('PicoRecipe','identifier'),b'test-recipe\0')
        p.stub('pico_display_refresh_current_page')
        def adjust(f,side):
            baseline=f.reg(1 if side=='old' else 0);stored=f.reg(2 if side=='old' else 1);flag=f.reg(3 if side=='old' else 2)
            p.note(side,'adjust',baseline,f.read_u32(stored));f.write(flag,bytes([changed]));return baseline+int(changed)
        p.hook('pico_encoder_select_adjustment_level','pico_encoder_select_adjustment_level',adjust)
        def fetch(f,side):
            shift=0 if side=='old' else 1
            p.note(side,'fetch',f.cstring(f.reg(shift)),f.reg(shift+1),f.reg(shift+2));return fetch_result
        p.hook('pico_api_fetch_recipe','pico_api_fetch_recipe',fetch)
        p.call_direct('pico_ui_adjust_recipe',[old_recipe],new_args=[new_recipe],returns=True)
        assert p.old.read(old_recipe+4,0x234)==p.new.read(new_recipe+4,0x234),'recipe adjustment state'
    case('pico_ui_adjust_recipe',f'abv={abv},ibu={ibu},choices={choices},changed={changed},fetch={fetch_result}',run)

listing_scenarios = [
    {'name':'both_sources_fail','catalog':False,'associated':False,'choices':[]},
    {'name':'catalog_create_fail','catalog':True,'associated':True,'choices':[0],'create':False},
    {'name':'catalog_success','catalog':True,'associated':True,'choices':[1],'create':True},
    {'name':'catalog_fetch_fail','catalog':True,'associated':True,'choices':[0],'create':True,'fetch_failed':True},
    {'name':'catalog_then_associated_exit','catalog':True,'associated':True,'choices':[2,2]},
    {'name':'catalog_next_page','catalog':True,'associated':True,'choices':[19,1],'paged':True,'create':True},
    {'name':'associated_unique','catalog':False,'associated':True,'choices':[1]},
    {'name':'associated_group_first','catalog':False,'associated':True,'choices':[0,0]},
    {'name':'associated_group_cancel','catalog':False,'associated':True,'choices':[0,1]},
    {'name':'associated_fetch_fail','catalog':False,'associated':True,'choices':[1],'fetch_failed':True},
    {'name':'associated_exit','catalog':False,'associated':True,'choices':[2]},
]
for scenario in listing_scenarios:
    def run(scenario=scenario):
        p=WorkflowPair();p.menu(scenario['choices'])
        old_recipe=0x2002bcd8;new_recipe=p.app+offset('PicoAppState','recipe')
        for f,address in [(p.old,old_recipe),(p.new,new_recipe)]:f.write(address+4,b'existing-recipe\0')
        def list_response(f,side,associated):
            if side=='old':
                total_ptr,offset_ptr,count_ptr,ids_ptr=[f.reg(i) for i in range(4)]
                titles_ptr=f.read_u32(f.reg('sp')) if associated else None
                loaded_ptr=f.read_u32(f.reg('sp')+4) if associated else None
                menu_ptr=0x2002bf24
                page=f.read_u32(offset_ptr)
            else:
                base=f.reg(1)
                total_ptr=base+offset('PicoRecipeListing','total_items')
                offset_ptr=base+offset('PicoRecipeListing','page_offset')
                count_ptr=base+offset('PicoRecipeListing','menu_item_count')
                loaded_ptr=base+offset('PicoRecipeListing','loaded_items')
                ids_ptr=base+offset('PicoRecipeListing','identifiers')
                titles_ptr=base+offset('PicoRecipeListing','titles')
                menu_ptr=base+offset('PicoRecipeListing','menu_labels')
                page=f.read_u32(offset_ptr)
            p.note(side,'recipe_list',associated,page)
            if not scenario['associated' if associated else 'catalog']:return 0
            if associated:
                identifiers=[b'pkA',b'pkB',b'pkC'];titles=[b'Amber',b'Amber',b'Lager'];labels=[b'Amber',b'Lager'];total=3
            else:
                count=19 if scenario.get('paged') and page==0 else 2
                identifiers=[str(100+page+i).encode() for i in range(count)]
                labels=[('Recipe '+str(page+i)).encode() for i in range(count)]
                titles=labels;total=21 if scenario.get('paged') else 2
            f.write_u32(total_ptr,total);f.write_u32(count_ptr,len(labels))
            if loaded_ptr is not None:f.write_u32(loaded_ptr,len(identifiers))
            for i,text in enumerate(identifiers):f.write(ids_ptr+22*i,text+b'\0')
            if titles_ptr is not None:
                for i,text in enumerate(titles):f.write(titles_ptr+22*i,text+b'\0')
            for i,text in enumerate(labels):f.write(menu_ptr+22*i,text+b'\0')
            return 1
        p.old.hook(MAP['pico_api_get_recipe_list'],lambda f:list_response(f,'old',False))
        p.old.hook(MAP['pico_api_get_associated_picopaks'],lambda f:list_response(f,'old',True))
        p.new.hook_symbol('pico_api_recipe_list',lambda f:list_response(f,'new',bool(f.reg(2))))
        def create(f,side):
            p.note(side,'create_picopak',f.reg(0 if side=='old' else 1))
            if scenario.get('create',True):f.write((old_recipe if side=='old' else new_recipe)+4,b'created-picopak\0')
            return scenario.get('create',True)
        p.hook('pico_api_create_picopak','pico_api_create_picopak',create)
        def fetch(f,side):
            p.note(side,'default_recipe',f.cstring(f.reg(0 if side=='old' else 1)))
            return scenario.get('fetch_failed',False)
        p.service('fetch_default_recipe_failed','pico_api_fetch_default_recipe_failed',fetch)
        p.call_direct('pico_ui_select_recipe',[old_recipe],new_args=[new_recipe],returns=True)
        assert p.old.read(old_recipe+4,0x234)==p.new.read(new_recipe+4,0x234),'selected recipe state'
    case('pico_ui_select_recipe',scenario['name'],run)

for selected in [1,2]:
    def run(selected=selected):
        p=WorkflowPair();p.menu([selected]);p.call_direct('pico_ui_choose_program',returns=True)
    case('pico_ui_choose_program',f'forward_selection={selected}',run)
for lang in range(3):
    def run(lang=lang):
        p=WorkflowPair(lang)
        p.hook('pico_ui_confirm_at_row','pico_ui_confirm_at_row',lambda f,side:p.note(side,'confirm_row',f.reg(0)) or 0)
        p.call_direct('pico_ui_descale_step',[4],returns=True)
    case('pico_ui_descale_step',f'confirmation_declined,language={lang}',run)

finish()
