#ifndef PICO_RECOVERED_CORE_LAYOUTS_H
#define PICO_RECOVERED_CORE_LAYOUTS_H

/* Reconstructed from Pico Pro 0.1.34 accesses and diagnostic strings.
 * Layouts describe the original 32-bit ARM ABI. Pointer-bearing structures
 * have different offsets on a 64-bit host and must not map device memory there.
 */
typedef unsigned char pico_u8;
typedef unsigned int pico_u32;
typedef int pico_i32;

enum { PICO_MENU_CAPACITY = 22, PICO_TEXT_LINE_CAPACITY = 6 };

typedef struct PicoMenu {
    pico_u32 actions[PICO_MENU_CAPACITY]; /* +0x00 */
    pico_u32 count;                       /* +0x58 */
} PicoMenu;

typedef struct PicoTextScreen {
    pico_u32 line_count;                        /* +0x00 */
    const char *lines[PICO_TEXT_LINE_CAPACITY]; /* +0x04 */
} PicoTextScreen;

typedef struct PicoScreenState {
    char recipe_name[32];        /* +0x00 */
    char step_name[32];          /* +0x20 */
    pico_u32 progress_component; /* +0x40 */
} PicoScreenState;

typedef struct PicoProgramContext {
    char identifier[32];          /* +0x00, printed as id */
    pico_u32 program_id;          /* +0x20 */
    pico_u32 temperature;         /* +0x24 */
    pico_u32 duration;            /* +0x28 */
    pico_u32 location;            /* +0x2c */
    pico_u32 session_type;        /* +0x30 */
    const char *title;            /* +0x34 */
    const char *name;             /* +0x38 */
    const char *state_label;      /* +0x3c */
    const char *logo_text;        /* +0x40 */
    const char *interrupted_text; /* +0x44 */
    const void *bitmap_primary;   /* +0x48, exact visual role incomplete */
    const void *bitmap_secondary; /* +0x4c, exact visual role incomplete */
    const pico_u32 *actions;      /* +0x50 */
    pico_u32 action_count;        /* +0x54 */
} PicoProgramContext;

typedef struct PicoEncoder {
    pico_u8 phase_a_pin; /* +0x00 */
    pico_u8 phase_b_pin; /* +0x01 */
    pico_u8 button_pin;  /* +0x02 */
    pico_u8 unknown_03[9];
    pico_u8 click_latch; /* +0x0c; zero means pending click */
    pico_u8 unknown_0d[3];
    pico_i32 minimum;           /* +0x10 */
    pico_i32 maximum;           /* +0x14 */
    pico_i32 previous_position; /* +0x18 */
    pico_i32 step;              /* +0x1c */
    pico_i32 position;          /* +0x20 */
    pico_u8 button_interrupt;   /* +0x24 */
    pico_u8 phase_a_interrupt;  /* +0x25 */
    pico_u8 phase_b_interrupt;  /* +0x26 */
    pico_u8 unknown_27;
    pico_u8 wrap_enabled;      /* +0x28 */
    pico_u8 previous_phase;    /* +0x29 */
    pico_u8 current_phase;     /* +0x2a */
    pico_u8 last_stable_phase; /* +0x2b */
} PicoEncoder;

typedef struct PicoPageCarousel {
    pico_u32 page_index;
    pico_u32 page_count;
    pico_u32 last_update_ms;
    pico_u32 interval_ms;
    void (*pages[15])(void);
} PicoPageCarousel;

#endif
