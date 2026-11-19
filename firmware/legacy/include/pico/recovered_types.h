#ifndef PICO_RECOVERED_TYPES_H
#define PICO_RECOVERED_TYPES_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Unknown indirect ABI: parameters and return interpretation need review. */
typedef uint32_t PicoOpaqueFunction();
typedef void PicoInterruptHandler(void);
typedef struct __attribute__((packed)) {
    unsigned int value : 24;
} PicoPackedU24;

#define PICO_PART_U8(value, offset) (*(uint8_t *)((uint8_t *)&(value) + (offset)))
#define PICO_PART_U16(value, offset) (*(uint16_t *)((uint8_t *)&(value) + (offset)))
#define PICO_PART_U24(storage, offset)                                                             \
    (((PicoPackedU24 *)((uint8_t *)&(storage) + (offset)))->value)
#define PICO_PART_U32(value, offset) (*(uint32_t *)((uint8_t *)&(value) + (offset)))
#define PICO_PART_U64(value, offset) (*(uint64_t *)((uint8_t *)&(value) + (offset)))
/* GNU C expressions preserve bit extraction from both integer and float values. */
#define PICO_BITS(value)                                                                           \
    ({                                                                                             \
        __auto_type pico_bits_value = (value);                                                     \
        uint64_t pico_bits_result = 0;                                                             \
        __builtin_memcpy(&pico_bits_result, &pico_bits_value, sizeof(pico_bits_value));            \
        pico_bits_result;                                                                          \
    })
#define PICO_SUB_U8(value, offset) ((uint8_t)(PICO_BITS(value) >> (8u * (offset))))
#define PICO_SUB_U16(value, offset) ((uint16_t)(PICO_BITS(value) >> (8u * (offset))))
#define PICO_SUB_U32(value, offset) ((uint32_t)(PICO_BITS(value) >> (8u * (offset))))
#ifndef NAN
#define NAN (__builtin_nanf(""))
#endif
#ifndef INFINITY
#define INFINITY (__builtin_inff())
#endif

static inline uint16_t pico_join_u8_u8(uint8_t high, uint8_t low)
{
    return ((uint16_t)high << 8) | low;
}
static inline uint32_t pico_join_u8_u16(uint8_t high, uint16_t low)
{
    return ((uint32_t)high << 16) | low;
}
static inline uint32_t pico_join_u8_u24(uint8_t high, uint32_t low)
{
    return ((uint32_t)high << 24) | (low & 0xffffffu);
}
static inline uint32_t pico_join_u16_u16(uint16_t high, uint16_t low)
{
    return ((uint32_t)high << 16) | low;
}
static inline uint64_t pico_join_u16_u32(uint16_t high, uint32_t low)
{
    return ((uint64_t)high << 32) | low;
}
static inline uint64_t pico_join_u16_u48(uint16_t high, uint64_t low)
{
    return ((uint64_t)high << 48) | (low & UINT64_C(0xffffffffffff));
}
static inline uint32_t pico_join_u24_u8(uint32_t high, uint8_t low)
{
    return (high << 8) | low;
}
static inline uint64_t pico_join_u32_u32(uint32_t high, uint32_t low)
{
    return ((uint64_t)high << 32) | low;
}
static inline unsigned pico_count_leading_zeros_u32(uint32_t value)
{
    return value ? (unsigned)__builtin_clz(value) : 32u;
}
static inline float pico_abs_f32(float value)
{
    return __builtin_fabsf(value);
}
static inline bool pico_carry_u32(uint32_t left, uint32_t right)
{
    return (uint64_t)left + right > UINT32_MAX;
}
static inline bool pico_add_overflow_i32(int32_t left, int32_t right)
{
    int64_t sum = (int64_t)left + right;
    return sum > INT32_MAX || sum < INT32_MIN;
}
static inline bool pico_sub_overflow_i32(int32_t left, int32_t right)
{
    int64_t difference = (int64_t)left - right;
    return difference > INT32_MAX || difference < INT32_MIN;
}

/* Processor operations emitted by the decompiler. These declarations describe
 * the operations; a future target build must supply the ARM implementations.
 * They are not extra functions recovered from the firmware image. */
bool pico_cpu_is_privileged(void);
bool pico_cpu_irq_is_enabled(void);
void pico_cpu_disable_irq(void);
void pico_cpu_enable_irq(void);
void pico_cpu_data_sync_barrier(uint32_t scope);
void pico_cpu_instruction_sync_barrier(uint32_t scope);

/* Deliberately undefined: resolve the original register contract before linking.
 * This is an analysis marker, never a replacement returning fabricated data. */
uint64_t pico_unresolved_register_result(const char *file, unsigned line, const char *call);
#define PICO_UNRESOLVED_RETURN(call)                                                               \
    ((call), pico_unresolved_register_result(__FILE__, __LINE__, #call))
#endif
