#!/usr/bin/env python3
"""Execute the production allocator and bounded SDK heap provider in ARM.

No allocation, syscall, locking or hardware routine is substituted. ELF symbols
supply heap bounds, the native stack and libc's reentrancy/errno objects. Data
access outside physical application SRAM is rejected even though the generic
test harness maps a larger address range for its return sentinel.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from rebuilt_arm import RebuiltArm
from unicorn import UC_HOOK_MEM_INVALID

ENOMEM = 12  # Pinned compiler's errno.h; also observed in native allocator code.
SRAM_START, SRAM_END = 0x20004000, 0x20040000


class Allocator:
    def __init__(self, elf):
        self.arm = RebuiltArm(elf)
        self.lower = self.arm.address('_heap')
        self.upper = self.arm.address('_eheap')
        self.stack = self.arm.read_u32(self.arm.address('g_pfnVectors'))
        self.arm.STACK = self.stack
        self.context = self.arm.read_u32(self.arm.address('_impure_ptr'))
        self.errno_address = self.arm.call_symbol('__errno')['r0']
        self.syscall_errno_address = self.arm.address('errno')
        # Restrict the generic harness to physical application RAM. The sole
        # extra page is the test harness's synthetic return endpoint.
        self.arm.uc.mem_unmap(0x20000000, SRAM_START-0x20000000)
        self.arm.uc.mem_unmap(SRAM_END, 0x20100000-SRAM_END)
        self.arm.uc.mem_map(self.arm.RETURN & ~4095, 4096)
        self.arm.write(self.arm.RETURN, b'\x00\xbe')
        self.calls = 0
        self.instructions = 0
        self.checks = 0
        self.faults = []
        self.live = {}
        self.arm.write(self.lower, b'\xa5'*(self.upper-self.lower))
        self.guard = bytes([0x5a])*min(64, SRAM_END-self.upper)
        self.arm.write(self.upper, self.guard)
        self.arm.uc.hook_add(UC_HOOK_MEM_INVALID, self.invalid_access)

    def invalid_access(self, uc, access, address, size, value, user_data):
        self.faults.append({'address': hex(address), 'size': size, 'access': access,
                            'pc': hex(self.arm.reg('pc'))})
        return False

    def check(self, condition, message):
        self.checks += 1
        if not condition:
            raise AssertionError(message)

    def call(self, name, *arguments):
        result = self.arm.call_symbol(name, [value & 0xffffffff for value in arguments],
                                      max_instructions=2000000)
        self.calls += 1
        self.instructions += result['instructions']
        self.check(result['sp'] == self.stack, 'Native stack was not restored')
        self.check(self.arm.read(self.upper, len(self.guard)) == self.guard,
                   'Heap upper-bound canary was changed')
        self.check(not self.faults, 'Unmapped memory access')
        return result['r0']

    def error(self):
        return self.arm.read_u32(self.errno_address)

    def clear_error(self):
        self.arm.write_u32(self.errno_address, 0)
        self.arm.write_u32(self.syscall_errno_address, 0)

    def heap_break(self):
        # Query the actual SDK-owned heap_end through its own implementation.
        value = self.call('_sbrk', 0)
        self.check(self.lower <= value <= self.upper, 'SDK heap break escaped reservation')
        return value

    def own(self, pointer, size, payload=None):
        self.check(pointer != 0, 'Expected successful allocation')
        self.check(pointer % 8 == 0, 'Allocation violates eight-byte alignment')
        self.check(self.lower <= pointer <= pointer+size <= self.upper,
                   ('Allocation escapes heap', hex(pointer), size))
        for other, data in self.live.items():
            self.check(pointer+size <= other or other+len(data) <= pointer,
                       'Live user allocations overlap')
        if payload is None:
            payload = bytes((index*17+size) & 255 for index in range(size))
        self.arm.write(pointer, payload)
        self.live[pointer] = payload
        self.verify_live()

    def verify_live(self):
        for pointer, payload in self.live.items():
            self.check(self.arm.read(pointer, len(payload)) == payload,
                       ('Live allocation content changed', hex(pointer), len(payload)))

    def release(self, pointer):
        self.call('free', pointer)
        self.live.pop(pointer, None)
        self.verify_live()

    def failed(self, function, *arguments):
        self.clear_error()
        pointer = self.call(function, *arguments)
        self.check(pointer == 0, ('Allocation failure did not return NULL', function, hex(pointer)))
        self.check(self.error() == ENOMEM, ('Allocation failure errno', function, self.error()))
        self.verify_live()
        self.heap_break()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, default=ROOT/'build/firmware/pico-pro-recovered.elf')
    parser.add_argument('--output', type=Path, default=ROOT/'analysis/native-allocator-tests.json')
    args = parser.parse_args()
    results = []

    def case(name, action):
        allocator = Allocator(args.elf)
        try:
            action(allocator)
        except Exception as error:
            raise AssertionError(name + ': ' + str(error)) from error
        results.append({'name': name, 'checks': allocator.checks,
                        'native_calls': allocator.calls, 'arm_instructions': allocator.instructions,
                        'heap_lower': hex(allocator.lower), 'heap_upper': hex(allocator.upper),
                        'initial_stack': hex(allocator.stack), 'memory_faults': allocator.faults})

    for size in (0, 1, 2, 3, 4, 7, 8, 9, 15, 16, 17, 31, 32, 255, 256, 257, 1023, 4096, 16384):
        def ordinary(a, size=size):
            pointer = a.call('malloc', size)
            a.own(pointer, size)
            a.release(pointer)
            replacement = a.call('malloc', size)
            a.own(replacement, size)
            a.release(replacement)
            a.release(0)
            a.heap_break()
        case('malloc_alignment_write_free_reallocate_' + str(size), ordinary)

    for count, width in ((0, 12), (8, 0), (1, 1), (17, 13), (32, 256)):
        def zeroed(a, count=count, width=width):
            length = count*width
            pointer = a.call('calloc', count, width)
            a.check(a.arm.read(pointer, length) == b'\0'*length,
                    'calloc did not zero newly allocated nonzero SRAM')
            a.own(pointer, length)
            a.release(pointer)
        case(f'calloc_zeroed_{count}_by_{width}', zeroed)

    for count, width in ((0xffffffff, 2), (0x80000000, 2), (65536, 65536), (0x10000000, 16)):
        def overflow(a, count=count, width=width):
            before = a.heap_break()
            a.failed('calloc', count, width)
            a.check(a.heap_break() == before, 'calloc overflow consumed heap')
        case(f'calloc_multiplication_overflow_{count}_by_{width}', overflow)

    for size in (32768, 32769, 0x7fffffff, 0x80000000, 0xffffffff):
        def failed_first(a, size=size):
            a.failed('malloc', size)
            a.failed('calloc', 1, size)
            a.failed('realloc', 0, size)
            pointer = a.call('malloc', 64)
            a.own(pointer, 64)
            a.failed('realloc', pointer, size)
            a.release(pointer)
        case('empty_heap_and_realloc_failure_' + str(size), failed_first)

    def resize(a):
        pointer = a.call('realloc', 0, 96)
        a.own(pointer, 96)
        for new_size in (192, 4096, 1000, 16):
            previous = a.live[pointer]
            updated = a.call('realloc', pointer, new_size)
            a.check(updated != 0, 'Ordinary realloc unexpectedly failed')
            a.check(a.arm.read(updated, min(len(previous), new_size)) == previous[:new_size],
                    'realloc did not preserve the retained prefix')
            del a.live[pointer]
            a.own(updated, new_size)
            pointer = updated
        zero = a.call('realloc', pointer, 0)
        del a.live[pointer]
        # A zero-size realloc may return a minimum-sized allocation. The
        # selected full-newlib provider does so; that result must be freeable.
        if zero:
            a.own(zero, 0)
            a.release(zero)
        a.own(a.call('malloc', 128), 128)
    case('realloc_null_growth_shrink_and_zero', resize)

    for size in (128, 1024, 4096):
        def exhausted(a, size=size):
            blocks = []
            for _ in range((a.upper-a.lower)//size + 2):
                a.clear_error()
                pointer = a.call('malloc', size)
                if not pointer:
                    a.check(a.error() == ENOMEM, 'Heap exhaustion did not report ENOMEM')
                    break
                a.own(pointer, size)
                blocks.append(pointer)
            else:
                raise AssertionError('Allocator exceeded finite heap budget')
            a.check(bool(blocks), 'No ordinary allocation succeeded')
            a.failed('malloc', size)
            released = blocks.pop(len(blocks)//2)
            a.release(released)
            replacement = a.call('malloc', size)
            a.own(replacement, size)
            for pointer in blocks + [replacement]: a.release(pointer)
            a.own(a.call('calloc', 16, 128), 2048)
        case('exhaustion_and_recovery_block_' + str(size), exhausted)

    def grow_shrink(a):
        lower, upper = a.lower, a.upper
        a.check(a.call('__wrap__sbrk', 0) == lower, 'Initial break differs from linker heap')
        position = lower
        for increment in (1, 7, upper-lower-8, 0, -1, -7, -(upper-lower-8), 0):
            a.check(a.call('__wrap__sbrk', increment) == position, 'sbrk did not return previous break')
            position += increment
            a.check(a.heap_break() == position, 'sbrk growth/shrink differs')
        a.check(position == lower, 'Round-trip shrink did not reach heap start')
    case('sdk_heap_exact_bounds_signed_growth_and_shrink', grow_shrink)

    for growth, increment in ((0, -1), (0, -0x80000000), (0, 0x7fffffff),
                              (0, 32769), (0, -0x20000000), (32, -33), (32768, 1)):
        def bounds(a, growth=growth, increment=increment):
            a.call('__wrap__sbrk', growth)
            before = a.heap_break()
            a.clear_error()
            a.check(a.call('__wrap__sbrk', increment) == 0xffffffff,
                    'Invalid signed heap delta did not fail with (void*)-1')
            a.check(a.arm.read_u32(a.syscall_errno_address) == ENOMEM,
                    'Heap wrapper did not set syscall errno')
            a.check(a.heap_break() == before, 'Failed heap delta changed SDK-owned break')
            a.clear_error()
            a.check(a.call('_sbrk_r', a.context, increment) == 0xffffffff,
                    'Reentrant heap request did not preserve failure')
            a.check(a.error() == ENOMEM, '_sbrk_r did not propagate errno to active reentrancy object')
            a.check(a.heap_break() == before, 'Reentrant failure changed heap break')
        case(f'signed_sbrk_rejection_after_{growth}_delta_{increment}', bounds)

    for seed in (7, 421):
        def mixed(a, seed=seed):
            randomizer = random.Random(seed)
            for _ in range(80):
                choice = randomizer.randrange(4)
                if choice == 0 and a.live:
                    a.release(randomizer.choice(list(a.live)))
                elif choice == 1 and a.live:
                    pointer = randomizer.choice(list(a.live))
                    old = a.live[pointer]
                    size = randomizer.choice((1, 13, 128, 1024, 4096, 40000))
                    a.clear_error()
                    updated = a.call('realloc', pointer, size)
                    if not updated:
                        a.check(a.error() == ENOMEM, 'Mixed realloc failure missing errno')
                        a.verify_live()
                    else:
                        a.check(a.arm.read(updated, min(len(old), size)) == old[:size],
                                'Mixed realloc did not preserve data')
                        del a.live[pointer]
                        a.own(updated, size)
                else:
                    count = randomizer.choice((1, 2, 5, 17))
                    width = randomizer.choice((1, 7, 64, 256))
                    a.clear_error()
                    pointer = a.call('calloc', count, width) if choice == 2 else a.call('malloc', count*width)
                    if not pointer:
                        a.check(a.error() == ENOMEM, 'Mixed allocation failure missing errno')
                        a.verify_live()
                    else:
                        if choice == 2:
                            a.check(a.arm.read(pointer, count*width) == b'\0'*(count*width),
                                    'Mixed calloc reused bytes without zeroing')
                        a.own(pointer, count*width)
                a.heap_break()
            for pointer in list(a.live): a.release(pointer)
            a.own(a.call('malloc', 8192), 8192)
        case('bounded_mixed_allocation_sequence_seed_' + str(seed), mixed)

    report = {'passed': True, 'production_elf_sha256': hashlib.sha256(args.elf.read_bytes()).hexdigest(),
              'test_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'total_cases': len(results), 'total_checks': sum(x['checks'] for x in results),
              'native_calls': sum(x['native_calls'] for x in results), 'cases': results,
              'substituted_functions': [], 'physical_device_executed': False,
              'scope': 'Actual production malloc/calloc/realloc/free, full-newlib allocation family, bounded adapter and genuine SDK _sbrk execute under ARM emulation. Tests cover alignment, ownership, live-data preservation, calloc multiplication overflow and zeroing, empty/nonempty-heap exhaustion, free/reuse, realloc failure preservation, signed heap bounds and errno. Physical data access is restricted to application SRAM and the vector-table stack is used. This is bounded single-threaded runtime validation, not whole-appliance peak-memory or interrupt-concurrency validation.'}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed', 'total_cases', 'total_checks', 'native_calls')}, indent=2))


if __name__ == '__main__':
    main()
