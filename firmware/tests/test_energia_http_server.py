#!/usr/bin/env python3
"""Test native HTTP adapter control flow using explicit SimpleLink I/O doubles.

The production adapter is also compiled against the pinned SDK by the firmware
build. These tests establish error/handle/nonblocking contracts, not real RF,
flash durability, interrupt latency, or socket timing on a physical CC3200.
"""
import os
from pathlib import Path
import subprocess
import tempfile

SIMPLELINK = r'''
#ifndef SIMPLELINK_TEST_H
#define SIMPLELINK_TEST_H
#include <stdint.h>
typedef int16_t _i16; typedef uint16_t _u16; typedef uint8_t _u8;
typedef int32_t _i32; typedef uint32_t _u32; typedef uint16_t SlSocklen_t;
typedef struct { _u32 NonblockingEnabled; } SlSockNonblocking_t;
typedef struct { _u16 family; char data[14]; } SlSockAddr_t;
typedef struct { _u16 sin_family, sin_port; struct { _u32 s_addr; } sin_addr; } SlSockAddrIn_t;
typedef struct { _u32 FileLen; } SlFsFileInfo_t;
#define SL_EAGAIN (-11)
#define SL_AF_INET 2
#define SL_SOCK_STREAM 1
#define SL_IPPROTO_TCP 6
#define SL_SOL_SOCKET 1
#define SL_SO_NONBLOCKING 24
#define SL_NET_APP_HTTP_SERVER_ID 1
#define FS_MODE_OPEN_READ 0
#define SL_FS_ERR_FILE_NOT_EXISTS (-11)
#ifdef __cplusplus
extern "C" {
#endif
_i16 sl_NetAppStop(_u32);
_i16 sl_Socket(_i16,_i16,_i16);
_i16 sl_SetSockOpt(_i16,_i16,_i16,const void*,SlSocklen_t);
_u16 sl_Htons(_u16);
_i16 sl_Bind(_i16,const SlSockAddr_t*,SlSocklen_t);
_i16 sl_Listen(_i16,_i16);
_i16 sl_Accept(_i16,SlSockAddr_t*,SlSocklen_t*);
_i16 sl_Recv(_i16,void*,_i16,_i16);
_i16 sl_Send(_i16,const void*,_i16,_i16);
_i16 sl_Close(_i16);
_i16 sl_FsGetInfo(const _u8*,_u32,SlFsFileInfo_t*);
_i32 sl_FsOpen(const _u8*,_u32,_u32*,_i32*);
_i32 sl_FsRead(_i32,_u32,_u8*,_u32);
_i16 sl_FsClose(_i32,const _u8*,const _u8*,_u32);
#ifdef __cplusplus
}
#endif
#endif
'''
WIFI = r'''
#ifndef WIFI_TEST_H
#define WIFI_TEST_H
#include <stdint.h>
enum { WL_CONNECTED=3, WL_AP_MODE=8 };
class WiFiClass { public: static bool _initialized; uint8_t status(); };
extern WiFiClass WiFi;
uint32_t millis();
#endif
'''


def main():
    root = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix='pico-energia-http-test-') as directory:
        temp = Path(directory)
        (temp / 'utility').mkdir()
        (temp / 'utility/simplelink.h').write_text(SIMPLELINK)
        for name in ['fs.h', 'netapp.h', 'socket.h']:
            (temp / 'utility' / name).write_text('#include "simplelink.h"\n')
        (temp / 'WiFi.h').write_text(WIFI)
        flags = ['-Wall', '-Wextra', '-Werror', '-g', '-O1', '-fsanitize=address,undefined',
                 '-fno-omit-frame-pointer', '-I' + str(temp), '-I' + str(root / 'include')]
        object_file = temp / 'http.o'
        subprocess.run(['gcc', '-std=c11', *flags, '-c', str(root / 'src/http_server.c'),
                        '-o', str(object_file)], check=True)
        executable = temp / 'native-http-tests'
        subprocess.run(['g++', '-std=c++11', *flags, str(root / 'platform/energia_http_server.cpp'),
                        str(root / 'tests/energia_http_fixture.cpp'), str(object_file),
                        '-o', str(executable)], check=True)
        env = dict(os.environ)
        env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
        subprocess.run([str(executable)], check=True, env=env)


if __name__ == '__main__':
    main()
