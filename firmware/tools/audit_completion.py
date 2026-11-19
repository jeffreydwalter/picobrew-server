#!/usr/bin/env python3
"""Account for every indexed Pico application entry and verify concrete source mappings.

A complete entry map is a scope/accounting check. It is not proof that all
possible inputs, hardware timing or SDK implementation behavior are equivalent.
"""
import hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    reference=json.loads((ROOT/'reference/function_map.json').read_text())
    known={int(x['address'],16):x for x in reference}
    records={}
    invalid_sources=[]
    manifests=sorted((ROOT/'analysis/recovery').glob('*.json'))
    for path in manifests:
        data=json.loads(path.read_text())
        entries=list(data.get('entries',data.get('functions',[])))
        entries.extend(data.get('additional_entries',[]))
        entries.extend(data.get('dependency_replacements',[]))
        for entry in entries:
            source=entry.get('source',entry.get('replacement_file'))
            if not source or not (ROOT/source).is_file():
                invalid_sources.append({'manifest':str(path.relative_to(ROOT)),
                    'original_address':entry['original_address'],'source':source})
            address=int(entry['original_address'],16)
            ref=known.get(address,{})
            record=records.setdefault(address,{'original_address':f'0x{address:08x}',
                'original_name':ref.get('name',entry.get('original_name','Recovered unindexed entry')),
                'original_module':ref.get('module',entry.get('original_module','unindexed')),
                'indexed_application_entry':ref.get('file','').startswith('src/application/'),
                'mappings':[]})
            record['mappings'].append({'manifest':str(path.relative_to(ROOT)),**entry})
    expected={int(x['address'],16):x for x in reference if x['file'].startswith('src/application/')}
    missing=[{'address':f'0x{address:08x}','name':entry['name'],'module':entry['module']}
             for address,entry in expected.items() if address not in records]
    manifest_hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in manifests}
    result={'passed':not missing and not invalid_sources,'indexed_application_entries':len(expected),
        'accounted_application_entries':sum(a in records for a in expected),
        'additional_dependency_or_unindexed_entries':sum(a not in expected for a in records),
        'missing':missing,'invalid_source_mappings':invalid_sources,'manifest_sha256':manifest_hashes,
        'scope':'Per-original-entry source and evidence accounting; see each domain status/limitations. Mapped replacements include explicit upstream substitutions and inlined helpers, not recovered original symbol names or full path coverage.'}
    (ROOT/'analysis/completion-audit.json').write_text(json.dumps(result,indent=2)+'\n')
    recovery={'reference_sha256':hashlib.sha256((ROOT/'reference/original-0.1.34.bin').read_bytes()).hexdigest(),
        'distinct_original_entries':len(records),'entries':[records[k] for k in sorted(records)],
        'scope':result['scope']}
    (ROOT/'analysis/recovery-map.json').write_text(json.dumps(recovery,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    if missing or invalid_sources:raise SystemExit(1)
if __name__=='__main__':main()
