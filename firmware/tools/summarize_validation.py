#!/usr/bin/env python3
"""Require current source, complete image and matching test evidence before release."""
import hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def read(name):return json.loads((ROOT/name).read_text())
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def main():
    build=read('build/build-report.json');elf_hash=digest(ROOT/'build/application-validation.elf')
    require(build['validation_elf_sha256']==elf_hash,'Stale application ELF report')
    require(not build['undefined_symbols'],'Undefined application symbols')
    for group in ('source_sha256','header_sha256','test_fixture_sha256','platform_source_sha256'):
        for name,value in build[group].items():require(digest(ROOT/name)==value,'Changed build input: '+name)
    core_hash=digest(ROOT/'build/libpicobrew.a');adapter_hash=digest(ROOT/'build/libpicobrew_energia.a')
    require(core_hash==build['archive_sha256'] and adapter_hash==build['energia_adapter_sha256'],'Archive changed')
    lock_hash=digest(ROOT/'dependencies/lock.json');require(build['sdk_dependency_lock_sha256']==lock_hash,'Dependency lock changed')
    specs=[
      ('sensors','sensor_tests.json','cases','mismatch_count','rebuilt_elf_sha256'),
      ('actuators','actuator_tests.json','cases','mismatches','rebuilt_elf_sha256'),
      ('state_pid','state-pid-tests.json','total_cases','mismatches','rebuilt_elf_sha256'),
      ('control','control-tests.json','passed','failed','elf_sha256'),
      ('phases','phase-tests.json','passed','failed','elf_sha256'),
      ('device','device-tests.json','passed','failed','elf_sha256'),
      ('rfid','rfid-tests.json','passed','failed','elf_sha256'),
      ('rfid_application','rfid-application-tests.json','passed','failed','elf_sha256'),
      ('application','application-tests.json','passed','failed','elf_sha256'),
      ('network','network-tests.json','total_cases',None,'elf_sha256'),
      ('network_direct','network-direct-tests.json','total_cases',None,'elf_sha256'),
      ('network_direct_still','network-direct-still-tests.json','total_cases',None,'elf_sha256'),
      ('recipe','recipe-tests.json','differential_cases',None,'validation_elf_sha256'),
      ('ui','ui-tests.json','passed_cases','failed_cases','elf_sha256'),
      ('ui_interactions','ui-interactions-tests.json','passed_cases','failed_cases','elf_sha256'),
      ('ui_workflows','ui-workflows-tests.json','passed_cases','failed_cases','elf_sha256'),
      ('ui_direct_display','ui-direct-display-tests.json','passed_cases','failed_cases','elf_sha256'),
      ('ui_direct_menus','ui-direct-menus-tests.json','passed_cases','failed_cases','elf_sha256'),
      ('ui_direct_workflows','ui-direct-workflows-tests.json','passed_cases','failed_cases','elf_sha256')]
    suites=[]
    for name,file,count,failed,hash_key in specs:
        path='analysis/'+file;result=read(path)
        require(result[hash_key]==elf_hash,'Stale suite ELF: '+name)
        if failed:require(result[failed]==0,'Suite failure: '+name)
        if name.startswith('network'):require(result['passed'],'Network suite failed: '+name)
        total=result[count]
        if name=='recipe':total+=len(result['bounded_input_checks'])
        if name in ('ui_direct_display','network_direct_still'):total+=result['repair_scenarios']
        suites.append({'name':name,'scenarios':total,'failures':0,'report':path,
            'report_sha256':digest(ROOT/path),
            'scope':result.get('scope',result.get('boundary_contract',result.get('limitations',
                'Original/rebuilt ARM states and ordered observable effects; domain report records exact cases.')))})
    native=read('build/firmware/native/validation.json');firmware=read('build/firmware/firmware-report.json')
    require(native['pass'] and not native['undefined_symbols'] and not native['missing_application_symbols'],'Native link failed')
    require(native['dependency_lock_sha256']==lock_hash,'Native dependency lock differs')
    require({x['sha256'] for x in native['selected_archives']}=={core_hash,adapter_hash},'Native link has different archives')
    require(firmware['image_layout']['passed'],'Firmware layout failed')
    require(firmware['elf_sha256']==digest(ROOT/'build/firmware/pico-pro-recovered.elf'),'Firmware ELF changed')
    require(firmware['binary_sha256']==digest(ROOT/'build/firmware/pico-pro-recovered.bin'),'Firmware binary changed')
    require(firmware['update_body_sha256']==digest(ROOT/'build/firmware/pico-pro-recovered.update.txt'),'Update body changed')
    for name,value in firmware['app_sources_sha256'].items():require(digest(ROOT/name)==value,'Firmware source changed: '+name)
    sockets=read('analysis/native-network-tests.json')
    require(sockets['passed'] and sockets['elf_sha256']==firmware['elf_sha256'],'Native socket evidence stale/failed')
    startup=read('analysis/native-startup-tests.json')
    require(startup['passed'] and startup['production_elf_sha256']==firmware['elf_sha256'] and
        startup['test_sha256']==digest(ROOT/'tests/test_native_startup.py'),'Native startup evidence stale/failed')
    native_direct=read('analysis/native-network-direct-tests.json')
    require(native_direct['passed'] and native_direct['production_elf_sha256']==firmware['elf_sha256'] and
        native_direct['test_sha256']==digest(ROOT/'tests/test_native_network_direct.py'),'Native boundary evidence stale/failed')
    allocator=read('analysis/native-allocator-tests.json')
    require(allocator['passed'] and allocator['production_elf_sha256']==firmware['elf_sha256'] and
        allocator['test_sha256']==digest(ROOT/'tests/test_native_allocator.py'),'Native allocator evidence stale/failed')
    serial=read('analysis/native-serial-tests.json')
    require(serial['passed'] and serial['elf_sha256']==firmware['elf_sha256'] and
        serial['test_sha256']==digest(ROOT/'tests/test_native_serial.py'),'Native serial evidence stale/failed')
    assets=read('analysis/ui-assets-tests.json')
    require(assets['failed_cases']==0 and assets['elf_sha256']==elf_hash,'UI asset evidence stale/failed')
    package=read('analysis/update-package-tests.json')
    require(package['passed'] and package['application_elf_sha256']==elf_hash and
        package['firmware_binary_sha256']==firmware['binary_sha256'] and
        package['update_body_sha256']==firmware['update_body_sha256'],'Update package evidence stale/failed')
    require(read('analysis/board-pin-validation.json')['passed'],'Board pins differ')
    edit=read('analysis/edit-rebuild-tests.json')
    require(edit['passed'] and edit['baseline_elf_sha256']==elf_hash,'Edit/rebuild evidence stale/failed')
    require(edit['original_source_sha256']==digest(ROOT/'src/sensors.c'),'Sensor edit baseline differs')
    completion=read('analysis/completion-audit.json');require(completion['passed'] and not completion['missing'],'Unmapped application entries')
    for name,value in completion['manifest_sha256'].items():require(digest(ROOT/name)==value,'Completion manifest changed')
    files=[p for directory in ('src','include','platform') for p in sorted((ROOT/directory).rglob('*'))
           if p.suffix in ('.c','.cpp','.h','.inc')]
    findings=[]
    for path in files:
        contents=re.sub(r'/\*.*?\*/|//[^\n]*','',path.read_text(),flags=re.S)
        for pattern in (r'0x200[0-9a-fA-F]{5}',r'PICO_UNRESOLVED',r'FUN_[0-9a-f]{8}',r'DAT_[0-9a-f]{8}'):
            if re.search(pattern,contents):findings.append({'file':str(path.relative_to(ROOT)),'pattern':pattern})
    source_check={'passed':not findings,'production_files':len(files),'findings':findings,
       'scope':'No old application RAM/code address calls, unresolved-return macros or raw decompiler labels in production C/C++ after removing comments. Explicit TI peripheral addresses are allowed.'}
    (ROOT/'analysis/production-source-check.json').write_text(json.dumps(source_check,indent=2)+'\n')
    require(not findings,'Production source still depends on old image/decompiler addresses')
    summary={'passed':True,'full_firmware_linked':True,'hardware_tested':False,
       'reference_sha256':digest(ROOT/'reference/original-0.1.34.bin'),
       'indexed_application_entries':completion['indexed_application_entries'],
       'accounted_application_entries':completion['accounted_application_entries'],
       'additional_dependency_or_unindexed_entries':completion['additional_dependency_or_unindexed_entries'],
       'production_c_modules':len(list((ROOT/'src').rglob('*.c'))),
       'production_cpp_units':len(list((ROOT/'platform').glob('*.cpp'))),
       'compiler':build['compiler'],'runtime':'Compiler-provided newlib-nano stdio, full-newlib allocation family, bounded SDK heap adapter, libm/libgcc/libnosys and pinned Energia/TI SDK',
       'application_elf_sha256':elf_hash,'validated_scenarios':sum(x['scenarios'] for x in suites)+sockets['cases']+native_direct['cases']+startup['total_cases']+allocator['total_cases']+serial['cases']+package['cases'],
       'failures':0,'suites':suites,'native_socket_scenarios':sockets['cases'],
       'full_image_update_receiver_scenarios':package['cases'],
       'native_startup_scenarios':startup['total_cases'],
       'native_startup_assertions':startup['total_checks'],
       'native_boundary_scenarios':native_direct['cases'],
       'native_allocator_scenarios':allocator['total_cases'],
       'native_allocator_assertions':allocator['total_checks'],
       'native_allocator_calls':allocator['native_calls'],
       'native_serial_scenarios':serial['cases'],
       'immutable_asset_checks':assets['passed_cases'],
       'immutable_asset_arrays':assets['native_arrays'],
       'immutable_asset_bytes':assets['native_bytes'],
       'edit_rebuild_checks':edit['temperature_comparisons']+edit['unchanged_thermoblock_comparisons'],
       'firmware':{'binary_bytes':firmware['image_layout']['binary_bytes'],
                   'binary_sha256':firmware['binary_sha256'],'elf_sha256':firmware['elf_sha256'],
                   'layout':firmware['image_layout']},
       'sdk':{'upstream_sources_compiled':native['upstream_sources_compiled'],
              'retained_public_definitions':len(native['retained_application_symbols']),
              'undefined_symbols':[],'upstream_warnings':native['warnings']},
       'production_source_check':source_check,
       'test_source_sha256':{str(p.relative_to(ROOT)):digest(p) for p in (ROOT/'tests').glob('*') if p.suffix in ('.py','.json','.c')},
       'limits':'Counts combine exact original/rebuilt ARM comparisons, documented bounded-input regressions and concrete integration checks. Complete entry accounting is not exhaustive path coverage. No physical device, real radio/flash, interrupt race, full brew, installation or recovery test was performed.'}
    (ROOT/'analysis/validation-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps({k:summary[k] for k in ('passed','accounted_application_entries','production_c_modules','validated_scenarios','edit_rebuild_checks','full_firmware_linked','hardware_tested')},indent=2))
if __name__=='__main__':main()
