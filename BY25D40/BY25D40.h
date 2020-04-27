#ifndef _H_BY25D40
#define _H_BY25D40

/**
 * *****************************************************
 * 
 * set BY25D40's PINs macro before include this header
 * 
 * *****************************************************
*/

#if !defined (BY25D40_CS_HIGH) || !defined (BY25D40_CS_LOW)
#error "BY25D40: please define pin 'CS' !"
#endif

#if !defined (BY25D40_WP_HIGH) || !defined (BY25D40_WP_LOW)
#error "BY25D40: please define pin 'WP' !"
#endif

//***********************************************************

/**
 * functions define
*/

typedef uint8_t (*BY25D40_SPIHook)(uint8_t);

typedef struct
{
    uint8_t vendorID;   // Fixed value: 0x68
    uint8_t devID;      // Fixed value: 0x12
    uint8_t memType;    // Fixed value: 0x40
    uint8_t capacity;   // Fixed value: 0x13
    uint8_t uniqueID[8];
} BY25D40_DeviceInfo;

typedef enum
{
    BY25D40_PROTECT_NONE = 0x00U,
    BY25D40_PROTECT_256KB = 0x06U,
    BY25D40_PROTECT_384KB = 0x05U,
    BY25D40_PROTECT_448KB = 0x04U,
    BY25D40_PROTECT_480KB = 0x03U,
    BY25D40_PROTECT_496KB = 0x02U,
    BY25D40_PROTECT_504KB = 0x01U,
    BY25D40_PROTECT_512KB = 0x07U
} BY25D40_ProtectSize;

typedef enum
{
    BY25D40_ERASE_SECTOR = 0x20U,       // 4KB
    BY25D40_ERASE_HALF_BLOCK = 0x52U,   // 32KB
    BY25D40_ERASE_BLOCK = 0xD8U,        // 64KB
    BY25D40_ERASE_CHIP = 0x60U          // 512KB
} BY25D40_EraseType;

/**
 * Init SPI hook
*/
void BY25D40_InitHook(BY25D40_SPIHook spiHook);

/**
 * read operations
*/

uint8_t BY25D40_ReadByte(uint32_t addr);
uint16_t BY25D40_ReadWord(uint32_t addr);
void BY25D40_ReadBytes(uint32_t addr, uint8_t *list, uint8_t size);

/**
 * write operations
*/

void BY25D40_WriteByte(uint32_t addr, uint8_t dat);
void BY25D40_WriteWord(uint32_t addr, uint16_t word);
void BY25D40_WriteBytes(uint32_t addr, uint8_t *list, uint8_t len);
void BY25D40_Erase(uint32_t addr, BY25D40_EraseType type);

/**
 * lock protection bits
*/

void BY25D40_LockProtectBits(void);
void BY25D40_UnlockProtectBits(void);

/**
 * block protection
*/

BY25D40_ProtectSize BY25D40_GetProtectSize(void);
void BY25D40_SetProtectSize(BY25D40_ProtectSize size);
void BY25D40_ClearProtection(void);

/**
 * deep sleep/wakeup
*/

void BY25D40_GotoSleep(void);
void BY25D40_Wakeup(void);

/**
 * get device information
*/
void BY25D40_GetDeviceInfo(BY25D40_DeviceInfo *info);

#endif
