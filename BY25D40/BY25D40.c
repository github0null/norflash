#include "BY25D40.h"

/**
 * Status register
 * ---+----+-----+-----+-----+-----+-----+-----+
 * S7 | S6 |  S5 |  S4 |  S3 |  S2 | S1  | S0  |
 * ---+----+-----+-----+-----+-----+-----+-----+
 * SRP|  / |  /  | BP2 | BP1 | BP0 | WEL | WIP |
 * ---+----+-----+-----+-----+-----+-----+-----+
*/

#define STATUS_WR_BUSY 0x01
#define STATUS_WR_ENABLE 0x02
#define STATUS_WR_PROTECT 0x80
#define GET_PROTECT_BLOCK(status) ((0x07) & (status >> 2))

/**
 * -----------------------------------------------------------------------------------
 * Instruction Name     Byte1   Byte2       Byte3       Byte4       Byte5       Byte6 
 * -----------------------------------------------------------------------------------
 * Write Enable         06H      
 * Write Disable        04H      
 * Read Status          05H     (S7-S0)     
 * Write Status         01H     (S7-S0)     
 * Read Data            03H     A23-A16     A15-A8      A7-A0       (D7-D0)     Nextbyte 
 * Fast Read            0BH     A23-A16     A15-A8      A7-A0       dummy       (D7-D0) 
 * Dual Fast Read       3BH     A23-A16     A15-A8      A7-A0       dummy       (D7-D0)(1) 
 * Page Program         02H     A23-A16     A15-A8      A7-A0       (D7-D0)     Nextbyte 
 * Page Program         F2H     A23-A16     A15-A8      A7-A0       (D7-D0)     Nextbyte 
 * Sector Erase         20H     A23-A16     A15-A8      A7-A0   
 * Block Erase(32K)     52H     A23-A16     A15-A8      A7-A0   
 * Block Erase(64K)     D8H     A23-A16     A15-A8      A7-A0   
 * Chip Erase           C7/60H      
 * Deep Power-Down      B9H      
 * Wakeup               ABH     dummy       dummy       dummy       (ID7-ID0)  
 * Wakeup               ABH      
 * Vendor/Device_ID     90H     dummy       dummy       00H         (M7-M0)     (ID7-ID0) 
 * JEDEC_ID             9FH     (M7-M0)     (ID15-ID8) (ID7-ID0) 
 * 64 bit Unique ID     4BH     dummy       dummy       dummy       dummy       (bit63~bit0)  
*/

#define CMD_NOP 0x00

#define CMD_WR_EN 0x06
#define CMD_WR_DIS 0x04

#define CMD_RD_STATUS 0x05
#define CMD_WR_STATUS 0x01

#define CMD_RD_DATA 0x03
#define CMD_WR_DATA 0x02
/* 
#define CMD_ERASE_SECTOR 0x20
#define CMD_ERASE_BLOCK 0xD8
#define CMD_ERASE_CHIP 0x60 */

#define CMD_GOTO_SLEEP 0xB9
#define CMD_WAKEUP 0xAB

#define CMD_RD_DEV_ID 0x90
#define CMD_RD_JEDEC_ID 0x9F
#define CMD_RD_UNIQUE_ID 0x4B

//---------- internal macro --------------

#define CS_LOW() BY25D40_CS_LOW
#define CS_HIGH() BY25D40_CS_HIGH

#define WP_LOW() BY25D40_WP_LOW
#define WP_HIGH() BY25D40_WP_HIGH

//------------------- internal func -------------------

BY25D40_SPIHook SPI_SendByte;

#define EnableWrite()        \
    CS_LOW();                \
    SPI_SendByte(CMD_WR_EN); \
    CS_HIGH()

void WaitBusy()
{
    CS_LOW();
    SPI_SendByte(CMD_RD_STATUS);
    while ((SPI_SendByte(CMD_NOP) & STATUS_WR_BUSY))
        ;
    CS_HIGH();
}

void SendAddr(uint32_t addr)
{
    SPI_SendByte((uint8_t)((addr >> 16) & 0xFFU));
    SPI_SendByte((uint8_t)((addr >> 8) & 0xFFU));
    SPI_SendByte((uint8_t)(addr & 0xFFU));
}

//-----------------------------------------------

void BY25D40_InitHook(BY25D40_SPIHook spiHook)
{
    SPI_SendByte = spiHook;
}

uint8_t BY25D40_ReadByte(uint32_t addr)
{
    uint8_t dat;
    WaitBusy();
    CS_LOW();
    SPI_SendByte(CMD_RD_DATA);
    SendAddr(addr);
    dat = SPI_SendByte(CMD_NOP);
    CS_HIGH();
    return dat;
}

uint16_t BY25D40_ReadWord(uint32_t addr)
{
    uint16_t dat;
    WaitBusy();
    CS_LOW();
    SPI_SendByte(CMD_RD_DATA);
    SendAddr(addr);
    dat = SPI_SendByte(CMD_NOP);
    dat |= (((uint16_t)SPI_SendByte(CMD_NOP)) << 8);
    CS_HIGH();
    return dat;
}

void BY25D40_ReadBytes(uint32_t addr, uint8_t *list, uint8_t size)
{
    uint8_t i;
    WaitBusy();
    CS_LOW();
    SPI_SendByte(CMD_RD_DATA);
    SendAddr(addr);
    for (i = 0; i < size; i++)
    {
        list[i] = SPI_SendByte(CMD_NOP);
    }
    CS_HIGH();
}

void BY25D40_WriteByte(uint32_t addr, uint8_t dat)
{
    WaitBusy();
    EnableWrite();
    CS_LOW();
    SPI_SendByte(CMD_WR_DATA);
    SendAddr(addr);
    SPI_SendByte(dat);
    CS_HIGH();
}

void BY25D40_WriteWord(uint32_t addr, uint16_t word)
{
    WaitBusy();
    EnableWrite();
    CS_LOW();
    SPI_SendByte(CMD_WR_DATA);
    SendAddr(addr);
    SPI_SendByte((uint8_t)(word >> 8));
    SPI_SendByte((uint8_t)(word & 0x00FFU));
    CS_HIGH();
}

void BY25D40_WriteBytes(uint32_t addr, uint8_t *list, uint8_t len)
{
    uint8_t i;
    WaitBusy();
    EnableWrite();
    CS_LOW();
    SPI_SendByte(CMD_WR_DATA);
    SendAddr(addr);
    for (i = 0; i < len; i++)
    {
        SPI_SendByte(list[i]);
    }
    CS_HIGH();
}

void BY25D40_Erase(uint32_t addr, BY25D40_EraseType type)
{
    WaitBusy();
    EnableWrite();
    CS_LOW();
    SPI_SendByte((uint8_t)type);
    SendAddr(addr);
    CS_HIGH();
}

void BY25D40_LockProtectBits(void)
{
    uint8_t status;

    WaitBusy();
    CS_LOW();
    SPI_SendByte(CMD_RD_STATUS);
    status = SPI_SendByte(CMD_NOP);
    CS_HIGH();

    status |= 0x80;

    // write
    EnableWrite();
    CS_LOW();
    SPI_SendByte(CMD_WR_STATUS);
    SPI_SendByte(status);
    CS_HIGH();

    // enable lock
    WP_LOW();
}

void BY25D40_UnlockProtectBits(void)
{
    uint8_t status;

    // disable lock
    WP_HIGH();

    WaitBusy();
    CS_LOW();
    SPI_SendByte(CMD_RD_STATUS);
    status = SPI_SendByte(CMD_NOP);
    CS_HIGH();

    status &= 0x7F;

    // write
    EnableWrite();
    CS_LOW();
    SPI_SendByte(CMD_WR_STATUS);
    SPI_SendByte(status);
    CS_HIGH();
}

BY25D40_ProtectSize BY25D40_GetProtectSize(void)
{
    uint8_t status;
    WaitBusy();
    CS_LOW();
    SPI_SendByte(CMD_RD_STATUS);
    status = SPI_SendByte(CMD_NOP);
    CS_HIGH();
    return (BY25D40_ProtectSize)GET_PROTECT_BLOCK(status);
}

void BY25D40_SetProtectSize(BY25D40_ProtectSize size)
{
    uint8_t status;

    // read
    WaitBusy();
    CS_LOW();
    SPI_SendByte(CMD_RD_STATUS);
    status = SPI_SendByte(CMD_NOP);
    CS_HIGH();

    // clear old bits, set new bits
    status &= 0xE3;
    status |= ((size & 0x07) << 2);

    // write
    EnableWrite();
    CS_LOW();
    SPI_SendByte(CMD_WR_STATUS);
    SPI_SendByte(status);
    CS_HIGH();
}

void BY25D40_ClearProtection(void)
{
    BY25D40_SetProtectSize((BY25D40_ProtectSize)0x0);
}

void BY25D40_GotoSleep(void)
{
    WaitBusy();
    CS_LOW();
    SPI_SendByte(CMD_GOTO_SLEEP);
    CS_HIGH();
}

void BY25D40_Wakeup(void)
{
    CS_LOW();
    SPI_SendByte(CMD_WAKEUP);
    CS_HIGH();
}

void BY25D40_GetDeviceInfo(BY25D40_DeviceInfo *info)
{
    WaitBusy();

    // read device id
    CS_LOW();
    SPI_SendByte(CMD_RD_DEV_ID);
    SendAddr(0x0U);
    info->vendorID = SPI_SendByte(CMD_NOP);
    info->devID = SPI_SendByte(CMD_NOP);
    CS_HIGH();

    // read JEDEC info
    CS_LOW();
    SPI_SendByte(CMD_RD_JEDEC_ID);
    SPI_SendByte(CMD_NOP);
    info->memType = SPI_SendByte(CMD_NOP);
    info->capacity = SPI_SendByte(CMD_NOP);
    CS_HIGH();

    // read unique ID
    CS_LOW();
    SPI_SendByte(CMD_RD_UNIQUE_ID);
    // Dummy 4 byte
    SPI_SendByte(CMD_NOP);
    SPI_SendByte(CMD_NOP);
    SPI_SendByte(CMD_NOP);
    SPI_SendByte(CMD_NOP);
    // 64 bit data
    info->uniqueID[0] = SPI_SendByte(CMD_NOP);
    info->uniqueID[1] = SPI_SendByte(CMD_NOP);
    info->uniqueID[2] = SPI_SendByte(CMD_NOP);
    info->uniqueID[3] = SPI_SendByte(CMD_NOP);
    info->uniqueID[4] = SPI_SendByte(CMD_NOP);
    info->uniqueID[5] = SPI_SendByte(CMD_NOP);
    info->uniqueID[6] = SPI_SendByte(CMD_NOP);
    info->uniqueID[7] = SPI_SendByte(CMD_NOP);
    CS_HIGH();
}
