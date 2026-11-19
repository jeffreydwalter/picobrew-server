from ui_test_support import *
results=[];failures=[]
def case(name,label,run):
 try:run();results.append({'function':name,'case':label})
 except Exception as e:
  failures.append({'function':name,'case':label,'error':str(e)});print('FAIL',name,label,str(e),flush=True)
# Native graphics including transformations, clipping, all font characters.
rng=random.Random(771234)
for rotation in range(4):
 for i in range(20):
  x=rng.randint(-5,135);y=rng.randint(-5,135);color=rng.randrange(3)
  def run(rotation=rotation,x=x,y=y,color=color):
   p=Pair();p.old.call(MAP['GFX_set_rotation'],[0x2002bc00,rotation]);p.new.call_symbol('pico_graphics_rotation',[p.display,rotation]);initial=bytes(rng.randrange(256) for _ in range(1024));p.old.write(0x2002af60,initial);p.new.write(p.fb,initial)
   p.old.call(MAP['oled_draw_pixel'],[0x2002bc00,u32(x),u32(y),color]);p.new.call_symbol('pico_graphics_pixel',[p.display,u32(x),u32(y),color]);p.check_display()
  case('oled_draw_pixel',f'rotation={rotation},x={x},y={y},color={color}',run)
 for text in ['Hello, Pico!', 'A\nB\rC','A'*22, ''.join(chr(i) for i in range(1,256))]:
  for scale in [1,2]:
   def run(rotation=rotation,text=text,scale=scale):
    p=Pair();p.old.call(MAP['GFX_set_rotation'],[0x2002bc00,rotation]);p.new.call_symbol('pico_graphics_rotation',[p.display,rotation]);p.old.call(MAP['GFX_set_text_size'],[0x2002bc00,scale]);p.new.call_symbol('pico_graphics_text_scale',[p.display,scale]);data=text.encode('latin1')+b'\0';p.old.write(p.old.SCRATCH,data);p.new.write(p.new.SCRATCH,data);p.call('pico_display_text_at_pixels',[3,5,p.old.SCRATCH]);p.check_display()
   case('GFX_draw_character',f'rotation={rotation},scale={scale},textlen={len(text)}',run)
 for geom,new_name,args,stack in [('GFX_draw_line','pico_graphics_line',[3,4,123],[51,1]),('GFX_fill_rectangle','pico_graphics_fill_rectangle',[-4,7,78],[51,1]),('GFX_draw_round_rectangle','pico_graphics_round_rectangle',[0,11,128],[43,20,1]),('GFX_fill_triangle','pico_graphics_fill_triangle',[40,31,34],[31,37,37,1])]:
  def run(rotation=rotation,geom=geom,new_name=new_name,args=args,stack=stack):
   p=Pair();p.old.call(MAP['GFX_set_rotation'],[0x2002bc00,rotation]);p.new.call_symbol('pico_graphics_rotation',[p.display,rotation]);p.old.call(MAP[geom],[0x2002bc00,*map(u32,args)],stack_args=stack);p.new.call_symbol(new_name,[p.display,*map(u32,args)],stack_args=stack);p.check_display()
  case(geom,f'rotation={rotation}',run)
# Static/localized views exercise the actual native graphics above, not mocked text.
modules={'screens','ui_screens','translations','menus'}
records=json.loads((Path(__file__).resolve().parent/'ui_original_inventory.json').read_text())
exclude={'pico_screen_get_instruction_lines','pico_screen_device_information','pico_screen_still_setup_step','pico_menu_initialize','pico_menu_action_at','pico_menu_add_action','pico_menu_set_actions','pico_menu_populate_action_labels','pico_menu_populate_from_menu','pico_menu_populate_in_brew_options','pico_menu_append_wifi_search_actions','pico_ui_draw_circulation_status','pico_title_label','pico_brew_display_state_label','pico_ui_draw_instruction_screen','pico_ui_draw_instruction_with_header','pico_ui_draw_instruction_with_context'}
for record in records:
 name=record['name']
 if record['module'] not in modules or name in exclude or name not in RebuiltArm(ELF).symbols:continue
 signature=record['signature']
 for lang in range(3):
  def run(name=name,lang=lang,signature=signature):
   p=Pair(lang);args=[]
   if name.startswith('pico_label_'):
    args=[p.old.SCRATCH]+([4] if name=='pico_label_compartment' else [])
   elif name=='pico_ui_draw_error':args=[13]
   elif name=='pico_ui_show_wifi_test_result':args=[3]
   elif name=='pico_menu_set_load_more_label':args=[2]
   elif name=='pico_menu_populate_still_actions':args=[0]
   elif name=='pico_ui_draw_separator':args=[2,1,2]
   elif name=='pico_ui_draw_header_at_row':args=[p.old.SCRATCH,1]
   elif name=='pico_ui_draw_footer_option':args=[p.old.SCRATCH,1]
   elif name=='pico_ui_draw_recipe_choices':args=[p.old.SCRATCH,1]
   elif 'char *' in signature:args=[p.old.SCRATCH]
   elif '(void)' not in signature:args=[1]
   r1,r2=p.call(name,args)
   if name.startswith('pico_label_'):assert p.old.cstring(p.old.SCRATCH)==p.new.cstring(p.new.SCRATCH),'label output mismatch'
   p.check_display();p.check_labels()
  case(name,f'language={lang}',run)
# Instruction screen IDs and languages, both title modes.
for sid in range(25):
 for language in range(3):
  for context in [0,1]:
   def run(sid=sid,language=language,context=context):
    p=Pair(language);p.call('pico_ui_draw_instruction_screen',[sid,language,p.old.SCRATCH,context]);p.check_display()
   case('pico_ui_draw_instruction_screen',f'id={sid},language={language},context={context}',run)
# Native large digits, conversion/truncation and time layouts.
for name,values in [('pico_display_large_temperature',[0.0,9.9,32.0,99.9,153.7,212.0,999.0]),('pico_display_large_timer',[0,1,59,60,125,599]),('pico_display_temperature_columns',[0.0,32.0,154.6]),('pico_display_temperature_summary',[0.0,32.0,154.6])]:
 for celsius in [0,1]:
  for value in values:
   def run(name=name,celsius=celsius,value=value):
    p=Pair(celsius=celsius)
    if name=='pico_display_large_temperature':p.call(name,[*words_from_double(value),1])
    elif name=='pico_display_large_timer':p.call(name,[value,7,1])
    elif name=='pico_display_temperature_columns':p.call(name,[3,0,*words_from_double(value)],[*words_from_double(187.4),*words_from_double(154.5)])
    else:p.call(name,[*words_from_double(value),*words_from_double(187.4)],[*words_from_double(154.5),7])
    p.check_display()
   case(name,f'celsius={celsius},value={value}',run)
report={'elf_sha256':hashlib.sha256(ELF.read_bytes()).hexdigest(),'passed_cases':len(results),'failed_cases':len(failures),'tested_functions':sorted(set(x['function'] for x in results)),'failures':failures,'cases':results}
write_report(report,'differential-ui.json','ui-tests.json')
print(json.dumps({k:report[k] for k in ['passed_cases','failed_cases','tested_functions','failures']},indent=2))
if failures:sys.exit(1)
