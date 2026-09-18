#include "portINT.h"

#pragma vector=PORT5_VECTOR
__interrupt void P5_ISR( void )
{
    GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_clearInterrupt(GPIO_PORT_P5, GPIO_PIN6);
}
