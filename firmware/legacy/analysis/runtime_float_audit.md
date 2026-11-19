# Bounded runtime floating-point audit

## Conclusion

The generated runtime files parse, but the reviewed software floating-point
bodies are not behaviorally faithful C yet. There is a systematic mismatch
between numeric C casts and the raw bit reinterpretation intended by the
machine instructions. This is a semantic issue independent of pointer-warning
settings. These routines should remain labeled as analysis reconstructions.

Scope: generated `src/platform/software_float.c`, the relevant macros in
`include/pico/recovered_types.h`, the floating-point conversion warnings in
`src/platform/libc.c`, and selected original Thumb instructions. This is a
bounded audit, not a full review of all runtime behavior. No generated file,
generator, or Ghidra project was modified by this audit. The final packaging
pass subsequently added a declared, zero-safe leading-zero helper; the semantic
issues below remain unresolved.

## Confirmed issues

| Area | Concrete evidence | Intended operation |
| --- | --- | --- |
| Double bits read as numeric integers | `libm_ceil` contains `(uint32_t)((uint64_t)value >> 0x20)` with `value` declared `double`. Original instruction at `0x2001a834` directly extracts exponent bits from `r1`. | Read the upper word of the IEEE-754 representation. Numeric conversion of `1.5` gives integer1 and upper word0; its actual upper bits are `0x3ff80000`. |
| Integer bit patterns numerically converted to floating point | Many arithmetic/conversion return paths contain `(double)pico_join_u32_u32(...)` or `(float)(value_9 | 0x7f800000)`. | Reinterpret the assembled bits. For example, numeric `(float)0x3f800000` is1065353216.0, whereas reinterpreting those bits yields1.0. |
| `ABS(left)` exponent extraction | `__aeabi_fsub` contains `(uint32_t)ABS(left) >> 0x17`, with `left` a float. Assembly `0x2002152c` shifts its raw register left1, then `0x20021542` shifts right24. | Extract `(float_bits(left) & 0x7fffffffU) >>23`. Defining ABS as `fabsf` does not fix the subsequent numeric cast. |
| Double storage emitted as `uint64_t` while assignments are floating expressions | `libc_atof` declares `__aeabi_ui2d_result_2` as `uint64_t` but assigns `INFINITY`, `NAN`, and results of double arithmetic. Compiler diagnostics confirm infinity/NaN-to-integer conversions. | This local is a raw double-storage view. Either use a double variable with explicit bit access or keep raw bits and explicitly encode/decode every floating assignment/use. Simply suppressing warnings changes nothing. |
| Rounded printed constant exceeds double range | `libc_atof` emits `1.79769313486232e+308`; compiler diagnoses overflow. The original input reconstruction used raw maximum-finite bits `0x7fefffffffffffff`. | Preserve exact raw bits or the exact hexadecimal constant `0x1.fffffffffffffp+1023`. |
| Flags and hidden-register results remain unresolved | `__aeabi_dcmpeq`/`dcmplt`/`dcmple`/`dcmpge`/`dcmpgt` read uninitialized `unresolved_input_ZR`/`unresolved_input_CY` after a helper call. Multiply/divide special paths also use unresolved register-result locals. | Reconstruct the helper's flag/multiple-register contract or use a verified original runtime implementation. Bitcast helpers cannot restore missing flag dataflow. |

Other known limits within this file include undefined C shifts for counts at or
above operand width, signed shifts/overflow used to model ARM integer behavior,
and intentional division-by-zero handler paths currently
rendered as C `1 / 0`. These require target-semantics review and cannot be
certified by a syntax check.

## Safe, readable helpers for established bit operations

The existing `PICO_BITS`/`PICO_SUB_U32` family already uses a memcpy-based read,
which is appropriate for known bit extraction on this little-endian target.
For example, use `PICO_SUB_U32(value,4)` for the high word of a double instead
of `(uint32_t)((uint64_t)value >>32)` when the original instruction is a bit
extraction.

Explicit typed helpers can express the reverse operation without aliasing:

```c
static inline uint32_t float_bits(float value)
{
    uint32_t bits;
    __builtin_memcpy(&bits, &value, sizeof bits);
    return bits;
}

static inline float float_from_bits(uint32_t bits)
{
    float value;
    __builtin_memcpy(&value, &bits, sizeof value);
    return value;
}

static inline uint64_t double_bits(double value)
{
    uint64_t bits;
    __builtin_memcpy(&bits, &value, sizeof bits);
    return bits;
}

static inline double double_from_bits(uint64_t bits)
{
    double value;
    __builtin_memcpy(&value, &bits, sizeof value);
    return value;
}
```

Require `sizeof(float)==4`, `sizeof(double)==8`, and the expected IEEE binary
formats for these interpretations. Preserve exact image NaN payloads where
known; assigning generic `NAN` may discard the original bit pattern. A CLZ
helper must define the zero-input case as32 to model ARM CLZ.

Apply these helpers only where instruction/pcode evidence establishes a
representation change. A blanket replacement of every cast is unsound: true
numeric conversions coexist with raw bit operations. Ghidra's pcode/typed
operation metadata distinguishes those intentions better than C-text regexes.
Likewise, the PICO_PART writable casts deserve separate alignment/aliasing
review; read-only memcpy helpers alone do not repair those stores.

## Packaging recommendation

Describe the current verification as a syntax/structure check, and preserve
the raw image/address map and original decompilation. Record that the compiler
runtime and some floating-point storage views still require semantic cleanup.
The hand-reconstructed PID methods are a separate, reviewed abstraction and
should not be cited as validation of these runtime implementations.

For an eventual executable firmware build, prefer the matching verified TI/GCC
runtime objects or a separately validated runtime port. Do not implement
`__aeabi_dadd` as `return left + right` on the same soft-float target: the
compiler may lower that expression to a call to `__aeabi_dadd` itself. Such
direct expressions can be useful in distinctly named host-side reference code,
but are not a safe drop-in definition of the target compiler helper.
