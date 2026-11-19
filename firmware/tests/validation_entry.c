/* Entry point for the function-validation ELF, not a brewer startup routine.
 * Tests enter individual symbols in a separate ARM emulator instance.
 */
void pico_validation_entry(void)
{
    for (;;)
        __asm__ volatile("nop");
}
