#ifndef _H_BY25DXX
#define _H_BY25DXX

#include <BY25DXX_conf.h>
#include <stdint.h>

/**
 * *****************************************************
 * 
 * implement pin interface at "BY25DXX_conf.h"
 * 
 * *****************************************************
*/

#if !defined(BY25DXX_CS_HIGH) || !defined(BY25DXX_CS_LOW)
#error "macro 'BY25DXX_CS_HIGH()' and 'BY25DXX_CS_LOW()' must be implemented"
#endif

#if !defined(BY25DXX_WP_HIGH) || !defined(BY25DXX_WP_LOW)
#error "macro 'BY25DXX_WP_HIGH()' and 'BY25DXX_WP_LOW()' must be implemented"
#endif

//--------------------------------------------------------------

#define BY25DXX_VENDOR_ID 0x68

#if defined(BY25D40)
#define BY25DXX_DEV_ID 0x12
#else
#warning "You should define a BOYA_MICRO SPI Flash device series !"
#endif

//--------------------------------------------------------------

typedef uint8_t (*BY25DXX_SPIHook)(uint8_t);

typedef struct
{
    uint8_t vendorID; // Fixed value: 0x68
    uint8_t devID;
    uint8_t memType;
    uint8_t capacity;
    uint8_t uniqueID[8];
} BY25DXX_DeviceInfo;

typedef enum
{
    BY25DXX_PROTECT_NONE = 0x00U,
#if defined(BY25D40)
    BY25DXX_PROTECT_256KB = 0x06U,
    BY25DXX_PROTECT_384KB = 0x05U,
    BY25DXX_PROTECT_448KB = 0x04U,
    BY25DXX_PROTECT_480KB = 0x03U,
    BY25DXX_PROTECT_496KB = 0x02U,
    BY25DXX_PROTECT_504KB = 0x01U,
    BY25DXX_PROTECT_512KB = 0x07U
#endif
} BY25DXX_ProtectSize;

typedef enum
{
    BY25DXX_ERASE_SECTOR = 0x20U,     // 4KB
    BY25DXX_ERASE_HALF_BLOCK = 0x52U, // 32KB
    BY25DXX_ERASE_BLOCK = 0xD8U,      // 64KB
    BY25DXX_ERASE_CHIP = 0x60U        // ALL
} BY25DXX_EraseType;

typedef enum
{
    BY25DXX_ERR_NONE = 0,
    BY25DXX_ERR_FAILED = 1
} BY25DXX_ErrorCode;

/**
 * Init BY25DXX
*/
BY25DXX_ErrorCode BY25DXX_Init(BY25DXX_SPIHook spiHook);

/**
 * read operations
*/

uint8_t BY25DXX_ReadByte(uint32_t addr);
uint16_t BY25DXX_ReadWord(uint32_t addr);
void BY25DXX_ReadBytes(uint32_t addr, uint8_t *buf, uint32_t size);

/**
 * write operations
*/

void BY25DXX_WriteByte(uint32_t addr, uint8_t dat);
void BY25DXX_WriteWord(uint32_t addr, uint16_t word);
void BY25DXX_WriteBytes(uint32_t addr, uint8_t *buf, uint32_t len);
void BY25DXX_Erase(uint32_t addr, BY25DXX_EraseType type);

/**
 * lock protection bits
*/

void BY25DXX_LockProtectBits(void);
void BY25DXX_UnlockProtectBits(void);

/**
 * block protection
*/

BY25DXX_ProtectSize BY25DXX_GetProtectSize(void);
void BY25DXX_SetProtectSize(BY25DXX_ProtectSize size);
void BY25DXX_ClearProtection(void);

/**
 * deep sleep/wakeup
*/

void BY25DXX_GotoSleep(void);
void BY25DXX_Wakeup(void);

/**
 * get device information
*/
void BY25DXX_GetDeviceInfo(BY25DXX_DeviceInfo *info);

#endif
