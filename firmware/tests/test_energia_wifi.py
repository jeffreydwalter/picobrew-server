#!/usr/bin/env python3
"""Exercise the production Wi-Fi bridge, connection loop and SDK event patch.

Only driver calls/time are modeled. The callback bodies come directly from the
hash-pinned standalone SDK patch used by the firmware build, rather than from
a fake that silently fixes the stale-state bug. No radio/device is accessed.
"""
import json
import os
from pathlib import Path
import subprocess
import tempfile

SIMPLELINK = r'''
#pragma once
#include <stdint.h>
typedef uint8_t _u8; typedef int32_t _i32; typedef uint32_t _u32;
#define SL_ERROR_WIFI_ALREADY_DISCONNECTED (-129)
#define SL_ERROR_WIFI_NOT_CONNECTED (-59)
#define SL_POLICY_CONNECTION 0x10
#define SL_CONNECTION_POLICY(a,b,c,d,e) ((a)|((b)<<1)|((c)<<2)|((d)<<3)|((e)<<4))
#define SL_IPV4_STA_P2P_CL_DHCP_ENABLE 1
#define SL_IPV4_STA_P2P_CL_GET_INFO 2
#define SL_SEC_TYPE_WEP 1
#define SL_SEC_TYPE_WPA 2
#define SL_AF_INET 2
#define FS_MODE_OPEN_READ 0
#define FS_MODE_OPEN_WRITE 1
#define SL_WLAN_CONNECT_EVENT 1
#define SL_WLAN_DISCONNECT_EVENT 2
#define SL_WLAN_STA_CONNECTED_EVENT 5
#define SL_WLAN_STA_DISCONNECTED_EVENT 6
#define SL_NETAPP_IPV4_IPACQUIRED_EVENT 1
#define SL_NETAPP_IPV6_IPACQUIRED_EVENT 2
#define SL_NETAPP_IP_LEASED_EVENT 3
#define SL_NETAPP_IP_RELEASED_EVENT 4
#define sl_WlanEvtHdlr SimpleLinkWlanEventHandler
#define sl_NetAppEvtHdlr SimpleLinkNetAppEventHandler
typedef struct { uint8_t Type; signed char *Key; uint8_t KeyLen; } SlSecParams_t;
typedef struct { uint32_t ipV4,ipV4Mask,ipV4Gateway,ipV4DnsServer; } SlNetCfgIpV4Args_t;
typedef struct { uint32_t PingIntervalTime,PingSize,PingRequestTimeout,TotalNumberOfAttempts,Flags,Ip; } SlPingStartCommand_t;
typedef struct { uint32_t PacketsReceived; } SlPingReport_t;
typedef struct {
  uint32_t Event;
  struct {
    struct { char ssid_name[32]; uint8_t ssid_len,bssid[6]; } STAandP2PModeWlanConnected;
    struct { uint8_t mac[6]; } APModeStaConnected,APModestaDisconnected;
  } EventData;
} SlWlanEvent_t;
typedef struct {
  uint32_t Event;
  struct { struct { uint32_t ip; } ipAcquiredV4; struct { uint32_t ip_address; uint8_t mac[6]; } ipLeased; } EventData;
} SlNetAppEvent_t;
#ifdef __cplusplus
extern "C" {
#endif
int sl_WlanPolicySet(int,int,const void*,int);
int sl_WlanDisconnect(void);
int sl_WlanProfileDel(int);
int sl_WlanConnect(const signed char*,int,const void*,const SlSecParams_t*,const void*);
int sl_WlanProfileAdd(const signed char*,int,const void*,const SlSecParams_t*,const void*,int,int);
int sl_NetCfgSet(int,int,int,const unsigned char*);
int sl_NetCfgGet(int,void*,unsigned char*,unsigned char*);
uint32_t sl_Htonl(uint32_t);
int sl_FsOpen(const _u8*,_u32,_u32*,_i32*);
int sl_FsRead(_i32,_u32,_u8*,_u32);
int sl_FsWrite(_i32,_u32,_u8*,_u32);
int sl_FsClose(_i32,const _u8*,const _u8*,_u32);
int sl_NetAppPingStart(const SlPingStartCommand_t*,int,SlPingReport_t*,void*);
void SimpleLinkWlanEventHandler(SlWlanEvent_t*);
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t*);
#ifdef __cplusplus
}
#endif
'''
WIFI = r'''
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "simplelink.h"
#define MAX_SSID_LEN 32
#define BSSID_LEN 6
enum wl_status_t { WL_IDLE_STATUS=0,WL_CONNECTED=3,WL_CONNECT_FAILED=4,WL_DISCONNECTED=6 };
class IPAddress { uint32_t address; public: IPAddress(uint32_t n=0):address(n){} uint8_t operator[](unsigned i)const{return (address>>(i*8))&255;} };
class WiFiClass { public:
  static bool _initialized,_connecting; static volatile uint32_t local_IP;
  static volatile wl_status_t WiFi_status; static char connected_ssid[MAX_SSID_LEN];
  static unsigned char connected_bssid[BSSID_LEN]; static unsigned _connectedDeviceCount;
  static bool init(); uint8_t status(); IPAddress localIP(); void setIpDefaults(); int disconnect();
  int scanNetworks(); const char *SSID(uint8_t); uint8_t encryptionType(uint8_t); int32_t RSSI(uint8_t); void macAddress(uint8_t*);
  static void _registerNewDeviceIP(uint8_t*,uint8_t*); static void _unregisterDevice(uint8_t*);
};
extern WiFiClass WiFi;
class SerialClass { public: void print(const char*); void print(float,unsigned); };
extern SerialClass Serial;
uint32_t millis(); void delay(uint32_t);
'''
SLFS = r'''
#pragma once
#include <stdint.h>
#include <stddef.h>
#define SL_FS_OK 0
#define SLFS_LIB_ERR_FILE_ALREADY_OPEN (-10004)
class FlashClass { public: void begin(); int32_t open(const char*,int32_t); int close(); int read(); int32_t write(const uint8_t*,size_t); int del(const char*); };
extern FlashClass SerFlash;
'''
DRIVER = r'''
#pragma once
#include <stdint.h>
#include <stdbool.h>
#define SHAMD5_BASE 0
#define PRCM_DTHE 0
#define PRCM_RUN_MODE_CLK 0
#define SHAMD5_ALGO_MD5 0
uint32_t SHAMD5IntStatus(uint32_t,bool);
void SHAMD5IntDisable(uint32_t,uint32_t);
void SHAMD5IntEnable(uint32_t,uint32_t);
void SHAMD5IntRegister(uint32_t,void(*)(void));
void SHAMD5ConfigSet(uint32_t,uint32_t);
bool SHAMD5DataProcess(uint32_t,uint8_t*,unsigned,uint8_t*);
void PRCMPeripheralClkEnable(uint32_t,uint32_t);
void PRCMMCUReset(bool);
'''


def main():
    root = Path(__file__).resolve().parents[1]
    patches = json.loads((root / 'dependencies/lock.json').read_text())['energia']['standalone_patches']
    callback_patch = next(p for p in patches if p['file'] == 'libraries/WiFi/utility/SimpleLinkCallbacks.cpp')
    with tempfile.TemporaryDirectory(prefix='pico-energia-wifi-test-') as directory:
        temp = Path(directory)
        for folder in ('driverlib', 'inc'):
            (temp / folder).mkdir()
        (temp / 'simplelink.h').write_text(SIMPLELINK)
        (temp / 'WiFi.h').write_text(WIFI)
        (temp / 'Energia.h').write_text('#include "WiFi.h"\n')
        (temp / 'WiFiClient.h').write_text('#include "WiFi.h"\nclass WiFiClient { public: virtual int connect(IPAddress,uint16_t); };\n')
        (temp / 'SLFS.h').write_text(SLFS)
        for path in ('driverlib/prcm.h', 'driverlib/shamd5.h', 'inc/hw_memmap.h'):
            (temp / path).write_text(DRIVER)
        callbacks = temp / 'callbacks.cpp'
        callbacks.write_text('#include "WiFi.h"\n#include <string.h>\n#if defined(sl_WlanEvtHdlr)\n' + callback_patch['after'])
        flags = ['-Wall', '-Wextra', '-Werror', '-g', '-O1', '-DPICO_STANDALONE=1',
                 '-ffunction-sections', '-fdata-sections', '-fsanitize=address,undefined',
                 '-fno-omit-frame-pointer', '-I' + str(temp), '-I' + str(root), '-I' + str(root / 'include')]
        network = temp / 'network.o'
        subprocess.run(['gcc', '-std=c11', *flags, '-c', str(root / 'src/network/network.c'), '-o', str(network)], check=True)
        control = temp / 'control.o'
        subprocess.run(['gcc', '-std=c11', *flags, '-c', str(root / 'src/local_control.c'), '-o', str(control)], check=True)
        executable = temp / 'wifi-tests'
        subprocess.run(['g++', '-std=c++11', *flags, str(root / 'tests/energia_wifi_fixture.cpp'),
                        str(callbacks), str(network), str(control), '-Wl,--gc-sections', '-lm', '-o', str(executable)], check=True)
        env = dict(os.environ)
        env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
        subprocess.run([str(executable)], check=True, env=env, timeout=30)


if __name__ == '__main__':
    main()
