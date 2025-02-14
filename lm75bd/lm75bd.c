#include "lm75bd.h"
#include "i2c_io.h"
#include "errors.h"
#include "logging.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

/* LM75BD Registers (p.8) */
#define LM75BD_REG_CONF 0x01U  /* Configuration Register (R/W) */
#define LM75BD_REG_TEMP 0 /* Pointer Register */
#define TEMP_SIGN_MASK 0b10000000 

error_code_t lm75bdInit(lm75bd_config_t *config) {
  error_code_t errCode;

  if (config == NULL) return ERR_CODE_INVALID_ARG;

  RETURN_IF_ERROR_CODE(writeConfigLM75BD(config->devAddr, config->osFaultQueueSize, config->osPolarity,
                                         config->osOperationMode, config->devOperationMode));

  // Assume that the overtemperature and hysteresis thresholds are already set
  // Hysteresis: 75 degrees Celsius
  // Overtemperature: 80 degrees Celsius

  return ERR_CODE_SUCCESS;
}

error_code_t readTempLM75BD(uint8_t devAddr, float *temp) {
  /* Implement this driver function */

  error_code_t errCode;
  
  uint8_t pointerReg = LM75BD_REG_TEMP;
  error_code_t _ret = i2cSendTo(LM75BD_OBC_I2C_ADDR, &pointerReg, 1);
  RETURN_IF_ERROR_CODE(_ret);

  #define ARRAY_SIZE 2 //array size constant
  uint8_t twosCOMP[ARRAY_SIZE] = {0};
  int16_t decimalREP = 0;
  _ret=i2cReceiveFrom(LM75BD_OBC_I2C_ADDR, twosCOMP, ARRAY_SIZE);
  RETURN_IF_ERROR_CODE(_ret);

  decimalREP = twosCOMP[0];
  decimalREP=(decimalREP << 8) | twosCOMP[1];
  
  if((twosCOMP[0] & TEMP_SIGN_MASK) == TEMP_SIGN_MASK){ //two's complement
    decimalREP = ((~decimalREP)>>5)+1;
    
    *temp = decimalREP * -0.125;
  }
  else{ //not two's complement
    decimalREP = decimalREP>>5;
    *temp = decimalREP*0.125;
  }
  return ERR_CODE_SUCCESS;
}

#define CONF_WRITE_BUFF_SIZE 2U
error_code_t writeConfigLM75BD(uint8_t devAddr, uint8_t osFaultQueueSize, uint8_t osPolarity,
                                   uint8_t osOperationMode, uint8_t devOperationMode) {
  error_code_t errCode;

  // Stores the register address and data to be written
  // 0: Register address
  // 1: Data
  uint8_t buff[CONF_WRITE_BUFF_SIZE] = {0};

  buff[0] = LM75BD_REG_CONF;

  uint8_t osFaltQueueRegData = 0;
  switch (osFaultQueueSize) {
    case 1:
      osFaltQueueRegData = 0;
      break;
    case 2:
      osFaltQueueRegData = 1;
      break;
    case 4:
      osFaltQueueRegData = 2;
      break;
    case 6:
      osFaltQueueRegData = 3;
      break;
    default:
      return ERR_CODE_INVALID_ARG;
  }

  buff[1] |= (osFaltQueueRegData << 3);
  buff[1] |= (osPolarity << 2);
  buff[1] |= (osOperationMode << 1);
  buff[1] |= devOperationMode;

  errCode = i2cSendTo(LM75BD_OBC_I2C_ADDR, buff, CONF_WRITE_BUFF_SIZE);
  if (errCode != ERR_CODE_SUCCESS) return errCode;

  return ERR_CODE_SUCCESS;
}
