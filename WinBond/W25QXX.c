#include "W25QXX.h"

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

W25QXX_SPIHook __spi_send_byte;

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

void W25QXX_WritePage(uint32_t addr, uint8_t *buf, uint16_t size)
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

W25QXX_ErrorCode W25QXX_Init(W25QXX_SPIHook spiHook)
{
    W25QXX_DeviceInfo devInfo;
    uint8_t status;

    __spi_send_byte = spiHook;

    W25QXX_GetDeviceInfo(&devInfo);

#ifdef W25QXX_DEV_ID
    if (devInfo.vendorID != W25QXX_VENDOR_ID || devInfo.devID != W25QXX_DEV_ID)
        return W25QXX_ERR_FAILED;
#else
    if (devInfo.vendorID != W25QXX_VENDOR_ID)
        return W25QXX_ERR_FAILED;
#endif

    // set SEC, TAB bits to 0
    status = ReadStatus(CMD_RD_STATUS);
    WriteStatus(CMD_WR_STATUS, status & 0x1F);

    // set CMP bit to 0
    status = ReadStatus(CMD_RD_STATUS_2);
    WriteStatus(CMD_WR_STATUS_2, status & 0xBF);

    return W25QXX_ERR_NONE;
}

uint8_t W25QXX_ReadByte(uint32_t addr)
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

void W25QXX_WriteByte(uint32_t addr, uint8_t dat)
{
    if (!IsEmptyPage(addr)) // is not a empty page
        W25QXX_Erase(addr, W25QXX_ERASE_SECTOR);

    WaitBusy();
    EnableWrite();
    CS_LOW();
    __spi_send_byte(CMD_WR_DATA);
    SendAddr(addr);
    __spi_send_byte(dat);
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
    __spi_send_byte(CMD_RD_DATA);
    SendAddr(addr);

    while (size--)
    {
        *buf = __spi_send_byte(CMD_NOP);
        buf++;
    }

    CS_HIGH();
}

void W25QXX_WriteBytes(uint32_t addr, uint8_t *buf, uint32_t size)
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
            W25QXX_Erase(chkAddr, W25QXX_ERASE_SECTOR);

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

            // caculate next write size
            if (size > PAGE_SIZE)
                pageRemain = PAGE_SIZE;
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
    __spi_send_byte((uint8_t)type);
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
    __spi_send_byte(CMD_GOTO_SLEEP);
    CS_HIGH();
}

void W25QXX_Wakeup(void)
{
    CS_LOW();
    __spi_send_byte(CMD_WAKEUP);
    CS_HIGH();
}

void W25QXX_GetDeviceInfo(W25QXX_DeviceInfo *info)
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
