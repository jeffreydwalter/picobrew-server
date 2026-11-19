/* Host integration tests use the actual application, UI, recipes, actuator
 * interlocks, local storage and controller. Only electrical/SDK boundaries are
 * simulated; no appliance or network is contacted. */
#include "picobrew/local_control.h"
#include "picobrew/local_api.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static PicoApplication app;
static PicoControl control;
static PicoActuators actuators;
static PicoLocalApi api;
static struct {
    uint32_t now;
    unsigned polls, begins, disconnects, removals, energized_after_stop;
    unsigned address_reads;
    int pins[40], wifi, open_file;
    size_t file_cursor;
    bool irq_masked, stopping, queue_circulate, stop_on_power, stop_on_poll, stop_in_brew_menu;
    bool click_on_clock, no_ip;
    uint8_t spi_command, rfid_address;
} sim;
static struct { char name[40]; uint8_t data[2048]; size_t length; } files[32];

static uint32_t clock_ms(void *c)
{ (void)c; if (sim.click_on_clock) app.ui.encoder.click_latch=0; return ++sim.now; }
static void delay_ms(void *c, uint32_t ms) { (void)c; sim.now += ms; }
static void no_op(void *c) { (void)c; }
static void pin_mode(void *c, uint8_t p, uint8_t m) { (void)c; (void)p; (void)m; }
static int pin_read(void *c, uint8_t p) { (void)c; return sim.pins[p]; }
static bool power_pin(unsigned p)
{
    return p == PICO_PIN_HEATER || p == PICO_PIN_STEAM_PUMP ||
           p == PICO_PIN_DRAIN_PUMP || p == PICO_PIN_SHUTTLE_PUMP;
}
static void pin_write(void *c, uint8_t p, uint8_t value)
{
    (void)c;
    if (value && power_pin(p) && pico_local_control_stopped())
        ++sim.energized_after_stop;
    sim.pins[p] = value;
    if (p == 4 && !value) sim.rfid_address = 0;
}
static uint8_t spi(void *c, uint8_t value)
{
    (void)c;
    if (!sim.pins[4] && sim.pins[18]) {
        if (!sim.rfid_address) { sim.rfid_address = value; return 0; }
        return sim.rfid_address == 0xd4 ? 0x80 : sim.rfid_address == 0xa2 ? 0x3d : 0;
    }
    if (sim.pins[4] && !sim.pins[18]) {
        if (value != 0xff) sim.spi_command = value;
        return sim.spi_command == 0x37 ? 12 : sim.spi_command;
    }
    return 0;
}
static void spi_mode(void *c, uint8_t m) { (void)c; (void)m; }
static void disable(void *c) { (void)c; sim.irq_masked = true; }
static bool enable(void *c) { (void)c; bool old = sim.irq_masked; sim.irq_masked = false; return old; }
static void attach(void *c, unsigned p, void (*h)(void), unsigned m)
{ (void)c; (void)p; (void)h; (void)m; }
static void serial_begin(void *c, uint32_t b) { (void)c; (void)b; }
static void serial(void *c, const char *text) { (void)c; (void)text; }
static const PicoApplicationHardware hardware = {
    clock_ms, delay_ms, pin_mode, pin_read, pin_write, no_op, spi, spi_mode, spi_mode,
    disable, enable, attach, serial_begin
};
static void actuator_mode(void *c, unsigned p, unsigned m) { pin_mode(c, (uint8_t)p, (uint8_t)m); }
static int actuator_read(void *c, unsigned p) { return pin_read(c, (uint8_t)p); }
static void actuator_write(void *c, unsigned p, unsigned v) { pin_write(c, (uint8_t)p, (uint8_t)v); }
static void microseconds(void *c, uint32_t us) { (void)c; (void)us; }
static uint8_t phase(void *c) { (void)c; return app.state.controller.phase; }
static void pause_filter(void *c) { (void)c; pico_phase_pause_until_resume(&app.phase); }
static void insert_filter(void *c) { (void)c; pico_device_wait_for_filter(&app.device); }
static const PicoActuatorOps actuator_ops = {actuator_mode, actuator_read, actuator_write,
    microseconds, delay_ms, pause_filter, insert_filter, phase};
static void heater_off(void *c) { (void)c; pico_actuator_heater_off(&actuators); }
static void shuttle_off(void *c) { (void)c; pico_actuator_shuttle_off(&actuators); }
static void timer2(void *c, uint32_t a, uint32_t b) { (void)c; (void)a; (void)b; }
static void timer1(void *c, uint32_t a) { (void)c; (void)a; }
static void timer3(void *c, uint32_t a, uint32_t b, uint32_t d)
{ (void)c; (void)a; (void)b; (void)d; }
static void timer_register(void *c, uint32_t a, uint32_t b, void (*h)(void))
{ (void)c; (void)a; (void)b; (void)h; }
static float wort(void *c) { (void)c; return 70; }
static const PicoControlOps control_ops = {.millis=clock_ms, .interrupts_disable=disable,
    .interrupts_enable=enable, .heater_off=heater_off, .shuttle_pump_off=shuttle_off,
    .read_wort_fahrenheit=wort, .peripheral_clock_enable=timer2, .peripheral_reset=timer1,
    .timer_configure=timer2, .timer_load_set=timer3, .timer_interrupt_register=timer_register,
    .timer_interrupt_enable=timer2, .timer_enable=timer2};

static int find_file(const char *name)
{
    for (unsigned i=0; i<32; ++i) if (!strcmp(files[i].name,name)) return (int)i;
    return -1;
}
static int file_stat(void *c, const char *name, size_t *size)
{
    (void)c; int i=find_file(name); if (i<0) return 1; *size=files[i].length; return 0;
}
static bool file_open(void *c, const char *name, uint32_t mode)
{
    (void)c; assert(sim.open_file == -1);
    int i=find_file(name);
    if (i<0 && mode != 0 && mode != 0x1000) {
        for (i=0;i<32 && files[i].name[0];++i) {}
        assert(i<32); snprintf(files[i].name,sizeof files[i].name,"%s",name);
    }
    if (i<0) return false;
    sim.open_file=i; sim.file_cursor=0; return true;
}
static int file_read(void *c)
{
    (void)c; assert(sim.open_file >= 0);
    if (sim.file_cursor >= files[sim.open_file].length) return -1;
    return files[sim.open_file].data[sim.file_cursor++];
}
static int32_t file_write(void *c, const uint8_t *data, size_t length)
{
    (void)c; assert(sim.open_file >= 0 && length<=sizeof files[0].data);
    memcpy(files[sim.open_file].data,data,length); files[sim.open_file].length=length;
    return (int32_t)length;
}
static bool file_close(void *c) { (void)c; sim.open_file=-1; return true; }
static int file_remove(void *c, const char *name)
{
    (void)c; ++sim.removals; int i=find_file(name); if (i>=0) memset(&files[i],0,sizeof files[i]); return 0;
}
static bool yes(void *c) { (void)c; return true; }
static int wifi_status(void *c) { (void)c; return sim.wifi; }
static int wifi_begin(void *c, const char *s, const uint8_t *p, uint8_t sec, bool binary)
{ (void)c; (void)s; (void)p; (void)sec; (void)binary; ++sim.begins; return sim.wifi; }
static void disconnect(void *c) { (void)c; ++sim.disconnects; }
static PicoIpv4 ip(void *c) { (void)c; return (PicoIpv4){{192,168,1,42}}; }
static void ip_config(void *c, PicoIpv4 *local, PicoIpv4 *dns_ip, PicoIpv4 *gateway, PicoIpv4 *mask)
{
    (void)c; ++sim.address_reads;
    *local=*dns_ip=*gateway=*mask=sim.no_ip ? (PicoIpv4){{0}} : ip(NULL);
}
static int dns(void *c, const char *h, PicoIpv4 *a)
{ (void)c; (void)h; (void)a; assert(!"Unexpected DNS request"); return -1; }
static void set_dns(void *c, PicoIpv4 a) { (void)c; (void)a; assert(!"Unexpected DNS override"); }
static int connect_client(void *c, PicoIpv4 a, uint16_t p, bool b)
{ (void)c; (void)a; (void)p; (void)b; assert(!"Unexpected outbound HTTP"); return -1; }
static int disconnected(void *c) { (void)c; return 0; }
static void mac(void *c, uint8_t out[6]) { (void)c; memset(out,0x42,6); }
static bool md5(void *c, const uint8_t *data, size_t length, uint8_t digest[16], uint32_t start)
{ (void)c; (void)data; (void)length; (void)start; memset(digest,0x24,16); return true; }
static void reset_mcu(void *c) { (void)c; assert(!"Unexpected firmware activation/reset"); }
static const PicoNetworkPlatform network = {.millis=clock_ms, .delay_ms=delay_ms,
    .serial_write=serial, .wifi_initialize=yes, .wifi_initialized=yes, .wifi_connecting=yes,
    .wifi_status=wifi_status, .wifi_begin=wifi_begin, .wifi_disconnect=disconnect, .local_ip=ip,
    .resolve_host=dns, .set_dns=set_dns, .client_connect=connect_client,
    .client_connected=disconnected, .mac_address=mac, .md5=md5, .file_begin=no_op,
    .file_open=file_open, .file_read=file_read, .file_write=file_write,
    .file_close=file_close, .file_remove=file_remove, .reset_mcu=reset_mcu,
    .ip_configuration=ip_config};

static bool outputs_off(void)
{
    return !sim.pins[5] && !sim.pins[8] && !sim.pins[24] && !sim.pins[27];
}
static void poll(void *c)
{
    (void)c; assert(!sim.irq_masked); assert(sim.pins[4] && sim.pins[18]);
    assert(sim.open_file == -1); ++sim.polls;
    unsigned before=sim.polls;
    sim.now+=5; pico_local_service(); assert(before==sim.polls); /* no reentry */
    if (sim.queue_circulate) {
        sim.queue_circulate=false;
        assert(pico_local_control_command("circulate",NULL)==PICO_LOCAL_CONTROL_OK);
        sim.stop_on_power=true;
    }
    if (sim.stop_in_brew_menu) {
        if (app.ui.encoder.minimum == 7) {
            sim.stop_in_brew_menu=false;
            sim.stop_on_poll=true;
        } else if (!outputs_off()) {
            assert(pico_local_control_encoder(0,true)==PICO_LOCAL_CONTROL_OK);
        }
    }
    if (sim.stop_on_poll || (sim.stop_on_power && !outputs_off())) {
        sim.stop_on_poll=sim.stop_on_power=false;
        assert(pico_local_control_command("stop",NULL)==PICO_LOCAL_CONTROL_OK);
        assert(outputs_off());
        assert(pico_local_control_command("clear-stop",NULL)==PICO_LOCAL_CONTROL_BUSY);
        sim.stopping=true;
    }
}
static void initialize(void)
{
    memset(&app,0,sizeof app); memset(&sim,0,sizeof sim); memset(files,0,sizeof files);
    sim.open_file=-1;
    for (unsigned i=0;i<40;++i) sim.pins[i]=1;
    sim.pins[11]=0; sim.pins[5]=sim.pins[8]=sim.pins[24]=sim.pins[27]=0;
    control=(PicoControl){&app.state.controller,&app.state.control_signals,&control_ops,NULL};
    actuators=(PicoActuators){&actuator_ops,NULL};
    app.state.controller.phase=PICO_BREW_PHASE_IDLE;
    app.state.controller.wort_fahrenheit=70;
    app.state.controller.thermoblock_fahrenheit=120;
    app.state.controller.candidate_shuttle_scale=0.2;
    app.state.recipe.program=&app.state.session;
    app.state.carousel.interval_ms=10000;
    const PicoPageCallback *pages=pico_application_default_pages();
    for (unsigned i=0;i<5;++i) app.state.carousel.pages[i]=pages[i];
    app.state.carousel.page_count=5;
    pico_pid_construct(&app.state.controller.wort_pid,&app.state.controller.wort_fahrenheit,
        &app.state.controller.wort_pid_output,&app.state.controller.wort_target_fahrenheit,
        1,1,1,0,3000,0);
    pico_pid_construct(&app.state.controller.thermoblock_pid,&app.state.controller.thermoblock_fahrenheit,
        &app.state.controller.thermoblock_pid_output,&app.state.controller.thermoblock_target_fahrenheit,
        1,1,1,0,3000,0);
    pico_application_bind_services(&app,&control,&actuators,&hardware,NULL,&network,NULL);
    pico_rfid_construct(&app.reader,4,18,0);
    pico_local_store_bind(&app.network,file_stat,NULL);
    pico_local_api_initialize(&api,&app);
    pico_local_control_bind(&app,poll,NULL);
}
static void ready(void) { pico_local_control_ready(true); pico_local_control_enter_menu(); }

static void test_boot_and_offline_adapters(void)
{
    initialize();
    /* No stored SSID, no internet, empty flash: startup reaches the main loop. */
    pico_application_setup(&app);
    assert(pico_local_control_is_ready() && outputs_off() && sim.begins==0);
    assert(!app.network.server_host[0]);
    assert(!pico_network_resolve_address(&app.network));
    assert(!pico_network_open_request(&app.network,"/anything",true,false));
    assert(!pico_network_call(&app.network,"/anything",true,false));
    assert(pico_api_register(&app.network));
    char first[32],second[32];
    assert(pico_api_get_session(&app.network,first,sizeof first));
    assert(pico_api_get_session(&app.network,second,sizeof second));
    assert(strcmp(first,second));
    PicoLocalRecipe recipe={.id="test-ale",.name="Test Ale",.step_count=1};
    strcpy(recipe.steps[0].name,"Mash"); recipe.steps[0].location=2;
    recipe.steps[0].active_minutes=20; recipe.steps[0].target_fahrenheit=152;
    bool created=false;
    assert(pico_local_recipe_put(&app.network,&recipe,&created)==PICO_LOCAL_STORE_OK && created);
    assert(pico_api_fetch_recipe(&app.network,"test-ale",-1,-1)==0);
    assert(!strcmp(app.state.screen.recipe_name,"Test Ale"));
    assert(app.state.recipe.program==&app.state.session && app.state.recipe.steps[0].target_fahrenheit==152);
    PicoRecipeListing listing={0}; assert(pico_api_recipe_list(&app.network,&listing,false));
    assert(listing.loaded_items==9 && !strcmp(listing.identifiers[0],"pico-pak-64") &&
           !strcmp(listing.identifiers[8],"test-ale"));
    assert(pico_api_fetch_recipe(&app.network,"missing",-1,-1)==14);
    assert(pico_api_required_actions(&app.network,NULL,0)==0);
    assert(pico_api_log_step(&app.network,"Mash") && pico_api_log_progress(&app.network,"Mash"));
    assert(pico_api_error(&app.network,19)); pico_api_report_state(&app.network);
    assert(api.event_count>0);
    assert(!pico_update_available(&app.network) && !pico_update_download(&app.network));
    pico_update_run(&app.network);
    app.settings.language=2; app.settings.temperature_unit_celsius=1;
    assert(pico_storage_save_display(&app.network));
    app.settings.language=0; app.settings.temperature_unit_celsius=0;
    pico_storage_load_preferences(&app.network);
    assert(app.settings.language==2 && app.settings.temperature_unit_celsius==1);
    puts("PASS: offline boot, recipes, sessions, reports, settings, no DNS/HTTP/update");
}
static void test_checkpoint_restore_is_explicit_and_recipe_bound(void)
{
    initialize(); ready();
    PicoLocalRecipe stored={.id="checkpoint",.name="Checkpoint Ale",.step_count=1};
    strcpy(stored.steps[0].name,"Mash"); stored.steps[0].location=1;
    stored.steps[0].target_fahrenheit=152; stored.steps[0].active_minutes=60;
    stored.steps[0].drain_minutes=3;
    assert(pico_local_recipe_put(&app.network,&stored,NULL)==PICO_LOCAL_STORE_OK);
    assert(pico_api_fetch_recipe(&app.network,"checkpoint",-1,-1)==0);
    pico_recipe_program_initialize(&app.recipe,&app.state.session,1,0);
    assert(pico_storage_save_recovery(&app.network,13,5,0,PICO_BREW_PHASE_HEATED_HOLD,true,1));
    PicoLocalRecovery before,after;
    assert(pico_local_recovery_get(&app.network,&before)==PICO_LOCAL_STORE_OK);
    assert(pico_recipe_session_restore(&app.recipe,&app.state.recipe));
    assert(app.state.recipe.steps[0].active_minutes==13 && app.state.recipe.steps[0].drain_minutes==5);
    assert(app.state.recipe.steps[0].target_fahrenheit==152);
    assert(app.recipe.recovery_phase==PICO_BREW_PHASE_HEATED_HOLD+1);
    assert(!app.state.session.identifier[0] && outputs_off());
    assert(pico_local_recovery_get(&app.network,&after)==PICO_LOCAL_STORE_OK);
    assert(!memcmp(&before,&after,sizeof before));
    /* Looking at the offer repeatedly still reads the original program. */
    assert(pico_recipe_session_restore(&app.recipe,&app.state.recipe));
    assert(app.state.recipe.steps[0].active_minutes==13 && outputs_off());
    stored.steps[0].target_fahrenheit=155;
    assert(pico_local_recipe_put(&app.network,&stored,NULL)==PICO_LOCAL_STORE_OK);
    assert(!pico_recipe_session_restore(&app.recipe,&app.state.recipe));
    assert(!app.recipe.recovery_phase && outputs_off());
    assert(pico_local_recipe_delete(&app.network,"checkpoint")==PICO_LOCAL_STORE_OK);
    assert(!pico_recipe_session_restore(&app.recipe,&app.state.recipe) && outputs_off());
    int checkpoint_file=find_file("/local/resume.a");
    assert(checkpoint_file>=0);
    files[checkpoint_file].data[20]^=0x80;
    assert(!pico_recipe_session_restore(&app.recipe,&app.state.recipe) && outputs_off());
    files[checkpoint_file].data[20]^=0x80;
    /* This is the same discard boundary used after declining the boot offer. */
    pico_storage_delete_restart(&app.network);
    assert(!pico_recipe_session_restore(&app.recipe,&app.state.recipe) && outputs_off());
    assert(pico_local_recovery_clear(&app.network));
    assert(!pico_recipe_session_restore(&app.recipe,&app.state.recipe));
    pico_recipe_initialize_cleaning(&app.recipe,&app.state.recipe,135);
    assert(pico_storage_save_recovery(&app.network,0,2,5,PICO_BREW_PHASE_DRAINING,true,0));
    assert(pico_recipe_session_restore(&app.recipe,&app.state.recipe));
    assert(app.state.recipe.current_step==5 && app.state.recipe.steps[5].active_minutes==0);
    assert(app.recipe.recovery_phase==PICO_BREW_PHASE_DRAINING+1 && outputs_off());
    initialize(); ready();
    assert(file_open(NULL,"restart.txt",0x3000));
    const char *legacy="0:checkpoint:0:0:0:13:3:0:1:";
    assert(file_write(NULL,(const uint8_t *)legacy,strlen(legacy))==(int)strlen(legacy));
    assert(file_close(NULL));
    assert(!pico_recipe_session_restore(&app.recipe,&app.state.recipe) && outputs_off());
    puts("PASS: checkpoint offers preserve remaining time, require unchanged recipe, leave outputs and sessions off");
}
static void test_guards_and_queue(void)
{
    initialize(); assert(pico_local_control_command("stop",NULL)==PICO_LOCAL_CONTROL_NOT_READY);
    ready();
    assert(pico_local_control_command("unknown",NULL)==PICO_LOCAL_CONTROL_INVALID);
    assert(pico_local_control_command("brew","../../bad")==PICO_LOCAL_CONTROL_INVALID);
    assert(pico_local_control_command("circulate",NULL)==PICO_LOCAL_CONTROL_OK);
    assert(pico_local_control_busy() && outputs_off());
    assert(pico_local_control_command("rinse",NULL)==PICO_LOCAL_CONTROL_BUSY);
    assert(pico_local_control_command("stop",NULL)==PICO_LOCAL_CONTROL_OK);
    assert(!pico_local_control_run_pending() && outputs_off());
    assert(pico_local_control_command("rinse",NULL)==PICO_LOCAL_CONTROL_STOPPED);
    assert(pico_local_control_command("clear-stop",NULL)==PICO_LOCAL_CONTROL_OK);
    app.ui.encoder.minimum=-10; app.ui.encoder.maximum=10;
    assert(pico_local_control_encoder(100,true)==PICO_LOCAL_CONTROL_OK);
    assert(app.ui.encoder.position==10 && app.ui.encoder.click_latch==0 && !sim.irq_masked);
    assert(pico_local_control_encoder(-100,false)==PICO_LOCAL_CONTROL_OK && app.ui.encoder.position==-10);
    assert(pico_local_control_encoder(101,true)==PICO_LOCAL_CONTROL_INVALID);
    sim.now+=10; pico_local_service(); unsigned count=sim.polls;
    pico_local_control_bus_select(4,true); sim.now+=100; pico_local_service(); assert(sim.polls==count);
    pico_local_control_bus_select(4,false); pico_local_service(); assert(sim.polls==count+1);
    count=sim.polls;
    pico_device_spi_selected_byte(&app.device,0x29); /* actual shared-SPI delay path */
    assert(sim.polls==count);
    puts("PASS: readiness, validation, deferred queue, encoder, SPI exclusion and reentry");
}
static void test_stop_unwinds_workflows(void)
{
    initialize(); ready(); sim.now=100; sim.queue_circulate=true;
    pico_application_loop(&app); /* queues inside the actual blocking main menu */
    assert(sim.stopping && pico_local_control_stopped() && outputs_off());
    assert(sim.energized_after_stop==0 && !app.state.control_signals.enabled);
    assert(app.state.controller.phase==PICO_BREW_PHASE_IDLE);
    /* Old workflow or timer-side actuator calls cannot reenergize a latched stop. */
    pico_actuator_heater_on(&actuators); pico_actuator_shuttle_on(&actuators);
    pico_actuator_drain_on(&actuators); pico_actuator_steam_on(&actuators);
    pico_control_start(&control);
    assert(outputs_off() && sim.energized_after_stop==0 && !app.state.control_signals.enabled);
    pico_local_control_enter_menu();
    assert(pico_local_control_command("clear-stop",NULL)==PICO_LOCAL_CONTROL_OK);
    assert(outputs_off());
    sim.stopping=false; sim.stop_on_power=true;
    assert(pico_local_control_command("drain-filter",NULL)==PICO_LOCAL_CONTROL_OK);
    assert(pico_local_control_run_pending());
    assert(sim.stopping && outputs_off() && !sim.energized_after_stop);
    initialize(); ready(); pico_local_control_leave_menu(); sim.stop_on_power=true;
    assert(!pico_device_check_thermoblock_heating(&app.device,&control));
    assert(sim.stopping && outputs_off() && sim.now<1000);
    initialize(); ready(); pico_local_control_leave_menu(); sim.stop_on_poll=true;
    pico_encoder_clear_click(&app.ui.encoder);
    pico_ui_wait_for_resume();
    assert(sim.stopping && outputs_off());
    initialize(); ready(); pico_local_control_leave_menu(); sim.stop_in_brew_menu=true;
    pico_recipe_program_initialize(&app.recipe,&app.state.session,1,0);
    PicoStepRequest step={0,1,0,2,"Test Recipe","Hold",0,1,7,0};
    assert(pico_recipe_run_step(&app.recipe,&step)==0);
    assert(app.state.recipe.current_step==7);
    assert(sim.stopping && outputs_off() && !sim.energized_after_stop);
    puts("PASS: queued main-menu dispatch; stop unwinds circulation, drain, thermal wait, pause, in-brew menu");
}
static void test_bounded_wifi(void)
{
    initialize(); ready(); uint32_t start=sim.now;
    assert(!pico_network_connect_stored(&app.network));
    assert(sim.begins==2 && sim.disconnects==2 && sim.now-start<22000);
    sim.wifi=3; assert(pico_network_resolve_server(&app.network));
    assert(pico_network_check_status(&app.network,false,true)==1);
    puts("PASS: WiFi CONNECTING is bounded and local connectivity requires no DNS");
}
static void test_network_status_failure(void)
{
    uint8_t expected[PICO_FRAMEBUFFER_SIZE];
    initialize(); ready(); pico_display_initialize();
    pico_display_clear(); pico_screen_wifi_connection_failed();
    memcpy(expected,app.ui.display.framebuffer,sizeof expected);
    sim.click_on_clock=true;
    pico_ui_show_network_addresses();
    assert(sim.address_reads==0 && !memcmp(expected,app.ui.display.framebuffer,sizeof expected));
    initialize(); ready(); pico_display_initialize();
    sim.wifi=3;sim.no_ip=true;sim.click_on_clock=true;
    pico_ui_show_network_addresses();
    assert(sim.address_reads==1 && !memcmp(expected,app.ui.display.framebuffer,sizeof expected));
    puts("PASS: network status reports connection failure instead of displaying stale or absent addresses");
}
int main(void)
{
    setvbuf(stdout,NULL,_IOLBF,0);
    test_boot_and_offline_adapters(); test_guards_and_queue();
    test_checkpoint_restore_is_explicit_and_recipe_bound();
    test_stop_unwinds_workflows(); test_bounded_wifi(); test_network_status_failure();
    puts("Standalone control integration tests passed."); return 0;
}
