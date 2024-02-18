#include "bootloader.h"

#include <stdint.h>
#include <stddef.h>
#include <checksum.h>
#include <stdio.h>
#include <string.h>

#include "io/serialwire.h"
#include "io/gpio.h"
#include "utils/hexdump.h"
#include "flash.h"
#include "utils/millis.h"
#include "secureboot.h"

uint8_t deviceInfo[9] = {0x34, 0x37, 0x31, 0x00, 0x1f, 0x06, 0x06, 0x01, 0x30};

typedef union
{
    uint16_t word;
    uint8_t bytes[2];
} uint8_16_u;

bool checkCrc(uint8_t *pBuff, uint16_t length)
{
    uint8_16_u receivedCrc16;

    uint16_t calculatedCrc16 = crc_16(pBuff, length - 2);

    receivedCrc16.bytes[0] = pBuff[length - 2];
    receivedCrc16.bytes[1] = pBuff[length - 1];
    return receivedCrc16.word == calculatedCrc16;
}

void makeCrc(uint8_t *pBuff, uint16_t length)
{
    uint8_16_u calculatedCrc16;

    calculatedCrc16.word = crc_16(pBuff, length);
    memcpy(pBuff + length, calculatedCrc16.bytes, 2);
}

static bool receiveBuffer = false;
static uint16_t bufferSize = 0;

int getCommandBytes(uint8_t command)
{
    switch(command) 
    {
        case CMD_KEEP_ALIVE:
            return 0;

        case CMD_RUN:
            return 0;

        case CMD_SET_BUFFER:
            return 3;

        case CMD_SET_ADDRESS:
            return 3;

        case CMD_PROG_FLASH:
            return 1;

        case CMD_VERIFY_FLASH:
            return 1;

        default:
            return -1;
    }
}

void processCmd(uint8_t *packet, size_t packetSize)
{
    uint8_t resp[256];
    static uint32_t address = 0;
    static uint8_t buffer[1024];

    if(receiveBuffer)
    {
        if(packetSize == bufferSize)
        {
            receiveBuffer = false;
            memcpy(buffer, packet, packetSize);
            resp[0] = ACK;
            writeBuffer(resp, 1);
            //printf("receiveBuffer, size=%d\n", packetSize);
            return;
        }
    }

    uint8_t command = packet[0];
    switch(command) 
    {
        case CMD_KEEP_ALIVE:
            //printf("CMD_KEEP_ALIVE\n");
            resp[0] = NACK_BAD_COMMAND;
            writeBuffer(resp, 1);
            break;

        case CMD_RUN:
            //printf("CMD_RUN\n");
            resp[0] = ACK;
            writeBuffer(resp, 1);
            break;

        case CMD_SET_BUFFER:
            //printf("CMD_SET_BUFFER: %d\n", bufferSize);
            //resp[0] = ACK;
            receiveBuffer = true;
            bufferSize = (packet[2] << 8) | packet[3];
            //writeBuffer(resp, 1);
            break;

        case CMD_SET_ADDRESS:
            address = 0x08000000 + (packet[2] << 8 | packet[3]);
            resp[0] = ACK;
            writeBuffer(resp, 1);
            //printf("CMD_SET_ADDRESS: %08x\n", (unsigned int) address);
            break;

        case CMD_PROG_FLASH:
            //printf("CMD_PROG_FLASH\n");
            if(secureboot_write(address, bufferSize, buffer))
            {
                resp[0] = ACK;
            }
            else if(bl_is_valid_app_address(address) && bl_is_valid_app_address(address + bufferSize - 1))
            {
                //printf("CMD_PROG_FLASH: valid address\n");
                resp[0] = ACK;
                flash_write(buffer, bufferSize, address);
            }
            else
            {
                printf("CMD_PROG_FLASH: invalid address\n");
                resp[0] = NACK_BAD_ADDRESS;
            }
            writeBuffer(resp, 1);
            break;

        case CMD_VERIFY_FLASH:
            //printf("CMD_VERIFY_FLASH\n");
            bufferSize = packet[1];
            if(secureboot_read(address, bufferSize, resp))
            {
                makeCrc(resp, bufferSize);
                resp[bufferSize + 2] = ACK;
                writeBuffer(resp, bufferSize + 3);
            }
            else if(bl_is_valid_app_address(address) && bl_is_valid_app_address(address + bufferSize - 1))
            {
                //printf("CMD_VERIFY_FLASH: valid address\n");
                memcpy(resp, (const void*) address, bufferSize);
                makeCrc(resp, bufferSize);
                resp[bufferSize + 2] = ACK;
                writeBuffer(resp, bufferSize + 3);
            }
            else
            {
                printf("CMD_VERIFY_FLASH: invalid address %08x\n", (unsigned int) address);
                resp[0] = NACK_BAD_ADDRESS;
                writeBuffer(resp, 1);
            }
            break;

        default:
            resp[0] = NACK_BAD_COMMAND;
            writeBuffer(resp, 1);
            printf("NAK: bad cmd = %02x\n", command);
    }
}

bool bl_is_valid_app_address(intptr_t address)
{
    return address >= FLASH_APPLICATION_START_ADDRESS && address < FLASH_APPLICATION_END_ADDRESS;
}

uint8_t bootInit[] = {0x0D,'B','L','H','e','l','i',0xF4,0x7D};
bool connected = false;
bool check_boot_init(uint8_t *buf, size_t len)
{
    if(len < sizeof(bootInit))
    {
        return false;
    }
    for(int i = 0; i < (len - sizeof(bootInit) + 1); i++)
    {
        if(memcmp(buf + i, bootInit, sizeof(bootInit)) == 0)
        {
            return true;
        }
    }
    return false;
}

bool rebootPending = false;

void bl_reboot()
{
    rebootPending = true;
}

int bl_main()
{
    printf("initializing\n");
    secureboot_startup();
    //uint8_t oldc = 0;
    //printf("testing 5 sec\n");
    // uint32_t mi = millis();
    // while (millis() - mi < 5000)
    // {
    //     printf("testing\n");
    // }
    pinInit();
	

    uint8_t resp[256];
    uint32_t bytesToReceive;

    while (1)
    {
        uint8_t buf[512];
        size_t outLen;
        pinSetInputPullDownMode();
        if(!readChar(buf))
        {
            continue;
        }
        if(receiveBuffer)
        {
            bytesToReceive = bufferSize + 2;
        }
        else if(buf[0] == 0x00)
        {
            bytesToReceive = 20;
        }
        else
        {
            int cmdBytes = getCommandBytes(buf[0]);
            //printf("command = %02x, bytes = %d\n", buf[0], cmdBytes);
            if(cmdBytes == -1)
            {
                resp[0] = NACK_BAD_COMMAND;
                writeBuffer(resp, 1);
                continue;
            }
            bytesToReceive = cmdBytes + 2;
        }
        if(!readBuffer(buf + 1, bytesToReceive, &outLen))
        {
            continue;
        }
        outLen++;
        if (check_boot_init(buf, outLen))
        {
            connected = true;
            writeBuffer(deviceInfo, 9);
            //printf("boot init\n");
            continue;
        }
        if (checkCrc(buf, outLen))
        {
            processCmd(buf, outLen - 2);
            //hexdump(buf, outLen);
            continue;
        }
        if(rebootPending)
        {
            bl_target_reboot();
        }
        //printf("invalid bytes\n");
        //hexdump(buf, outLen);
        resp[0] = NACK_BAD_CRC;
        writeBuffer(resp, 1);
    }

    return 0;
}