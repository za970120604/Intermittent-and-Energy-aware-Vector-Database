//#include "FreeRTOS.h"
//#include "task.h"
//#include <stdio.h>
//#include "queue.h"
//#include "checkpoint.h"
//#include <driverlib.h>
//#include "my_timer.h"
//
//#define tskTEST_PRIORITY (tskIDLE_PRIORITY + 1)
//
//#pragma PERSISTENT(shutdown_cnt)
//uint8_t shutdown_cnt = 4;
//
//#if (EXPERIMENT == OP_BASED_EXPERIMENT)
//static void test_op_based_power_event(void *pvParameters);
//#endif
//
//#if (EXPERIMENT == TIME_BASED_EXPERIMENT)
//static void test_time_based_power_event(void *pvParameters);
//#endif
//
//void main_checkpoint(void)
//{
//#if (EXPERIMENT == OP_BASED_EXPERIMENT)
//    xTaskCreate(test_op_based_power_event, "test", configMINIMAL_STACK_SIZE, NULL, tskTEST_PRIORITY, NULL);
//#elif (EXPERIMENT == TIME_BASED_EXPERIMENT)
//    xTaskCreate(test_time_based_power_event, "test", configMINIMAL_STACK_SIZE, NULL, tskTEST_PRIORITY, NULL);
//#endif
//
//    vTaskStartScheduler();
//
//    for (;;);
//}
//
//uint32_t progress;
//#if (EXPERIMENT == OP_BASED_EXPERIMENT)
//static void test_op_based_power_event(void *pvParameters)
//{
//    (void)pvParameters;
//
//
//    for (progress = 1; progress <= 200; ++progress)
//    {
//        if (progress == 30)
//            checkpoint();
//
//        if (progress == 60)
//            checkpoint();
//
//        if (progress == 20 && shutdown_cnt == 4)
//        {
//            --shutdown_cnt;
//            shutdown();
//
//            /* SHOULD NOT EXECUTE CODE BELOW */
//            printf("YOU DARE USE MY OWN SPELL AGAINST ME POTTER!!!");
//        }
//        if (progress == 40 && shutdown_cnt == 3)
//        {
//            --shutdown_cnt;
//            shutdown();
//
//            /* SHOULD NOT EXECUTE CODE BELOW */
//            printf("YOU SHALL NOT PASS!!!");
//        }
//        if (progress == 50 && shutdown_cnt == 2)
//        {
//            --shutdown_cnt;
//            shutdown();
//
//            /* SHOULD NOT EXECUTE CODE BELOW */
//            printf("I DON'T LIKE SAND.");
//        }
//        if (progress == 99 && shutdown_cnt == 1)
//        {
//            --shutdown_cnt;
//            shutdown();
//
//            /* SHOULD NOT EXECUTE CODE BELOW */
//            printf("WE NEED TO GO DEEPER.");
//        }
//
//        if (progress == 120 && shutdown_cnt == 0)
//        {
//            --shutdown_cnt;
//            shutdown();
//
//            /* SHOULD NOT EXECUTE CODE BELOW */
//            printf("WE NEED TO GO DEEPER.");
//        }
//
//        printf("%d\n", progress);
//    }
////    while(++progress){
////        time_t t;
////        srand(time(&t));
////        if (progress%15 == 0) {
////            shutdown();
////        }
////        if (progress % 10 == 0) {
////            checkpoint();
////        }
////        printf("%d\n", progress);
////    }
//
//    for (;;);
//}
//#endif
//
//#if (EXPERIMENT == TIME_BASED_EXPERIMENT)
///* Add "progress" to expression and set breakpoint at A3_ISR & restore to check their functionalities */
//static void test_time_based_power_event(void *pvParameters)
//{
//    progress = 1;
//
//    while (1)
//    {
//        if (progress % 1000 == 0)
//        {
//            checkpoint();
//            printf("CHECKPOINT AT i = %u\n", progress); // Will be printed after restore
//        }
//
//        ++progress;
//        __delay_cycles(1000);
//        /* printing "progress" here is strongly not recommended */
//    }
//
//    for (;;);
//}
//#endif
