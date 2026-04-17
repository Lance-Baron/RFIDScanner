/*
 *                    Copyright (c), NXP Semiconductors
 *
 *                       (C) NXP Semiconductors 2014,2015
 *
 *         All rights are reserved. Reproduction in whole or in part is
 *        prohibited without the written consent of the copyright owner.
 *    NXP reserves the right to make changes without notice at any time.
 *   NXP makes no warranty, expressed, implied or statutory, including but
 *   not limited to any implied warranty of merchantability or fitness for any
 *  particular purpose, or that the use will not infringe any third party patent,
 *   copyright or trademark. NXP must not be liable for any loss or damage
 *                            arising from its use.
 */

/** @file
 *
 * phExHif.c:  Main application file and entry point of phExHif. Contains core logic part of the phExHif application.
 *             To know about phExHif application refer to PN7462AU_FW_UM user manual.
 *
 * Project:  PN7462AU
 *
 * $Date: 2016-06-20 15:49:09 +0530 (Mon, 20 Jun 2016) $
 * $Author: Purnank G (ing05193) $
 * $Revision: 18464 $ (v05.07.00)
 */

/* *****************************************************************************************************************
 * Includes
 * ***************************************************************************************************************** */

#include "ph_Datatypes.h"
#include "ph_Status.h"
#include <stdio.h>
#include "phCfg_EE.h"
#include "phFlashBoot.h"
#include "PN7462AU/PN7462AU_pcr.h"
#include "phExHif.h"
#include "phExHif_Hw.h"
#include "phhalPcr.h"
#include "phLED.h"
#include "ph_Log.h"
#include "phhalGpio.h"

/* *****************************************************************************************************************
 * Internal Definitions
 * *************************************************************************************************************** */

/* *****************************************************************************************************************
 * Type Definitions
 * *************************************************************************************************************** */

/* *****************************************************************************************************************
 * Global and Static Variables
 * Total Size: NNNbytes
 * *************************************************************************************************************** */

/* *****************************************************************************************************************
 * Private Functions Prototypes
 * *************************************************************************************************************** */
static void phExHif_HifTask(void);
/* *****************************************************************************************************************
 * Public Functions
 * *************************************************************************************************************** */

/* *****************************************************************************************************************
 * Private Functions
 * *************************************************************************************************************** */

static void phExHif_HifTask(void) {
    uint16_t wLength;
    phStatus_t eStatus;
    uint8_t bCommChannel = 0;
    uint8_t bHifInterface = 0;
    gphExHif_Hw_Ct_Init = false;

    if ((PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_L)
            | PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_H))) {
        LOG_TXT("Waiting for activity on HIF SEL PINS\n");
    }

        /* Used for synchronisation between Master(LPC1769) and Slave(PN7462AU).
         * Wait till all the Ingress GPIO ports go LOW
         */
    while ((PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_L)
            | PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_H)))
        ;

    while (1) {
        eStatus = PH_ERR_SUCCESS;

        phLED_SetStatus(LED_B);

        /** Read the Hif-Interface Gpio configs and Wait until Ingress ports NOT Equal to Zero */
        while (!(PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_L)
                | PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_H)))
            ;

        do {
            /** Initialize the Hif hardware. */
            bHifInterface = phExHif_Hw_InPort_Init();

            /** OutPort Initialization */
            bCommChannel = phExHif_Hw_OutPort_Init();

            PH_REG_SET_BIT(PCR_PADOUT_REG, PADOUT_GPIO1);

            /** Receive data over the HIF interface */
            eStatus = phExHif_Hw_HostInterface_Receive(&wLength);
            if (eStatus != PH_ERR_SUCCESS) {
                 phLED_SetStatus(LED_R);
                break;
            }

            /* On successfull reception set the INPORT LED ON */
            phLED_SetStatus(LED_G);

            /** Forward the received packets to selected communication channels. */
            eStatus = phExHif_Hw_Transmit(bCommChannel, bHifInterface, wLength,
                    (uint32_t *) gphExHif_Hw_RxBuffer);
            if (eStatus != PH_ERR_SUCCESS) {
                 phLED_SetStatus(LED_R);
                 LOG_X32("Error Code= ",eStatus);
            }

            /** Output Port De-Initialization */
            eStatus = phExHif_Hw_DeInit(bCommChannel);
            if (eStatus != PH_ERR_SUCCESS) {
                 phLED_SetStatus(LED_R);
                break;
            }

            /** De-Assert the Data Ready pin for synchronization. */
            PH_REG_CLEAR_BIT(PCR_PADOUT_REG, PADOUT_GPIO1);

            /** For synchronization,Wait till all the Ingress GPIO ports go LOW to ensure a transaction is complete */
            while ((PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_L)
                    | PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_H)))
                ;

        } while (0);
    }
}

int main(void) {

    uint8_t bCount;

    phFlashBoot_Main();

    /* Initialize LED's */
    phLED_Init();

    /** Configure IRQ and Data ready ping to LOW on bootup*/
    PH_REG_CLEAR_BIT(PCR_PADOUT_REG, PADOUT_IRQ);

    /** Configure the Data Ready pin as OUTPUT */
    PH_REG_SET_BITN(PCR_PADGPIO1_REG,
            PCR_PADGPIO1_REG_GPIO1_EN_OUT_POS);

    /** De-Assert the Data Ready pin.*/
    PH_REG_CLEAR_BIT(PCR_PADOUT_REG, PADOUT_GPIO1);

    /*Configuring GPIO's 4,5,6,7,8 as Inputs*/
    for(bCount = 4 ; bCount < 9 ;bCount ++)
    {
        phhalPcr_ConfigPuPd(bCount,false,true);
        phhalPcr_ConfigInput(bCount,true,false,false,false,true,false);
    }

    /* Create the Event group for HIF transactions */
    gphExHif_Hw_HifEvents = phRtos_EventGroupCreate();

    /* Execute the Hif Application*/
    phExHif_HifTask();

    return 0;

}
