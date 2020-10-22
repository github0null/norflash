#include "W25QXX.h"

#define PAGE_SIZE 256
#define SECTOR_SIZE 4096

#undef true
#define true 1

#undef false
#define false 0

/**
 * Status register
 * ---+-----+-----+-----+-----+-----+-----+-----+
 * S7 | S6  |  S5 |  S4 |  S3 |  S2 | S1  | S0  |
 * ---+-----+-----+-----+-----+-----+-----+-----+
 * SRP| SEC | TB  | BP2 | BP1 | BP0 | WEL | WIP |
 * ---+-----+-----+-----+-----+-----+-----+-----+
*/
#define STATUS_WR_BUSY 0x01
#define STATUS_WR_ENABLE 0x02
#define STATUS_TB_PROTECT 0x20
#define STATUS_SEC_PROTECT 0x40
#define STATUS_WR_PROTECT 0x80
#define GET_PROTECT_BLOCK(status) ((0x07) & (status >> 2))

#define CMD_NOP 0x00
#define CMD_WR_EN 0x06
#define CMD_WR_DIS 0x04

#define CMD_RD_STATUS 0x05
#define CMD_WR_STATUS 0x01
#define CMD_RD_STATUS_2 0x35
#define CMD_WR_STATUS_2 0x31

#define CMD_RD_DATA 0x03
#define CMD_WR_DATA 0x02

#define CMD_GOTO_SLEEP 0xB9
#define CMD_WAKEUP 0xAB

#define CMD_RD_DEV_ID 0x90
#define CMD_RD_JEDEC_ID 0x9F
#define CMD_RD_UNIQUE_ID 0x4B

//---------- internal macro --------------

#define CS_LOW() W25QXX_CS_LOW()
#define CS_HIGH() W25QXX_CS_HIGH()

#define WP_LOW() W25QXX_WP_LOW()
#define WP_HIGH() W25QXX_WP_HIGH()

//------------------- internal func -------------------

W25QXX_SPIHook SPI_SendByte;

void EnableWrite()
{
    CS_LOW();
    SPI_SendByte(CMD_WR_EN);
    CS_HIGH();
}

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
    SPI_SendByte((uint8_t)(addr >> 16));
    SPI_SendByte((uint8_t)(addr >> 8));
    SPI_SendByte((uint8_t)addr);
}

uint8_t ReadStatus(uint8_t cmd)
{
    uint8_t status;
    CS_LOW();
    SPI_SendByte(cmd);
    status = SPI_SendByte(CMD_NOP);
    CS_HIGH();
    return status;
}

void WriteStatus(uint8_t cmd, uint8_t dat)
{
    WaitBusy();
    EnableWrite();
    CS_LOW();
    SPI_SendByte(cmd);
    SPI_SendByte(dat);
    CS_HIGH();
}

uint8_t IsEmptyPage(uint32_t addr)
{
    uint16_t pageRemain = PAGE_SIZE - (addr % PAGE_SIZE);

    WaitBusy();
    CS_LOW();
    SPI_SendByte(CMD_RD_DATA);
    SendAddr(addr);

    while (pageRemain--)
    {
        if (SPI_SendByte(CMD_NOP) != 0xFF)
        {
            CS_HIGH();
            return false;
        }
    }

    CS_HIGH();

    return true;
}

void W25QXX_WritePage(uint32_t addr, uint8_t *buf, uint16_t size)
{
    uint16_t index;
    WaitBusy();
    EnableWrite();
    CS_LOW();
    SPI_SendByte(CMD_WR_DATA);
    SendAddr(addr);
    for (index = 0; index < size; index++)
        SPI_SendByte(buf[index]);
    CS_HIGH();
}

//-----------------------------------------------

W25QXX_ErrorCode W25QXX_Init(W25QXX_SPIHook spiHook)
{
    W25QXX_DeviceInfo devInfo;
    uint8_t status;

    SPI_SendByte = spiHook;

    W25QXX_GetDeviceInfo(&devInfo);
    if (devInfo.vendorID != W25QXX_VENDOR_ID)
        return W25QXX_ERR_FAILED;

    // set SEC, TAB bits to 0
    status = ReadStatus(CMD_RD_STATUS);
    WriteStatus(CMD_WR_STATUS, status & 0x1F);

    // set CMP bit to 0
    status = ReadStatus(CMD_RD_STATUS_2);
    WriteStatus(CMD_WR_STATUS_2, status & 0xBF);

    return W25QXX_ERR_OK;
}

uint8_t W25QXX_ReadByte(uint32_t addr)
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

void W25QXX_WriteByte(uint32_t addr, uint8_t dat)
{
    if (!IsEmptyPage(addr)) // is not a empty page
        W25QXX_Erase(addr, W25QXX_ERASE_SECTOR);

    WaitBusy();
    EnableWrite();
    CS_LOW();
    SPI_SendByte(CMD_WR_DATA);
    SendAddr(addr);
    SPI_SendByte(dat);
    CS_HIGH();
}

uint16_t W25QXX_ReadWord(uint32_t addr)
{
    uint16_t dat = W25QXX_ReadByte(addr + 1);
    return (dat << 8) | (uint16_t)W25QXX_ReadByte(addr);
}

void W25QXX_WriteWord(uint32_t addr, uint16_t word)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)word;
    buf[1] = (uint8_t)(word >> 8);
    W25QXX_WriteBytes(addr, buf, 2);
}

void W25QXX_ReadBytes(uint32_t addr, uint8_t *buf, uint32_t size)
{
    WaitBusy();
    CS_LOW();
    SPI_SendByte(CMD_RD_DATA);
    SendAddr(addr);

    while (size--)
    {
        *buf = SPI_SendByte(CMD_NOP);
        buf++;
    }

    CS_HIGH();
}

void W25QXX_WriteBytes(uint32_t addr, uint8_t *buf, uint32_t size)
{
    uint16_t pageRemain = PAGE_SIZE - (addr % PAGE_SIZE);
    uint32_t secAddr;

    for (secAddr = addr - (addr % SECTOR_SIZE); secAddr < addr + size; secAddr += SECTOR_SIZE)
        W25QXX_Erase(secAddr, W25QXX_ERASE_SECTOR);

    if (size <= pageRemain)
        pageRemain = size;

    while (1)
    {
        W25QXX_WritePage(addr, buf, pageRemain);

        if (size == pageRemain)
        {
            break; // done
        }
        else // size > pageRemain
        {
            buf += pageRemain;
            addr += pageRemain;
            size -= pageRemain;

            if (size > 256)
                pageRemain = 256;
            else
                pageRemain = size;
        }
    }
}

void W25QXX_Erase(uint32_t addr, W25QXX_EraseType type)
{
    WaitBusy();
    EnableWrite();
    CS_LOW();
    SPI_SendByte((uint8_t)type);
    SendAddr(addr);
    CS_HIGH();
}

void W25QXX_LockProtectBits(void)
{
    WriteStatus(CMD_WR_STATUS, ReadStatus(CMD_RD_STATUS) | 0x80);
    WP_LOW(); // lock
}

void W25QXX_UnlockProtectBits(void)
{
    WP_HIGH(); // Unlock
    WriteStatus(CMD_WR_STATUS, ReadStatus(CMD_RD_STATUS) | 0x7F);
}

W25QXX_ProtectSize W25QXX_GetProtectSize(void)
{
    return (W25QXX_ProtectSize)GET_PROTECT_BLOCK(ReadStatus(CMD_RD_STATUS));
}

void W25QXX_SetProtectSize(W25QXX_ProtectSize size)
{
    uint8_t status = ReadStatus(CMD_RD_STATUS);

    // clear old bits, set new bits
    status &= 0xE3;
    status |= ((size & 0x07) << 2);

    WriteStatus(CMD_WR_STATUS, status);
}

void W25QXX_ClearProtection(void)
{
    W25QXX_SetProtectSize((W25QXX_ProtectSize)0x0);
}

void W25QXX_GotoSleep(void)
{
    WaitBusy();
    CS_LOW();
    SPI_SendByte(CMD_GOTO_SLEEP);
    CS_HIGH();
}

void W25QXX_Wakeup(void)
{
    CS_LOW();
    SPI_SendByte(CMD_WAKEUP);
    CS_HIGH();
}

void W25QXX_GetDeviceInfo(W25QXX_DeviceInfo *info)
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
