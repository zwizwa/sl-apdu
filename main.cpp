/* Dump Saleae Logic output to stdout.
   Adapted from SaleaeDeviceSdk-1.1.14/source/ConsoleDemo.cpp */

#include <SaleaeDeviceApi.h>

#include <memory>
#include <iostream>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "apdu.h"  // protocol parser

void __stdcall OnConnect( U64 device_id, GenericInterface* device_interface, void* user_data );
void __stdcall OnDisconnect( U64 device_id, void* user_data );
void __stdcall OnReadData( U64 device_id, U8* data, U32 data_length, void* user_data );
void __stdcall OnWriteData( U64 device_id, U8* data, U32 data_length, void* user_data );
void __stdcall OnError( U64 device_id, void* user_data );

LogicInterface* gDeviceInterface = NULL;

#define LOG(...) fprintf(stderr, __VA_ARGS__)

U64 gLogicId = 0;
U32 gSampleRateHz = 4000000;

#define CMD_READ  thread_cmd[0]
#define CMD_WRITE thread_cmd[1]
int thread_cmd[2];

#define CMD_NOP   0
#define CMD_ERROR 1

int log_fd = -1;

struct apdu_state *apdu_sink;

int main( int argc, char *argv[] ) {
    DevicesManagerInterface::RegisterOnConnect( &OnConnect );
    DevicesManagerInterface::RegisterOnDisconnect( &OnDisconnect );
    DevicesManagerInterface::BeginConnect();
    
    // apdu_sink = apdu_new(16); // 3.817 MHz -> 238k baud

    apdu_sink = apdu_new(4000000,   // logic sample freq
                         3817000),  // card clock

    LOG("Samplerate %d\n", gSampleRateHz);

    if (0) {
        const char *log_file = "/tmp/sl-apdu.bin";
        log_fd = open(log_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        LOG("logic trace log file = %s\n", log_file);
    }

    // Use unix pipe for inter-thread communication.
    pipe(thread_cmd);
    int cmd;
    while(sizeof(int) == read(CMD_READ, &cmd, sizeof(cmd))) {
        switch(cmd) {
        default:
            LOG("Ignoring unknown command %d\n", cmd);
        case CMD_NOP:
            break;
        case CMD_ERROR:
            /* On error, we just restart. */
            if (!gDeviceInterface) {
                LOG("CMD_ERROR: gDeviceInterface == NULL\n");
            }
            else {
                LOG("CMD_ERROR: starting read\n");
                gDeviceInterface->ReadStart();
            }
            break;
        }
    }
}

void write_fd(int fd, U8* data, U32 data_length) {
    int i = 0;
    while (i < data_length) {
        int remaining = data_length - i;
        int rv = write(fd, data + i, remaining);
        if (rv > 0) {
            i += rv;
        }
        else {
            LOG("stdout write error %d\n", rv);
            exit(1);
        }
    }
}


void __stdcall OnReadData( U64 device_id, U8* data, U32 data_length, void* user_data ) {
    // LOG(".");

    // Parse APDU data on 8-bit bus: 0=IO,1=RST,2=VCC.
    apdu_push8(apdu_sink, data, data_length);

    // Optionally log to fd.
    if (log_fd >= 0) {
        write_fd(log_fd, data, data_length);
    }



    // We own, so need to delete.
    DevicesManagerInterface::DeleteU8ArrayPtr( data );
}

void __stdcall OnWriteData( U64 device_id, U8* data, U32 data_length, void* user_data )
{
    /* Not used */
}

void __stdcall OnError( U64 device_id, void* user_data )
{
    LOG("ERROR\n");
    // Notify main thread
    int cmd = CMD_ERROR;
    write(CMD_WRITE, &cmd, sizeof(cmd));
}

void __stdcall OnDisconnect( U64 device_id, void* user_data ) {
    if( device_id == gLogicId ) {
        LOG("Disconnect %08x\n", device_id);
        gDeviceInterface = NULL;
    }
}


void __stdcall OnConnect( U64 device_id, GenericInterface* device_interface, void* user_data )
{
    if( dynamic_cast<LogicInterface*>( device_interface ) != NULL ) {
        LOG("Connect %08x\n", device_id);

        gDeviceInterface = (LogicInterface*)device_interface;
        gLogicId = device_id;
        
        gDeviceInterface->RegisterOnReadData( &OnReadData );
        gDeviceInterface->RegisterOnWriteData( &OnWriteData );
        gDeviceInterface->RegisterOnError( &OnError );
        
        gDeviceInterface->SetSampleRateHz( gSampleRateHz );

        // Start automatically
        gDeviceInterface->ReadStart();

    }
}

