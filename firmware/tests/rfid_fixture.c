/* Test-only typed hardware boundaries. Python hooks these entry points;
 * this translation unit is excluded from the production application archive. */
#include "picobrew/rfid.h"
PicoRfid rfid_fixture_reader;
uint8_t rfid_fixture_data[128];
uint8_t rfid_fixture_result[128];
PicoRfidUid rfid_fixture_uid;
void rfid_fixture_pin_mode(void *c, uint8_t p, uint8_t m)
{
    (void)c;
    (void)p;
    (void)m;
}
void rfid_fixture_write(void *c, uint8_t p, uint8_t v)
{
    (void)c;
    (void)p;
    (void)v;
}
uint8_t rfid_fixture_transfer(void *c, uint8_t v)
{
    (void)c;
    (void)v;
    return 0;
}
void rfid_fixture_order(void *c, uint8_t o)
{
    (void)c;
    (void)o;
}
void rfid_fixture_mode(void *c, uint8_t m)
{
    (void)c;
    (void)m;
}
void rfid_fixture_delay(void *c, uint32_t m)
{
    (void)c;
    (void)m;
}
static const PicoRfidOps ops = {rfid_fixture_pin_mode, rfid_fixture_write, rfid_fixture_transfer,
                                rfid_fixture_order,    rfid_fixture_mode,  rfid_fixture_delay};
void rfid_fixture_bind(void)
{
    rfid_fixture_reader.ops = &ops;
    rfid_fixture_reader.context = 0;
}
PicoRfidApplication rfid_fixture_application;
uint32_t rfid_fixture_clock(void *c)
{
    (void)c;
    return 0;
}
void rfid_fixture_clear_click(void *c)
{
    (void)c;
}
bool rfid_fixture_click(void *c)
{
    (void)c;
    return false;
}
bool rfid_fixture_filter(void *c)
{
    (void)c;
    return true;
}
void rfid_fixture_screen(void *c)
{
    (void)c;
}
void rfid_fixture_flush(void *c)
{
    (void)c;
}
void rfid_fixture_log(void *c, const char *l, const char *m, const char *s)
{
    (void)c;
    (void)l;
    (void)m;
    (void)s;
}
void rfid_fixture_error(void *c, unsigned e, bool a, bool r)
{
    (void)c;
    (void)e;
    (void)a;
    (void)r;
}
static const PicoRfidApplicationOps application_ops = {
    rfid_fixture_clock,  rfid_fixture_clear_click, rfid_fixture_click, rfid_fixture_filter,
    rfid_fixture_screen, rfid_fixture_flush,       rfid_fixture_log,   rfid_fixture_error};
void rfid_fixture_bind_application(void)
{
    rfid_fixture_bind();
    rfid_fixture_application = (PicoRfidApplication){&rfid_fixture_reader, &application_ops, 0};
}
