#ifndef _H_W25QXX
#define _H_W25QXX

/**
 * *****************************************************
 * 
 * implement pin interface at "W25QXX_conf.h"
 * 
 * *****************************************************
*/

#include <W25QXX_conf.h>

#if !defined(W25QXX_CS_HIGH) || !defined(W25QXX_CS_LOW)
#error "macro 'W25QXX_CS_HIGH()' and 'W25QXX_CS_LOW()' must be implemented"
#endif

#if !defined(W25QXX_WP_HIGH) || !defined(W25QXX_WP_LOW)
#error "macro 'W25QXX_WP_HIGH()' and 'W25QXX_WP_LOW()' must be implemented"
#endif

//***********************************************************

#include <stdint.h>

/**
 * functions define
*/

#define W25QXX_VENDOR_ID 0xEF

typedef uint8_t (*W25QXX_SPIHook)(uint8_t);

typedef struct
{
    uint8_t vendorID; // Fixed value: 0xEF
    uint8_t devID;
    uint8_t memType;
    uint8_t capacity;
    uint8_t uniqueID[8];
} W25QXX_DeviceInfo;

typedef enum
{
    W25QXX_PROTECT_NONE = 0x00U,    // 0
    W25QXX_PROTECT_1_OF_64 = 0x01U, // 1/64
    W25QXX_PROTECT_1_OF_32 = 0x02U, // 1/32
    W25QXX_PROTECT_1_OF_16 = 0x03U, // 1/16
    W25QXX_PROTECT_1_OF_8 = 0x04U,  // 1/8
    W25QXX_PROTECT_1_OF_4 = 0x05U,  // 1/4
    W25QXX_PROTECT_1_OF_2 = 0x06U,  // 1/2
    W25QXX_PROTECT_ALL = 0x07U      // all
} W25QXX_ProtectSize;

typedef enum
{
    W25QXX_ERASE_SECTOR = 0x20U,     // 4KB
    W25QXX_ERASE_HALF_BLOCK = 0x52U, // 32KB
    W25QXX_ERASE_BLOCK = 0xD8U,      // 64KB
    W25QXX_ERASE_CHIP = 0x60U        // ALL
} W25QXX_EraseType;

typedef enum
{
    W25QXX_ERR_OK = 0,
    W25QXX_ERR_FAILED = 1
} W25QXX_ErrorCode;

/**
 * Init W25QXX
*/
W25QXX_ErrorCode W25QXX_Init(W25QXX_SPIHook spiHook);

/**
 * read operations
*/

uint8_t W25QXX_ReadByte(uint32_t addr);
uint16_t W25QXX_ReadWord(uint32_t addr);
void W25QXX_ReadBytes(uint32_t addr, uint8_t *buf, uint32_t size);

/**
 * write operations
*/

void W25QXX_WriteByte(uint32_t addr, uint8_t dat);
void W25QXX_WriteWord(uint32_t addr, uint16_t word);
void W25QXX_WriteBytes(uint32_t addr, uint8_t *buf, uint32_t len);
void W25QXX_Erase(uint32_t addr, W25QXX_EraseType type);

/**
 * lock protection bits
*/

void W25QXX_LockProtectBits(void);
void W25QXX_UnlockProtectBits(void);

/**
 * block protection
*/

W25QXX_ProtectSize W25QXX_GetProtectSize(void);
void W25QXX_SetProtectSize(W25QXX_ProtectSize size);
void W25QXX_ClearProtection(void);

/**
 * deep sleep/wakeup
*/

void W25QXX_GotoSleep(void);
void W25QXX_Wakeup(void);

/**
 * get device information
*/
void W25QXX_GetDeviceInfo(W25QXX_DeviceInfo *info);

#endif
