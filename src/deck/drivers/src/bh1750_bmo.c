#define DEBUG_MODULE "BH1750"

#include "deck.h"
#include "log.h"
#include "system.h"
#include "i2cdev.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"

// --- Hardware Config ---
// CHANGE: Trying alternate address 0x5C first. 
// If this fails (Lux=9999), swap back to 0x5C.
#define BH1750_ADDR         0x23  
#define I2C_PORT            I2C1_DEV

// BH1750 Instructions
#define CMD_POWER_ON        0x01
#define CMD_RESET           0x07
#define CMD_H_RES_MODE      0x10

// --- Global Variable for Logging ---
static uint16_t bmo_lux_value = 1234;

// --- The Sensor Task ---
static void bh1750Task(void *param)
{
    uint8_t buffer[2];

    systemWaitStart(); 

    // 1. Initialization
    buffer[0] = CMD_POWER_ON;
    i2cdevWrite(I2C_PORT, BH1750_ADDR, 1, buffer);
    vTaskDelay(M2T(10));

    buffer[0] = CMD_RESET;
    i2cdevWrite(I2C_PORT, BH1750_ADDR, 1, buffer);
    vTaskDelay(M2T(10));

    buffer[0] = CMD_H_RES_MODE;
    i2cdevWrite(I2C_PORT, BH1750_ADDR, 1, buffer);
    vTaskDelay(M2T(180));

    // 2. Main Loop (10Hz)
    while (1)
    {
        // Try to read 2 bytes
        if (i2cdevRead(I2C_PORT, BH1750_ADDR, 2, buffer))
        {
            uint16_t raw = (buffer[0] << 8) | buffer[1];
            // Success: Convert to Lux
            bmo_lux_value = (uint16_t)(raw / 1.2f);
        }
        else 
        {
            // FAILURE DEBUG: If I2C fails, set to 9999
            // This tells us the code is running, but the sensor isn't responding.
            bmo_lux_value = 9999; 
        }

        vTaskDelay(M2T(100));
    }
}

// --- Deck Initialization ---
void bh1750Init(DeckInfo *info)
{
    xTaskCreate(bh1750Task, "bh1750Worker", 
                2 * configMINIMAL_STACK_SIZE, NULL, 2, NULL);
}

// --- Driver Registration ---
const DeckDriver bh1750_driver = {
    .vid = 0,
    .pid = 0,
    .name = "BMO_Sensor",
    .usedGpio = 0,
    .init = bh1750Init,
};

DECK_DRIVER(bh1750_driver);

// --- Log Group Registration ---
LOG_GROUP_START(bmoSensors)
  LOG_ADD(LOG_UINT16, lux, &bmo_lux_value)
LOG_GROUP_STOP(bmoSensors)
