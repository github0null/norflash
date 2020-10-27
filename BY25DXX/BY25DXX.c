#include "BY25DXX.h"

#define PAGE_SIZE 256
#define SECTOR_SIZE 4096

#undef true
#define true 1

#undef false
#define false 0

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

#define CMD_RD_DATA 0x03
#define CMD_WR_DATA 0x02

#define CMD_GOTO_SLEEP 0xB9
#define CMD_WAKEUP 0xAB

#define CMD_RD_DEV_ID 0x90
#define CMD_RD_JEDEC_ID 0x9F
#define CMD_RD_UNIQUE_ID 0x4B

//---------- internal macro --------------

#define CS_LOW() BY25DXX_CS_LOW()
#define CS_HIGH() BY25DXX_CS_HIGH()

#define WP_LOW() BY25DXX_WP_LOW()
#define WP_HIGH() BY25DXX_WP_HIGH()

//------------------- internal func -------------------

BY25DXX_SPIHook __spi_send_byte;

void EnableWrite()
{
    CS_LOW();
    __spi_send_byte(CMD_WR_EN);
    CS_HIGH();
}

void WaitBusy()
{
    CS_LOW();
    __spi_send_byte(CMD_RD_STATUS);
    while ((__spi_send_byte(CMD_NOP) & STATUS_WR_BUSY))
        ;
    CS_HIGH();
}

void SendAddr(uint32_t addr)
{
    __spi_send_byte((uint8_t)(addr >> 16));
    __spi_send_byte((uint8_t)(addr >> 8));
    __spi_send_byte((uint8_t)addr);
}

uint8_t ReadStatus(uint8_t cmd)
{
    uint8_t status;
    CS_LOW();
    __spi_send_byte(cmd);
    status = __spi_send_byte(CMD_NOP);
    CS_HIGH();
    return status;
}

void WriteStatus(uint8_t cmd, uint8_t dat)
{
    WaitBusy();
    EnableWrite();
    CS_LOW();
    __spi_send_byte(cmd);
    __spi_send_byte(dat);
    CS_HIGH();
}

uint8_t IsEmptyPage(uint32_t addr)
{
    uint16_t pageRemain = PAGE_SIZE - (addr % PAGE_SIZE);

    WaitBusy();
    CS_LOW();
    __spi_send_byte(CMD_RD_DATA);
    SendAddr(addr);

    while (pageRemain--)
    {
        if (__spi_send_byte(CMD_NOP) != 0xFF)
        {
            CS_HIGH();
            return false;
        }
    }

    CS_HIGH();

    return true;
}

uint8_t IsEmptySector(uint32_t addr, uint32_t size)
{
    uint32_t secRemain = SECTOR_SIZE - (addr % SECTOR_SIZE);

    if (secRemain > size)
        secRemain = size;

    WaitBusy();
    CS_LOW();
    __spi_send_byte(CMD_RD_DATA);
    SendAddr(addr);

    while (secRemain--)
    {
        if (__spi_send_byte(CMD_NOP) != 0xFF)
        {
            CS_HIGH();
            return false;
        }
    }

    CS_HIGH();

    return true;
}

void BY25DXX_WritePage(uint32_t addr, uint8_t *buf, uint16_t size)
{
    uint16_t index;
    WaitBusy();
    EnableWrite();
    CS_LOW();
    __spi_send_byte(CMD_WR_DATA);
    SendAddr(addr);
    for (index = 0; index < size; index++)
        __spi_send_byte(buf[index]);
    CS_HIGH();
}

//-----------------------------------------------

BY25DXX_ErrorCode BY25DXX_Init(BY25DXX_SPIHook spiHook)
{
    BY25DXX_DeviceInfo devInfo;

    __spi_send_byte = spiHook;

    BY25DXX_GetDeviceInfo(&devInfo);

#ifdef BY25DXX_DEV_ID
    if (devInfo.vendorID != BY25DXX_VENDOR_ID || devInfo.devID != BY25DXX_DEV_ID)
        return BY25DXX_ERR_FAILED;
#else
    if (devInfo.vendorID != BY25DXX_VENDOR_ID)
        return BY25DXX_ERR_FAILED;
#endif

    return BY25DXX_ERR_NONE;
}

uint8_t BY25DXX_ReadByte(uint32_t addr)
{
    uint8_t dat;
    WaitBusy();
    CS_LOW();
    __spi_send_byte(CMD_RD_DATA);
    SendAddr(addr);
    dat = __spi_send_byte(CMD_NOP);
    CS_HIGH();
    return dat;
}

void BY25DXX_WriteByte(uint32_t addr, uint8_t dat)
{
    if (!IsEmptyPage(addr)) // is not a empty page
        BY25DXX_Erase(addr, BY25DXX_ERASE_SECTOR);

    WaitBusy();
    EnableWrite();
    CS_LOW();
    __spi_send_byte(CMD_WR_DATA);
    SendAddr(addr);
    __spi_send_byte(dat);
    CS_HIGH();
}

uint16_t BY25DXX_ReadWord(uint32_t addr)
{
    uint16_t dat = BY25DXX_ReadByte(addr + 1);
    return (dat << 8) | (uint16_t)BY25DXX_ReadByte(addr);
}

void BY25DXX_WriteWord(uint32_t addr, uint16_t word)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)word;
    buf[1] = (uint8_t)(word >> 8);
    BY25DXX_WriteBytes(addr, buf, 2);
}

void BY25DXX_ReadBytes(uint32_t addr, uint8_t *buf, uint32_t size)
{
    WaitBusy();
    CS_LOW();
    __spi_send_byte(CMD_RD_DATA);
    SendAddr(addr);

    while (size--)
    {
        *buf = __spi_send_byte(CMD_NOP);
        buf++;
    }

    CS_HIGH();
}

void BY25DXX_WriteBytes(uint32_t addr, uint8_t *buf, uint32_t size)
{
    uint16_t pageRemain = PAGE_SIZE - (addr % PAGE_SIZE);
    uint32_t sectorRemain = SECTOR_SIZE - (addr % SECTOR_SIZE);
    uint32_t chkAddr = addr, chkSize = size;

    // erase sector

    if (chkSize < sectorRemain)
        sectorRemain = chkSize;

    while (1)
    {
        if (!IsEmptySector(chkAddr, sectorRemain))
            BY25DXX_Erase(chkAddr, BY25DXX_ERASE_SECTOR);

        if (chkSize == sectorRemain)
        {
            break; // done
        }
        else // chkSize > sectorRemain
        {
            chkAddr += sectorRemain;
            chkSize -= sectorRemain;

            // caculate next check size
            if (chkSize > SECTOR_SIZE)
                sectorRemain = SECTOR_SIZE;
            else
                sectorRemain = chkSize;
        }
    }

    // write data

    if (size < pageRemain)
        pageRemain = size;

    while (1)
    {
        BY25DXX_WritePage(addr, buf, pageRemain);

        if (size == pageRemain)
        {
            break; // done
        }
        else // size > pageRemain
        {
            buf += pageRemain;
            addr += pageRemain;
            size -= pageRemain;

            // caculate next write size
            if (size > PAGE_SIZE)
                pageRemain = PAGE_SIZE;
            else
                pageRemain = size;
        }
    }
}

void BY25DXX_Erase(uint32_t addr, BY25DXX_EraseType type)
{
    WaitBusy();
    EnableWrite();
    CS_LOW();
    __spi_send_byte((uint8_t)type);
    SendAddr(addr);
    CS_HIGH();
}

void BY25DXX_LockProtectBits(void)
{
    WriteStatus(CMD_WR_STATUS, ReadStatus(CMD_RD_STATUS) | 0x80);
    WP_LOW(); // lock
}

void BY25DXX_UnlockProtectBits(void)
{
    WP_HIGH(); // Unlock
    WriteStatus(CMD_WR_STATUS, ReadStatus(CMD_RD_STATUS) | 0x7F);
}

BY25DXX_ProtectSize BY25DXX_GetProtectSize(void)
{
    return (BY25DXX_ProtectSize)GET_PROTECT_BLOCK(ReadStatus(CMD_RD_STATUS));
}

void BY25DXX_SetProtectSize(BY25DXX_ProtectSize size)
{
    uint8_t status = ReadStatus(CMD_RD_STATUS);

    // clear old bits, set new bits
    status &= 0xE3;
    status |= ((size & 0x07) << 2);

    WriteStatus(CMD_WR_STATUS, status);
}

void BY25DXX_ClearProtection(void)
{
    BY25DXX_SetProtectSize((BY25DXX_ProtectSize)0x0);
}

void BY25DXX_GotoSleep(void)
{
    WaitBusy();
    CS_LOW();
    __spi_send_byte(CMD_GOTO_SLEEP);
    CS_HIGH();
}

void BY25DXX_Wakeup(void)
{
    CS_LOW();
    __spi_send_byte(CMD_WAKEUP);
    CS_HIGH();
}

void BY25DXX_GetDeviceInfo(BY25DXX_DeviceInfo *info)
{
    WaitBusy();

    // read device id
    CS_LOW();
    __spi_send_byte(CMD_RD_DEV_ID);
    SendAddr(0x0U);
    info->vendorID = __spi_send_byte(CMD_NOP);
    info->devID = __spi_send_byte(CMD_NOP);
    CS_HIGH();

    // read JEDEC info
    CS_LOW();
    __spi_send_byte(CMD_RD_JEDEC_ID);
    __spi_send_byte(CMD_NOP);
    info->memType = __spi_send_byte(CMD_NOP);
    info->capacity = __spi_send_byte(CMD_NOP);
    CS_HIGH();

    // read unique ID
    CS_LOW();
    __spi_send_byte(CMD_RD_UNIQUE_ID);
    // Dummy 4 byte
    __spi_send_byte(CMD_NOP);
    __spi_send_byte(CMD_NOP);
    __spi_send_byte(CMD_NOP);
    __spi_send_byte(CMD_NOP);
    // 64 bit data
    info->uniqueID[0] = __spi_send_byte(CMD_NOP);
    info->uniqueID[1] = __spi_send_byte(CMD_NOP);
    info->uniqueID[2] = __spi_send_byte(CMD_NOP);
    info->uniqueID[3] = __spi_send_byte(CMD_NOP);
    info->uniqueID[4] = __spi_send_byte(CMD_NOP);
    info->uniqueID[5] = __spi_send_byte(CMD_NOP);
    info->uniqueID[6] = __spi_send_byte(CMD_NOP);
    info->uniqueID[7] = __spi_send_byte(CMD_NOP);
    CS_HIGH();
}
