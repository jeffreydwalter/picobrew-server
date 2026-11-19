#!/usr/bin/env python3
"""Generate the embedded API explorer catalog from the OpenAPI contract."""
import argparse
import gzip
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BEGIN = '// BEGIN GENERATED API CATALOG'
END = '// END GENERATED API CATALOG'
METHODS = {'get', 'post', 'put', 'patch', 'delete', 'head', 'options'}


def catalog(spec):
    def resolve(schema):
        while '$ref' in schema:
            schema = spec['components']['schemas'][schema['$ref'].rsplit('/', 1)[1]]
        return schema

    def sample(schema):
        schema = resolve(schema)
        if 'example' in schema:
            return schema['example']
        if 'default' in schema:
            return schema['default']
        if 'enum' in schema:
            return schema['enum'][0]
        if 'oneOf' in schema:
            return sample(schema['oneOf'][0])
        kind = schema.get('type')
        if kind == 'object':
            properties = schema['properties']
            fields = schema.get('required') or list(properties)[:schema.get('minProperties', 0)]
            return {field: sample(properties[field]) for field in fields}
        if kind == 'array':
            return [sample(schema['items']) for _ in range(schema.get('minItems', 1))]
        if kind in ('integer', 'number'):
            return schema.get('minimum', 0)
        if kind == 'boolean':
            return schema.get('default', False)
        if kind == 'string':
            return schema.get('default', 'example')
        raise ValueError(f'Cannot generate a request example: {schema}')

    operations = []
    for path, entry in spec['paths'].items():
        assert path.startswith('/api/v1') and '?' not in path
        parameters = entry.get('parameters', [])
        for method, operation in entry.items():
            if method not in METHODS:
                continue
            specs = []
            template = path
            for parameter in parameters + operation.get('parameters', []):
                assert parameter['in'] in {'path', 'query'}
                schema = resolve(parameter['schema'])
                assert schema['type'] in {'string', 'integer'}
                constraints = {key: schema[key] for key in
                               ('type', 'minLength', 'maxLength', 'pattern', 'minimum', 'maximum', 'default')
                               if key in schema}
                specs.append([parameter['name'], parameter['in'], parameter.get('required', False), constraints])
                if parameter['in'] == 'path':
                    template = template.replace('{'+parameter['name']+'}', '')
            assert '{' not in template
            row = [method.upper(), path, operation['summary']]
            if 'requestBody' in operation:
                schema = resolve(operation['requestBody']['content']['application/json']['schema'])
                mapping = schema.get('discriminator', {}).get('mapping')
                if mapping:
                    examples = []
                    # Stop is an explicit, bounded initial command example.
                    for command in sorted(mapping, key=lambda value: (value != 'stop', value)):
                        value = sample({'$ref': mapping[command]})
                        value['command'] = command
                        if command == 'encoder':
                            value['delta'] = 1
                        examples.append([command, value])
                else:
                    examples = [['Example', sample(schema)]]
                row.append(examples)
            if specs:
                if len(row) == 3:
                    row.append(None)
                row.append(specs)
            operations.append(row)
    operations.sort(key=lambda row: row[1] != '/api/v1/health')
    return operations


def generate(text):
    spec = json.loads((ROOT / 'docs/openapi.json').read_text())
    operations = catalog(spec)
    compact = lambda value: json.dumps(value, ensure_ascii=False, separators=(',', ':'))
    block = f'{BEGIN}\nconst apiOperations={compact(operations)};\n{END}'
    assert text.count(BEGIN) == text.count(END) == 1
    start, stop = text.index(BEGIN), text.index(END) + len(END)
    return text[:start] + block + text[stop:], len(operations)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    path = ROOT / 'web/api-explorer.js'
    previous = path.read_text()
    result, count = generate(previous)
    if args.check and previous != result:
        parser.error('API explorer catalog is stale; run tools/generate_api_explorer.py')
    if not args.check and previous != result:
        path.write_text(result)
    packed = gzip.compress(result.encode(), compresslevel=9, mtime=0)
    print(f'API explorer: {count} operations; {len(result.encode())} bytes; {len(packed)} gzip bytes')


if __name__ == '__main__':
    main()
