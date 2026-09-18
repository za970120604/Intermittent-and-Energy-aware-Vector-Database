//#ifndef ADC_H
//#define ADC_H
//
//void init_adc();
//float get_voltage();
//
//#endif
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#define VOLTAGE_OP_PENDING  0xFF
#define VOLTAGE_OP_WAIT     0xFE

void init_adc(void);
float get_voltage(uint32_t query_id);
void set_last_voltage_op(uint8_t op);

#endif
