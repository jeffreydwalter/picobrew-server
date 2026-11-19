#ifndef PICO_DISPLAY_H
#define PICO_DISPLAY_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_display_clear(void);
void pico_display_flush(void);
void pico_display_center_bitmap(short y, uint32_t *bitmap);
void pico_display_center_message(char *text);
void pico_display_construct(void);
void pico_display_refresh_current_page(void);
void pico_screen_set_recipe_name(PicoScreenState *screen, char *recipe_name);
void pico_screen_set_step(PicoScreenState *screen, pico_u32 component, char *step_name);
void pico_screen_add_lines(PicoTextScreen *screen, int count, ...);
void pico_display_clear_thunk(void);
void pico_display_advance_carousel(PicoPageCarousel *carousel);
uint8_t *pico_display_digit_bitmap(int digit);
void pico_display_initialize(void);
void pico_display_text_at_cell(short row, short column, char *text);
void pico_display_string_at_cell(short row, short column, char *text);
void pico_display_center_text(short row, char *text);
void pico_display_text_at_pixels(short y, short x, char *text);
void pico_display_device_bitmap(void);
void pico_display_logo(void);
void pico_display_right_aligned_text(short row, short column, char *text, uint32_t field_width,
                                     char argument_5);
void pico_display_character_on_row(short x, short row, char argument_3);
void pico_display_character_at_pixels(short x, short y, char argument_3);
void pico_display_draw_content_frame(void);
void pico_display_draw_titled_frame(char *title);
void pico_display_draw_program_frame(void);
void pico_display_recipe_decimal_metric(void);
void pico_display_recipe_integer_metric(void);
void pico_display_target_temperature(short row);
void pico_display_large_timer(int total_minutes, uint32_t seconds, int show_seconds);
void pico_display_large_temperature(uint32_t temperature_low, uint32_t temperature_high,
                                    int show_decimal);
void pico_display_large_timer_with_seconds(int total_minutes, uint32_t seconds);
void pico_display_large_temperature_with_decimal(uint32_t temperature_low,
                                                 uint32_t temperature_high);
void pico_display_stop_progress_indicator(uint8_t *indicator);
void pico_display_device_bitmap_variant_a(void);
void pico_display_device_bitmap_variant_b(void);
void pico_display_clear_row(short argument_1);
void pico_display_update_timer(short argument_1, int started_ms, int duration_seconds);
void pico_display_clear_rows(uint32_t first_row, uint32_t last_row);
void pico_display_valve_position(short argument_1, uint32_t position);
void pico_display_temperature_columns(short argument_1, uint32_t argument_2,
                                      uint32_t wort_temperature_low, uint32_t wort_temperature_high,
                                      double argument_5, double __aeabi_ddiv_result);
void pico_display_temperature_summary(uint32_t wort_temperature_low, uint32_t wort_temperature_high,
                                      uint32_t steam_temperature_low,
                                      uint32_t steam_temperature_high, double __aeabi_ddiv_result,
                                      uint16_t argument_6);
void pico_display_clear_row_from_column(short argument_1, short argument_2);
void pico_display_adjustment_box(int box_style, uint32_t box_index);

#endif /* PICO_DISPLAY_H */
