//#include "FreeRTOS.h"
//#include "driverlib.h"
//#include <stdlib.h>
//
//float voltage_value;
//
//#pragma PERSISTENT(voltage_arr)
//float voltage_arr[1500] = {0};
//
//#pragma PERSISTENT(voltage_arr_head)
//uint16_t voltage_arr_head = 0;
//
//#pragma PERSISTENT(ADC12MEM0_arr)
//uint16_t ADC12MEM0_arr[1500] = {0};

#include "FreeRTOS.h"
#include "driverlib.h"
#include <stdlib.h>
#include <stdint.h>
#include "adc.h"
#define VOLTAGE_LOG_CAPACITY 1500
float voltage_value;
#pragma PERSISTENT(voltage_arr)
float voltage_arr[VOLTAGE_LOG_CAPACITY] = {0};
#pragma PERSISTENT(ADC12MEM0_arr)
uint16_t ADC12MEM0_arr[VOLTAGE_LOG_CAPACITY] = {0};
#pragma PERSISTENT(voltage_query_id_arr)
uint32_t voltage_query_id_arr[VOLTAGE_LOG_CAPACITY] = {0};
#pragma PERSISTENT(voltage_op_arr)
uint8_t voltage_op_arr[VOLTAGE_LOG_CAPACITY] = {0};
#pragma PERSISTENT(voltage_arr_head)
uint16_t voltage_arr_head = 0;
static uint16_t last_voltage_log_idx = 0;

void init_adc()
{
  GPIO_setAsPeripheralModuleFunctionInputPin(
      GPIO_PORT_P3,
      GPIO_PIN1,
      GPIO_TERNARY_MODULE_FUNCTION);
  while (Ref_A_isRefGenBusy(REF_A_BASE));

  // Select internal ref = 1.2V
  Ref_A_setReferenceVoltage(REF_A_BASE, REF_A_VREF1_2V);

  // Turn on Reference Voltage
  Ref_A_enableReferenceVoltage(REF_A_BASE);
  while (!Ref_A_isVariableReferenceVoltageOutputReady(REF_A_BASE));

  ADC12_B_initParam initParam = {0};
  initParam.sampleHoldSignalSourceSelect = ADC12_B_SAMPLEHOLDSOURCE_SC;
  initParam.clockSourceSelect = ADC12_B_CLOCKSOURCE_ADC12OSC;
  initParam.clockSourceDivider = ADC12_B_CLOCKDIVIDER_1;
  initParam.clockSourcePredivider = ADC12_B_CLOCKPREDIVIDER__1;
  initParam.internalChannelMap = ADC12_B_NOINTCH;
  ADC12_B_init(ADC12_B_BASE, &initParam);

  // Enable the ADC12B module
  ADC12_B_enable(ADC12_B_BASE);

  /*
   * Base address of ADC12B Module
   * For memory buffers 0-7 sample/hold for 64 clock cycles
   * For memory buffers 8-15 sample/hold for 4 clock cycles (default)
   * Disable Multiple Sampling
   */
  ADC12_B_setupSamplingTimer(ADC12_B_BASE,
                             ADC12_B_CYCLEHOLD_64_CYCLES,
                             ADC12_B_CYCLEHOLD_4_CYCLES,
                             ADC12_B_MULTIPLESAMPLESDISABLE);

  // Configure Memory Buffer
  /*
   * Base address of the ADC12B Module
   * Configure memory buffer 0
   * Map input A1 to memory buffer 0
   * Vref+ = IntBuffer
   * Vref- = AVss
   * Memory buffer 0 is not the end of a sequence
   */
  ADC12_B_configureMemoryParam configureMemoryParam = {0};
  configureMemoryParam.memoryBufferControlIndex = ADC12_B_MEMORY_0;
  configureMemoryParam.inputSourceSelect = ADC12_B_INPUT_A13;
  configureMemoryParam.refVoltageSourceSelect = ADC12_B_VREFPOS_INTBUF_VREFNEG_VSS;
  configureMemoryParam.endOfSequence = ADC12_B_NOTENDOFSEQUENCE;
  configureMemoryParam.windowComparatorSelect = ADC12_B_WINDOW_COMPARATOR_DISABLE;
  configureMemoryParam.differentialModeSelect = ADC12_B_DIFFERENTIAL_MODE_DISABLE;
  ADC12_B_configureMemory(ADC12_B_BASE, &configureMemoryParam);
}

static inline void enable_adc(){
    ADC12CTL0 |= ADC12ENC | ADC12SC;
}

static inline void disable_adc(){
    ADC12CTL0 &= ~(ADC12ENC | ADC12SC);
}

float get_voltage(uint32_t query_id) {
    enable_adc();

    while (ADC12_B_isBusy(ADC12_B_BASE));

    uint16_t ADC12MEM0_reading = ADC12MEM0;

    voltage_value = (float)((float)ADC12MEM0_reading * (1.2 * 4 / 4096.0));

    if (voltage_arr_head >= VOLTAGE_LOG_CAPACITY) {
        voltage_arr_head = 0;
    }

    uint16_t idx = voltage_arr_head++;

    ADC12MEM0_arr[idx]        = ADC12MEM0_reading;
    voltage_arr[idx]          = voltage_value;
    voltage_query_id_arr[idx] = query_id;
    voltage_op_arr[idx]       = VOLTAGE_OP_PENDING;

    last_voltage_log_idx = idx;

    disable_adc();

    return voltage_value;
}

void set_last_voltage_op(uint8_t op)
{
    voltage_op_arr[last_voltage_log_idx] = op;
}

//float get_voltage() {
//    enable_adc();
//
//    while(ADC12_B_isBusy(ADC12_B_BASE));
//
//    uint16_t ADC12MEM0_reading = ADC12MEM0;
//
//    voltage_value = (float)((float)(ADC12MEM0_reading) * (1.2 * 4 / 4096.0));
//
//    if (voltage_arr_head == 1500) {
//        voltage_arr_head = 0;
//    }
//
//    voltage_arr[voltage_arr_head] = voltage_value;
//    ADC12MEM0_arr[voltage_arr_head++] = ADC12MEM0_reading;
//
//    disable_adc();
//
//    return voltage_value;
//}
