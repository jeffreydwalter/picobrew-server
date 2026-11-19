#!/usr/bin/env python3
"""Direct original/rebuilt ARM checks for the remaining display-side entries.

Draws through the real reconstructed and original rasterizers. OLED flushes,
clocks and SDK MAC retrieval are explicit boundaries; returned strings,
assertions/logging, mutable state, complete framebuffers and flush sequences
are compared. Removed SDK field thunks are compared with owned public fields.
"""
from collections import Counter
from ui_test_support import *
from unicorn import UC_HOOK_CODE

results = []
failures = []
ledger = json.loads((PROJECT / 'analysis/recovery/ui.json').read_text())
entries = {int(e['original_address'], 16): e for group in
           ('entries', 'additional_entries', 'dependency_replacements')
           for e in ledger[group]}
by_name = {}
for address, entry in entries.items():
    if entry.get('replacement_function'):
        by_name.setdefault(entry['replacement_function'], address)


def case(name, label, run, address=None, method='direct_original_vs_rebuilt_arm'):
    address = address or by_name[name]
    try:
        run()
        results.append({'original_address': f'0x{address:08x}', 'function': name,
                        'case': label, 'method': method})
    except Exception as error:
        failures.append({'original_address': f'0x{address:08x}', 'function': name,
                         'case': label, 'error': str(error)})
        print('FAIL', name, label, str(error), flush=True)


def paint(p, seed=17):
    data = bytes((i * 37 + seed) & 255 for i in range(1024))
    p.old.write(0x2002af60, data)
    p.new.write(p.fb, data)
    return data


def diagnostics(p):
    events = {'old': [], 'new': []}
    for side, arm in [('old', p.old), ('new', p.new)]:
        shift = side == 'new'
        def assertion(f, side=side, shift=shift):
            events[side].append(('assert', f.reg(shift), f.cstring(f.reg(shift + 1))))
        def log(f, side=side, shift=shift):
            events[side].append(('log', *(f.cstring(f.reg(shift + i)) for i in range(3))))
        arm.hook(MAP['pico_assert_nonfatal'] if not shift else arm.address('pico_network_assert'), assertion)
        arm.hook(MAP['pico_log_serial'] if not shift else arm.address('pico_network_log'), log)
    return events


def check_events(events):
    assert events['old'] == events['new'], ('diagnostic traces', events)


def graphics_state(p):
    old = p.old.read(0x2002bc0c, 16)
    new = p.new.read(p.display, 16)
    assert old == new, ('graphics fields', old.hex(), new.hex())


def set_scale(p, scale):
    p.old.call(MAP['GFX_set_text_size'], [0x2002bc00, scale])
    p.new.call_symbol('pico_graphics_text_scale', [p.display, scale])


def set_rotation(p, rotation):
    p.old.call(MAP['GFX_set_rotation'], [0x2002bc00, rotation])
    p.new.call_symbol('pico_graphics_rotation', [p.display, rotation])


def call_display(name, args=(), stack=(), *, scale=1, language=0, celsius=0, patterned=False):
    p = Pair(language, celsius)
    if patterned:
        paint(p)
    set_scale(p, scale)
    p.call(name, list(map(u32, args)), list(map(u32, stack)))
    p.check_display()
    graphics_state(p)


# Every action/title/state enum value, plus the actual fallback branches.
for name, values in [('pico_action_name', [*range(50), 51]),
                     ('pico_title_label', [*range(5), 6, 7]),
                     ('pico_brew_display_state_label', range(6))]:
    for value in [*values, 0xffffffff]:
        for language in ([0] if name == 'pico_action_name' else [0, 1, 2, 3, 255]):
            def run(name=name, value=value, language=language):
                p = Pair(); events = diagnostics(p)
                args = [value] if name == 'pico_action_name' else [value, language]
                a, b = p.call(name, args)
                assert p.old.cstring(a['r0']) == p.new.cstring(b['r0'])
                check_events(events)
            case(name, f'value={value},language={language}', run)
for action in [*range(50), 51, 0xffffffff]:
    for language in [0, 1, 2, 3]:
        for selector in ([0, 1, 255] if action == 9 else [0]):
            def run(action=action, language=language, selector=selector):
                p = Pair(); events = diagnostics(p)
                a, b = p.call('pico_action_label', [action, language, selector])
                assert p.old.cstring(a['r0']) == p.new.cstring(b['r0'])
                check_events(events)
            case('pico_action_label', f'{action},{language},{selector}', run)

for name in ['pico_display_clear', 'pico_display_clear_thunk']:
    for seed in [0, 123, 255]:
        def run(name=name, seed=seed):
            p = Pair(); paint(p, seed); p.call(name); p.check_display()
            assert p.new.read(p.fb, 1024) == bytes(1024)
        case(name, f'pattern={seed}', run)

for seed in [0, 51, 255]:
    def construct(seed=seed, base=False):
        p = Pair(); expected_fb = paint(p, seed)
        p.old.write(0x2002bc00, bytes([seed]) * 38)
        p.new.write(p.display, bytes([seed]) * 19)
        if base:
            p.old.call(0x20013ca0, [0x2002bc00, 128, 64])
            p.new.call_symbol('pico_display_construct')
        else:
            p.call('pico_display_construct')
            assert p.old.read(0x2002bc22, 1) == p.new.read(p.display + 16, 1)
            assert p.old.read(0x2002bc24, 2) == p.new.read(p.display + 17, 2)
        graphics_state(p)
        assert p.old.read(0x2002af60, 1024) == p.new.read(p.fb, 1024) == expected_fb
    case('pico_display_construct', f'initial_bytes={seed}', construct)
    case('GFX_construct', f'fixed_pico_geometry,initial_bytes={seed}',
         lambda seed=seed: construct(seed, True), address=0x20013ca0,
         method='direct_original_vs_owned_fixed_geometry_constructor')

for seed in [0, 17, 255]:
    def reset(seed=seed):
        p = Pair(); data = bytes((seed + i * 7) & 255 for i in range(44))
        p.old.write(0x2002bc28, data); p.new.write(p.encoder, data)
        p.call('pico_encoder_reset_state')
        assert p.old.read(0x2002bc28, 44) == p.new.read(p.encoder, 44)
    case('pico_encoder_reset_state', f'initial_pattern={seed}', reset)

# Original CString operations keep their own warning/truncation semantics.
for name in ['pico_screen_set_recipe_name', 'pico_screen_set_step']:
    for text in [b'', b'Heat water', b'x' * 31, b'y' * 32, b'z' * 100] + ([None] if name.endswith('recipe_name') else []):
        def set_screen(name=name, text=text):
            p = Pair(); events = diagnostics(p); obj = p.old.SCRATCH + 256
            initial = bytes((i * 13 + 9) & 255 for i in range(80))
            for arm in [p.old, p.new]:
                arm.write(obj, initial)
                if text is not None: arm.write(arm.SCRATCH, text + b'\0')
            pointer = p.old.SCRATCH if text is not None else 0
            args = [obj, pointer] if name.endswith('recipe_name') else [obj, 7, pointer]
            p.call(name, args)
            assert p.old.read(obj, 80) == p.new.read(obj, 80)
            check_events(events)
        case(name, 'null' if text is None else f'length={len(text)}', set_screen)

for count, initial_count in [(-1, 0), (0, 0), (1, 0), (2, 2), (6, 0), (8, 0), (3, 5), (2, 6)]:
    def add_lines(count=count, initial_count=initial_count):
        p = Pair(); events = diagnostics(p); obj = p.old.SCRATCH + 512
        ptrs = [p.old.SCRATCH + i * 32 for i in range(max(count, 6))]
        for arm in [p.old, p.new]:
            arm.write(obj, struct.pack('<7I', initial_count, *ptrs[:6]) + b'SENTINEL')
            for i, pointer in enumerate(ptrs): arm.write(pointer, f'Line {i}'.encode() + b'\0')
        args = [obj, u32(count), *ptrs[:2]]
        p.call('pico_screen_add_lines', args, ptrs[2:max(count, 0)])
        assert p.old.read(obj, 36) == p.new.read(obj, 36)
        check_events(events)
    case('pico_screen_add_lines', f'count={count},existing={initial_count}', add_lines)

for screen_id in [*range(25), 25, 0xffffffff]:
    for language in [0, 1, 2, 3, 255]:
        def instruction(screen_id=screen_id, language=language):
            p = Pair(); events = diagnostics(p); obj = p.old.SCRATCH + 512
            for arm in [p.old, p.new]:arm.write(obj, bytes(28) + b'SENTINEL')
            p.call('pico_screen_get_instruction_lines', [obj, screen_id, language])
            count = p.old.read_u32(obj)
            assert count == p.new.read_u32(obj) <= 6
            before = [p.old.cstring(p.old.read_u32(obj + 4 + 4 * i)) for i in range(count)]
            after = [p.new.cstring(p.new.read_u32(obj + 4 + 4 * i)) for i in range(count)]
            assert before == after
            assert p.old.read(obj + 28, 8) == p.new.read(obj + 28, 8) == b'SENTINEL'
            check_events(events)
        case('pico_screen_get_instruction_lines', f'id={screen_id},language={language}', instruction)

# Character and text layout directly exercises each original wrapper.
for name in ['pico_display_text_at_cell', 'pico_display_string_at_cell', 'pico_display_text_at_pixels']:
    for scale in [1, 2]:
        for position in [(0, 0), (3, 5), (-1, -2), (7, 20)]:
            for text in [b'A\nB\rC', b'Pico 0123456789', b'x' * 25]:
                def text_at(name=name, scale=scale, position=position, text=text):
                    p = Pair(); set_scale(p, scale)
                    for arm in [p.old, p.new]:arm.write(arm.SCRATCH, text + b'\0')
                    p.call(name, [*map(u32, position), p.old.SCRATCH]); p.check_display(); graphics_state(p)
                case(name, f'scale={scale},position={position},length={len(text)}', text_at)
for name in ['pico_display_center_text', 'pico_display_center_message']:
    for scale in [1, 2, 3]:
        for text in [b'', b'Hello Pico', b'T' * 21, b'W' * 30]:
            def center(name=name, scale=scale, text=text):
                p = Pair(); set_scale(p, scale)
                for arm in [p.old, p.new]:arm.write(arm.SCRATCH, text + b'\0')
                p.call(name, [3, p.old.SCRATCH] if name.endswith('center_text') else [p.old.SCRATCH])
                p.check_display(); graphics_state(p)
            case(name, f'scale={scale},length={len(text)}', center)
for scale in [1, 2]:
    for width in [0, 2, 8, 21]:
        for padding in [ord(' '), ord('0')]:
            def right(scale=scale, width=width, padding=padding):
                p = Pair(); set_scale(p, scale)
                for arm in [p.old, p.new]:arm.write(arm.SCRATCH, b'42\0')
                p.call('pico_display_right_aligned_text', [3, 1, p.old.SCRATCH, width], [padding])
                p.check_display(); graphics_state(p)
            case('pico_display_right_aligned_text', f'scale={scale},width={width},padding={padding}', right)
for name in ['pico_display_character_on_row', 'pico_display_character_at_pixels']:
    for x, y, character in [(0, 0, 65), (-4, 3, 175), (121, 7, 176), (4, 2, 10), (4, 2, 13)]:
        case(name, f'{x},{y},{character}', lambda name=name,x=x,y=y,character=character:
             call_display(name, [x, y, character]))

for name in ['pico_copy_firmware_version', 'pico_append_program_state_label', 'pico_append_program_name']:
    for prefix in [b'', b'Prefix:', b'x' * 64]:
        def strings(name=name, prefix=prefix):
            p = Pair()
            for arm in [p.old, p.new]:arm.write(arm.SCRATCH, (prefix + b'\0').ljust(128, b'!'))
            p.call(name, [p.old.SCRATCH])
            assert p.old.read(p.old.SCRATCH, 128) == p.new.read(p.new.SCRATCH, 128)
        case(name, f'prefix_length={len(prefix)}', strings)

for name in ['pico_display_device_bitmap', 'pico_display_logo', 'pico_display_draw_content_frame',
             'pico_display_draw_titled_frame', 'pico_display_draw_program_frame',
             'pico_display_device_bitmap_variant_a', 'pico_display_device_bitmap_variant_b']:
    for patterned in [False, True]:
        args = [0x20080000] if name == 'pico_display_draw_titled_frame' else []
        case(name, f'patterned={patterned}', lambda name=name,args=args,patterned=patterned:
             call_display(name, args, patterned=patterned))
for y in [-4, 0, 21, 63]:
    for width, height in [(8, 8), (13, 9), (128, 16)]:
        def centered_bitmap(y=y, width=width, height=height):
            p = Pair(); addr = p.old.SCRATCH + 256; pixels = p.old.SCRATCH + 512
            image = bytes((i * 31 + 11) & 255 for i in range(((width + 7) // 8) * height))
            for arm in [p.old, p.new]:
                arm.write(addr, struct.pack('<III', pixels, width, height)); arm.write(pixels, image)
            p.call('pico_display_center_bitmap', [u32(y), addr]); p.check_display()
        case('pico_display_center_bitmap', f'y={y},size={width}x{height}', centered_bitmap)
for digit in [-5, -1, *range(12), 255]:
    def digits(digit=digit):
        p = Pair(); a, b = p.call('pico_display_digit_bitmap', [u32(digit)])
        assert p.old.read(a['r0'], 32) == p.new.read(b['r0'], 32)
    case('pico_display_digit_bitmap', str(digit), digits)

for name in ['pico_display_clear_row', 'pico_display_clear_row_from_column', 'pico_display_clear_rows']:
    for scale in [1, 2, 3]:
        for first, second in [(0, 0), (3, 6), (7, 21), (8, 9)]:
            args = [first] if name.endswith('clear_row') else [first, second]
            case(name, f'scale={scale},args={args}', lambda name=name,args=args,scale=scale:
                 call_display(name, args, scale=scale, patterned=True))
for position in [*range(9), 255]:
    case('pico_display_valve_position', str(position),
         lambda position=position: call_display('pico_display_valve_position', [3, position], patterned=True))
for style in [0, 1, 2, 3, 0xffffffff]:
    for index in [0, 5, 9, 10, 255]:
        case('pico_display_adjustment_box', f'{style},{index}',
             lambda style=style,index=index: call_display('pico_display_adjustment_box', [style, index], patterned=True))
for started, duration, now in [(1000, 0, 1000), (1000, 0, 61000), (1000, 0, 3599999),
                             (1000, 60, 1000), (1000, 60, 60999), (1000, 60, 61000),
                             (1000, 60, 61001), (0xfffffff0, 0, 2000), (0xfffffff0, 60, 32)]:
    def timer(started=started, duration=duration, now=now):
        p = Pair(); paint(p)
        p.old.hook(MAP['millis'], lambda f: now)
        p.new.hook_symbol('ui_fixture_millis', lambda f: now)
        p.call('pico_display_update_timer', [3, started, duration]); p.check_display(); graphics_state(p)
    case('pico_display_update_timer', f'{started},{duration},{now}', timer)
for minutes, seconds in [(0, 0), (1, 9), (59, 59), (60, 0), (599, 59), (600, 0xffffffff)]:
    case('pico_display_large_timer_with_seconds', f'{minutes},{seconds}',
         lambda minutes=minutes,seconds=seconds: call_display('pico_display_large_timer_with_seconds', [minutes, seconds]))
for celsius in [0, 1]:
    for value in [-10.5, 0.0, 9.9, 32.0, 99.9, 154.6, 212.0]:
        case('pico_display_large_temperature_with_decimal', f'{value},celsius={celsius}',
             lambda value=value,celsius=celsius: call_display('pico_display_large_temperature_with_decimal', words_from_double(value), celsius=celsius))
        def target(value=value, celsius=celsius):
            p = Pair(celsius=celsius); paint(p)
            put_double(p.old, 0x2002d4c8, value); put_double(p.new, p.app + 0x1d0, value)
            p.call('pico_display_target_temperature', [7]); p.check_display()
        case('pico_display_target_temperature', f'{value},celsius={celsius}', target)
for value in [-1.5, 0.0, 5.29, 9.9, 10.0, 99.95]:
    def decimal(value=value):
        p = Pair(); put_double(p.old, 0x2002bef0, value)
        put_double(p.new, p.app + p.offsets[9] + 0x218, value)
        p.call('pico_display_recipe_decimal_metric'); p.check_display()
    case('pico_display_recipe_decimal_metric', str(value), decimal)
for value in [-1, 0, 9, 10, 99, 100, 999, 1000]:
    def integer(value=value):
        p = Pair(); p.old.write_u32(0x2002beec, u32(value))
        p.new.write_u32(p.app + p.offsets[9] + 0x214, u32(value))
        p.call('pico_display_recipe_integer_metric'); p.check_display()
    case('pico_display_recipe_integer_metric', str(value), integer)
for celsius in [0, 1]:
    for seconds in [0, 59, 60, 3599, 3600, 3661, 86399]:
        case('pico_ui_draw_circulation_status', f'{seconds},celsius={celsius}',
             lambda seconds=seconds,celsius=celsius: call_display('pico_ui_draw_circulation_status',
                 [*words_from_double(154.6), seconds], celsius=celsius))

for row, start, end in [(0, 0, 0), (4, 0, 21), (7, 20, 21), (8, 3, 7), (3, 20, 2)]:
    def stop_progress(row=row, start=start, end=end):
        p = Pair(); paint(p); addr = p.old.SCRATCH + 256
        before_old = bytearray(range(24)); before_old[0:3] = bytes([row, start, end]); before_old[16] = 255
        before_new = bytearray([7, 13, row, start, end, 9, 255])
        p.old.write(addr, bytes(before_old)); p.new.write(addr, bytes(before_new))
        p.call('pico_display_stop_progress_indicator', [addr])
        before_old[16] = 0; before_new[6] = 0
        assert p.old.read(addr, 24) == bytes(before_old)
        assert p.new.read(addr, 7) == bytes(before_new)
        p.check_display()
    case('pico_display_stop_progress_indicator', f'{row},{start},{end}', stop_progress)

for delay in [1, 3, 9]:
    def wait_click(delay=delay):
        p = Pair(); paint(p)
        p.old.unhook(MAP['pico_display_wait_for_click']); p.new.unhook(p.new.address('pico_display_wait_for_click'))
        polls = {'old': 0, 'new': 0}
        for side, arm in [('old', p.old), ('new', p.new)]:
            encoder = 0x2002bc28 if side == 'old' else p.encoder
            def clock(f, side=side, encoder=encoder):
                polls[side] += 1
                if polls[side] == delay: f.write(encoder + 12, b'\0')
                return polls[side]
            arm.hook(MAP['millis'] if side == 'old' else arm.address('ui_fixture_millis'), clock)
        p.call('pico_display_wait_for_click')
        assert polls['old'] == polls['new'] == delay
        assert p.old.read(0x2002bc28, 44) == p.new.read(p.encoder, 44)
        p.check_display()
    case('pico_display_wait_for_click', f'click_after_polls={delay}', wait_click)

for mac in [bytes(6), bytes([0, 1, 15, 16, 127, 255]), bytes([0xde, 0xad, 0xbe, 0xef, 0x12, 0x34])]:
    def info(mac=mac):
        p = Pair(); observed = {'old': [], 'new': []}
        uid_offset = p.new.read_u32(p.new.address('fixture_offsets') + 6 * 4)
        p.old.write(0x2002da9d, p.new.cstring(p.app + uid_offset) + b'\0')
        for side, arm in [('old', p.old), ('new', p.new)]:
            def mac_read(f, side=side):
                observed[side].append('mac'); f.write(f.reg(1), mac)
            arm.hook(0x20014b9c if side == 'old' else arm.address('fixture_mac'), mac_read)
        p.call('pico_screen_device_information'); p.check_display()
        assert observed['old'] == observed['new'] == ['mac']
    case('pico_screen_device_information', mac.hex(), info)

PAGES = ['pico_screen_changing_step', 'pico_screen_water_error_help', 'pico_screen_ending_process']

def install_pages(p, old_address, new_address, header, null_page=None):
    events = {'old': [], 'new': []}
    for side, arm, address in [('old', p.old, old_address), ('new', p.new, new_address)]:
        callbacks = [MAP[name] | 1 if side == 'old' else arm.address(name) | 1 for name in PAGES]
        if null_page is not None: callbacks[null_page] = 0
        arm.write(address, struct.pack('<7I', *header, *callbacks))
        for index, target in enumerate(callbacks):
            if target:
                def observe(uc, addr, size, data, side=side, index=index):events[side].append(index)
                arm.uc.hook_add(UC_HOOK_CODE, observe, begin=target & ~1, end=target & ~1)
    return events

for index in [0, 1, 2]:
    def refresh(index=index):
        p = Pair(); paint(p)
        events = install_pages(p, 0x2002d9d0, p.app + p.offsets[12], [index, 3, 0, 1000])
        p.call('pico_display_refresh_current_page')
        assert events['old'] == events['new'] == [0]
        p.check_display()
    case('pico_display_refresh_current_page', f'current_index={index};always_page_zero', refresh)
for index, count, last, interval, now, null_page in [
        (0, 3, 100, 1000, 1099, None), (0, 3, 100, 1000, 1100, None),
        (0, 3, 100, 1000, 1101, None), (2, 3, 100, 1000, 1101, None),
        (0, 3, 0xfffffff0, 40, 0x20, None), (0, 0, 100, 0, 101, None),
        (1, 3, 100, 1000, 200, 1), (0, 3, 100, 1000, 1101, 1)]:
    def advance(index=index, count=count, last=last, interval=interval, now=now, null_page=null_page):
        p = Pair(); paint(p); address = p.old.SCRATCH + 256
        events = install_pages(p, address, address, [index, count, last, interval], null_page)
        polls = {'old': 0, 'new': 0}
        for side, arm in [('old', p.old), ('new', p.new)]:
            def clock(f, side=side):polls[side] += 1; return (now + polls[side] - 1) & 0xffffffff
            arm.hook(MAP['millis'] if side == 'old' else arm.address('ui_fixture_millis'), clock)
        p.call('pico_display_advance_carousel', [address])
        assert p.old.read(address, 16) == p.new.read(address, 16)
        assert events['old'] == events['new']
        assert polls['old'] == polls['new']
        p.check_display()
    case('pico_display_advance_carousel', f'{index},{count},{last},{interval},{now},null={null_page}', advance)

# The original SDK retains small virtual helpers that the owned renderer
# replaces with fields or a composition of real rasterizer operations.
def native_line(p, x0, y0, x1, y1, color):
    p.new.call_symbol('pico_graphics_line', [p.display, u32(x0), u32(y0), u32(x1)],
                      stack_args=[u32(y1), color])

for rotation in range(4):
    for x, y, width, height in [(0, 0, 1, 1), (3, 4, 41, 23), (-5, -7, 24, 30), (60, 62, 80, 5)]:
        def rectangle(rotation=rotation, x=x, y=y, width=width, height=height):
            p = Pair(); paint(p); set_rotation(p, rotation)
            p.old.call(0x20013bb0, [0x2002bc00, u32(x), u32(y), width], stack_args=[height, 1])
            native_line(p, x, y, x + width - 1, y, 1)
            native_line(p, x, y + height - 1, x + width - 1, y + height - 1, 1)
            native_line(p, x, y, x, y + height - 1, 1)
            native_line(p, x + width - 1, y, x + width - 1, y + height - 1, 1)
            p.check_display()
        case('GFX_draw_rectangle', f'rotation={rotation},{x},{y},{width},{height}', rectangle,
             address=0x20013bb0, method='direct_original_vs_composed_owned_rasterizer')
    for address, name, vertical in [(0x20013c0e, 'GFX_draw_fast_vertical_line', True),
                                     (0x20013c2a, 'GFX_draw_fast_horizontal_line', False)]:
        for length in [-3, 0, 1, 16, 128]:
            def line(rotation=rotation, address=address, vertical=vertical, length=length):
                p = Pair(); paint(p); set_rotation(p, rotation); x, y = 3, -2
                p.old.call(address, [0x2002bc00, x, u32(y), u32(length)], stack_args=[1])
                native_line(p, x, y, x if vertical else x + length - 1,
                            y + length - 1 if vertical else y, 1)
                p.check_display()
            case(name, f'rotation={rotation},length={length}', line, address=address,
                 method='direct_original_vs_owned_endpoint_line')
    for color in [0, 1]:
        def full_screen(rotation=rotation, color=color):
            p = Pair(); paint(p); set_rotation(p, rotation)
            width, height = struct.unpack('<hh', p.new.read(p.display, 4))
            p.old.call(0x20013c80, [0x2002bc00, color])
            p.new.call_symbol('pico_graphics_fill_rectangle', [p.display, 0, 0, width],
                             stack_args=[height, color])
            p.check_display()
        case('GFX_fill_screen', f'rotation={rotation},color={color}', full_screen,
             address=0x20013c80, method='direct_original_vs_owned_full_bounds_fill')
    for radius in [0, 1, 5, 20]:
        def corners(rotation=rotation, radius=radius):
            p = Pair(); paint(p); set_rotation(p, rotation)
            x, y, width, height = 3, 5, 110, 48
            for cx, cy, corner in [(x + radius, y + radius, 1),
                                   (x + width - radius - 1, y + radius, 2),
                                   (x + width - radius - 1, y + height - radius - 1, 4),
                                   (x + radius, y + height - radius - 1, 8)]:
                p.old.call(0x20013cd0, [0x2002bc00, cx, cy, radius], stack_args=[corner, 1])
            # The compiler inlines circle_corners into the production rounded
            # rectangle. Suppress only its four independently tested straight
            # edge calls, leaving the actual compiled corner rasterizer intact.
            edges = []
            p.new.hook_symbol('pico_graphics_line', lambda f: edges.append(tuple(f.reg(i) for i in range(4))))
            p.new.call_symbol('pico_graphics_round_rectangle', [p.display, x, y, width],
                             stack_args=[height, radius, 1])
            assert len(edges) == 4
            p.check_display()
        case('GFX_draw_circle_quadrants', f'rotation={rotation},radius={radius},all_four_masks',
             corners, address=0x20013cd0, method='direct_original_vs_actual_inlined_caller_body')
    for address, name in [(0x2001412c, 'GFX_draw_bitmap'), (0x200141aa, 'GFX_draw_bitmap_alias')]:
        for x, y, width, height in [(-3, 1, 13, 9), (0, 0, 8, 8), (62, 60, 17, 11), (0, 0, 0, 1)]:
            def bitmap(rotation=rotation, address=address, x=x, y=y, width=width, height=height):
                p = Pair(); paint(p); set_rotation(p, rotation)
                data = bytes((i * 83 + 7) & 255 for i in range(max(1, ((width + 7) // 8) * height)))
                for arm in [p.old, p.new]:arm.write(arm.SCRATCH, data)
                stack = [width, height, 1]
                p.old.call(address, [0x2002bc00, u32(x), u32(y), p.old.SCRATCH], stack_args=stack)
                p.new.call_symbol('pico_graphics_bitmap', [p.display, u32(x), u32(y), p.new.SCRATCH], stack_args=stack)
                p.check_display()
            case(name, f'rotation={rotation},{x},{y},{width},{height}', bitmap, address=address)

# Text setters/getters compare the entire graphics-state block. The removed
# getter/wrap thunks are represented by their documented public owned fields.
setters = [
    (0x20014568, 'GFX_set_cursor', 'pico_graphics_cursor', [(0, 0), (-3, -7), (127, 63), (32767, -32768)]),
    (0x2001456e, 'GFX_set_text_size', 'pico_graphics_text_scale', [(0,), (1,), (2,), (255,)]),
    (0x20014578, 'GFX_set_text_color', 'pico_graphics_text_color', [(0,), (1,), (65535,)]),
    (0x2001457e, 'GFX_set_text_colors', 'pico_graphics_text_colors', [(0, 0), (1, 0), (0, 1), (65535, 15)]),
    (0x2001458c, 'GFX_set_rotation', 'pico_graphics_rotation', [(0,), (1,), (2,), (3,), (4,), (255,)])]
for address, name, new_name, inputs in setters:
    for args in inputs:
        def setter(address=address, new_name=new_name, args=args):
            p = Pair(); paint(p)
            p.old.call(address, [0x2002bc00, *map(u32, args)])
            p.new.call_symbol(new_name, [p.display, *map(u32, args)])
            p.check_display(); graphics_state(p)
        case(name, repr(args), setter, address=address)
for rotation in range(4):
    for address, name, offset, fmt in [(0x20014588, 'GFX_get_rotation', 13, '<B'),
                                      (0x200145aa, 'GFX_get_width', 0, '<h'),
                                      (0x200145b0, 'GFX_get_height', 2, '<h')]:
        def getter(rotation=rotation, address=address, offset=offset, fmt=fmt):
            p = Pair(); set_rotation(p, rotation)
            value = p.old.call(address, [0x2002bc00])['r0']
            owned = struct.unpack(fmt, p.new.read(p.display + offset, struct.calcsize(fmt)))[0]
            assert value == u32(owned)
        case(name, f'rotation={rotation}', getter, address=address,
             method='direct_original_vs_owned_public_field')
for wrap in [0, 1]:
    def wrapping(wrap=wrap):
        p = Pair(); before = p.new.read(p.display, 16)
        p.old.call(0x20014584, [0x2002bc00, wrap])
        p.new.write(p.display + 14, bytes([wrap]))
        graphics_state(p)
        assert p.new.read(p.display, 14) == before[:14]
    case('GFX_set_text_wrap', f'wrap={wrap}', wrapping, address=0x20014584,
         method='direct_original_vs_owned_public_field_assignment')
for invert in [0, 1]:
    def noop(invert=invert):
        p = Pair(); paint(p); before = p.old.read(0x2002bc00, 40)
        new_before = p.new.read(p.display, 19 + 1024)
        p.old.call(0x20013c9c, [0x2002bc00, invert])
        assert p.old.read(0x2002bc00, 40) == before
        assert p.new.read(p.display, 19 + 1024) == new_before
        p.check_display()
    case('GFX_invert_display_noop', f'invert={invert}', noop, address=0x20013c9c,
         method='original_noop_execution_vs_elided_base_method')
for character in [0, 10, 13, 32, 65, 175, 176, 255]:
    for scale in [1, 2]:
        for wrap in [0, 1]:
            def character_write(character=character, scale=scale, wrap=wrap):
                p = Pair(); paint(p); set_scale(p, scale)
                p.old.write(0x2002bc1a, bytes([wrap])); p.new.write(p.display + 14, bytes([wrap]))
                p.old.call(0x20014568, [0x2002bc00, 122, 7])
                p.new.call_symbol('pico_graphics_cursor', [p.display, 122, 7])
                p.old.call(0x20014470, [0x2002bc00, character])
                p.new.call_symbol('pico_graphics_write', [p.display, character])
                p.check_display(); graphics_state(p)
            case('GFX_write_character', f'character={character},scale={scale},wrap={wrap}',
                 character_write, address=0x20014470)

# The extra callbacks are called at their actual old entry points and their
# real linked local symbols. No implementation body is supplied by a fixture.
for phase in range(4):
    for previous in [1, 2]:
        def rotation_isr(phase=phase, previous=previous):
            p = Pair(); state = bytearray(44); state[:3] = bytes([7, 8, 9])
            struct.pack_into('<iiiii', state, 16, -2, 2, 0, 1, 0)
            state[40:44] = bytes([1, previous, 0, 3 - phase])
            p.old.write(0x2002bc28, bytes(state)); p.new.write(p.encoder, bytes(state))
            events = {'old': [], 'new': []}
            for side, arm in [('old', p.old), ('new', p.new)]:
                def read(f, side=side):
                    pin = f.reg(0 if side == 'old' else 1); events[side].append(pin)
                    return phase // 2 if pin == 7 else phase & 1
                arm.hook(MAP['digitalRead'] if side == 'old' else arm.address('ui_fixture_read'), read)
            p.old.call(0x20004714); p.new.call_symbol('pico_encoder_rotation_isr')
            assert p.old.read(0x2002bc28, 44) == p.new.read(p.encoder, 44)
            assert events['old'] == events['new'] == [7, 8]
        case('pico_encoder_rotation_isr', f'phase={phase},previous={previous}', rotation_isr,
             address=0x20004714)
for name, address, literal in [('recipe_image_page', 0x20004788, 0x200047ac),
                               ('recipe_logo_page', 0x20009a60, 0x20009ab4)]:
    for detailed in [0, 1]:
        for celsius in [0, 1]:
            def recipe_bitmap(name=name, address=address, literal=literal,
                              detailed=detailed, celsius=celsius):
                p = Pair(detailed=detailed, celsius=celsius)
                data = bytes((i * 29 + 123) & 255 for i in range(1024))
                original_buffer = p.old.read_u32(literal)
                p.old.write(original_buffer, data)
                p.new.write(p.new.SCRATCH, data)
                if name == 'recipe_logo_page':
                    p.new.call_symbol('pico_ui_recipe_logo_ready', [p.ui, p.new.SCRATCH])
                else:
                    p.new.call_symbol('pico_ui_recipe_image_ready', [p.ui, 0, p.new.SCRATCH])
                    p.new.write(p.new.SCRATCH, bytes(1024))
                    p.new.call_symbol('pico_ui_recipe_image_ready', [p.ui, 3, p.new.SCRATCH])
                p.old.call(address); p.new.call_symbol(name); p.check_display()
            case(name, f'detailed={detailed},celsius={celsius}', recipe_bitmap, address=address)
for detailed in [0, 1]:
    for seconds in [-1, 0, 59, 60, 3600]:
        def current_timer(detailed=detailed, seconds=seconds):
            p = Pair(detailed=detailed)
            p.old.write_u32(0x2002d2f8 + 0x1a8, u32(seconds))
            p.new.write_u32(p.app + 0x1a8, u32(seconds))
            p.old.call(0x20009b1c); p.new.call_symbol('current_timer'); p.check_display()
        case('current_timer', f'detailed={detailed},seconds={seconds}', current_timer, address=0x20009b1c)

# These are deliberately changed invalid-input behaviors, not equivalence
# cases. Record actual unsafe old pointers/assertions alongside defined results.
repairs = []
for name, value, label in [('pico_action_name', 50, b'<actionEnum>'),
                           ('pico_action_label', 50, b'<action>'),
                           ('pico_title_label', 5, b'<titleId>')]:
    for language in ([0] if name == 'pico_action_name' else [0, 1, 2, 3]):
        p = Pair(); events = diagnostics(p)
        args = [value] if name == 'pico_action_name' else ([value, language, 0] if name == 'pico_action_label' else [value, language])
        a, b = p.call(name, args)
        assert p.new.cstring(b['r0']) == label
        assert events['old'][0][1] == 1 and events['new'][0][1] == 0
        try:
            old_text = p.old.cstring(a['r0']).decode('latin1')
            old_pointer_invalid = False
        except Exception:
            old_text = None
            old_pointer_invalid = True
        repairs.append({'function': name, 'input': args,
                        'original_return_pointer': hex(a['r0']),
                        'original_string': old_text, 'original_pointer_invalid': old_pointer_invalid,
                        'rebuilt_string': label.decode(), 'bounds_assertion_now_rejects': True})



report = {'image_sha256': hashlib.sha256((PROJECT / 'reference/original-0.1.34.bin').read_bytes()).hexdigest(),
          'elf_sha256': hashlib.sha256(ELF.read_bytes()).hexdigest(),
          'test_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
          'passed_cases': len(results), 'failed_cases': len(failures),
          'tested_functions': sorted({x['function'] for x in results}),
          'tested_original_entries': sorted({x['original_address'] for x in results}),
          'evidence_per_entry': dict(Counter(x['original_address'] for x in results)),
          'repair_scenarios': len(repairs), 'repaired_input_checks': repairs,
          'preconditions': ['pico_screen_set_step requires a non-null name; both original and rebuilt formatting fault on NULL.'],
          'scope': __doc__, 'failures': failures, 'cases': results}
write_report(report, 'ui-direct-display-tests.json', 'ui-direct-display-tests.json')
print(json.dumps({k: report[k] for k in ('passed_cases', 'failed_cases', 'tested_original_entries', 'failures')}, indent=2))
if failures:sys.exit(1)
