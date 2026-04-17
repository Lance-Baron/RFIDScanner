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
 * phExHif_Hw.c: Implements the hardware modules initialization and its usage part of phExHif.
 *
 * Project:  PN7462AU
 *
 * $Date: 2016-01-19 14:43:27 +0530 (Tue, 19 Jan 2016) $
 * $Author: Purnank G (ing05193) $
 * $Revision: 17409 $ (v05.07.00)
 */

/* *****************************************************************************************************************
 * Includes
 * *************************************************************************************************************** */
#include "ph_Datatypes.h"
#include "ph_Status.h"
#include "phCfg_EE.h"
#include "phRtos.h"
#include "phUser.h"
#include "phhalHif.h"
#include "phhalI2CM.h"
#include "phhalSPIM.h"
#include "phhalEeprom.h"
#include "phhalFlash.h"
#include "PN7462AU/PN7462AU_hif.h"
#include "phhalPcr.h"
#include "phhalCt.h"
#include "ph_Status.h"
/* PN7462AU_HIF_EX */
#include "phExHif_Hw.h"
#include "ph_Log.h"
#include "phLED.h"
/* *****************************************************************************************************************
 * Internal Definitions
 * *************************************************************************************************************** */
#if PHFL_LOG_ENABLE
static const char * TX_STRING[] = { "LOOPBACK\n", "TX over I2CM\n", "TX over SPIM\n",
        "TX over I2CM and SPIM\n", "EEPROM WRITE SUCCESS\n", "FLASH WRITE SUCCESS\n",
        "CT TRANSACTION COMPLETE\n", "\n", };

static const char * RX_STRING[] = { "\0", "RX over HIF I2C\n", "RX over HIF SPI\n",
        "RX over HIF HSU\n", "\n", };
#endif /* PHFL_LOG_ENABLE*/
/* *****************************************************************************************************************
 * Type Definitions
 * *************************************************************************************************************** */
#define PH_EXMAINCT_MAX_NO_APDU 11

/*
 * Structure that helps while pointing to the received buffer directly.
 */
typedef struct {
    uint8_t baData[PHHAL_EEPROM_PAGE_SIZE];
    uint8_t bPageNum;
} phExHif_Eeprom_t;

typedef enum HIFEvent
{
    HIFEVT_BUFFER_CFG_CHANGED_ERROR      = 1 << 0,
    HIFEVT_EOT                           = 1 << 1,
    HIFEVT_HSU_RX_FER                    = 1 << 2,
    HIFEVT_RX0_BUFFER_OVERFLOW           = 1 << 3,
    HIFEVT_RX0_EOR                       = 1 << 4,
    HIFEVT_RX0_FRAME_UNDERFLOW           = 1 << 5,
    HIFEVT_RX_BUFFER_NOT_AVAILABLE       = 1 << 6,
    HIFEVT_WATERLEVEL_REACHED            = 1 << 7
} HIFEvent_t;

/* *****************************************************************************************************************
 * Global and Static Variables
 * Total Size: NNNbytes
 * *************************************************************************************************************** */
PH_NOINIT static uint8_t bApduRespBuffer[256];

PH_NOINIT uint32_t gphExHif_Hw_RxBuffer[PH_EXHIF_HW_BUFFER_SIZE];
PH_NOINIT phRtos_EventHandle_t gphExHif_Hw_HifEvents;
PH_NOINIT uint8_t gphExHif_Hw_Ct_Init;
PH_NOINIT phpalCt_DataParams_t gphExHif_Hw_DataParams;
PH_NOINIT uint8_t gphExHif_Hw_AtrBuffer[32];


/* *****************************************************************************************************************
 * Private Functions Prototypes
 * *************************************************************************************************************** */

static void phExHif_Hw_Generic_CallBack(uint32_t, void *);

static void phExHif_Hw_RX0_CallBack(uint32_t, void *);

static void phExHif_Hw_TX_CallBack(uint32_t, void *);

static phStatus_t phExHif_GetRxDataLength(phhalHif_BuffId_t, uint16_t *);

static phStatus_t phExHif_HostInterface_Transmit(uint16_t wTxLength,
        uint32_t *pdwTxData);

static phStatus_t phExHif_CtTransaction(uint8_t *pbAPDU, uint16_t wAPDULen);
/* *****************************************************************************************************************
 * Public Functions
 * *************************************************************************************************************** */

/** This API reads the Ingress ports and configures the corresponding Host interface for data reception. */
uint8_t phExHif_Hw_InPort_Init(void) {
    uint8_t bHifInterface;
    phhalHif_Config_t sHifConfigs;

    if (!(PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_L)
            || PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_H))) {
        LOG_TXT("Waiting for LPC to toggle HIF Sel PINS\n");
    }

    /** Read the Hif-Interface Gpio configs and wait until not equal to valid value other than ZERO. */
    while (!(PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_L)
            || PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_H)))
        ;

    /** Read the Hif configs from EEPROM. */
    sHifConfigs.eBufferType = E_BUFFER_FORMAT_FREE; /* E_BUFFER_FORMAT_FREE */
    sHifConfigs.bShortFrameLen = gpkphCfg_EE_HW_HIF->bShortFrameLen;
    sHifConfigs.bStoreErrData = gpkphCfg_EE_HW_HIF->bStoreErrData;
    sHifConfigs.bHeaderSize = gpkphCfg_EE_HW_HIF->bHeaderSize;
    sHifConfigs.bTimeout = gpkphCfg_EE_HW_HIF->bTimeout;

    /** Identify the Host interface for Initial communication */
    bHifInterface =
            (uint8_t) (PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_L)
                    | ( PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_H)
                            << 1));

    switch (bHifInterface) {
    case 1: /** Hif-I2c config. */
        LOG_TXT("HIF=I2C\n");
        sHifConfigs.eInterface = E_HIF_I2C;
        sHifConfigs.sConfig.sI2cConfig = gpkphCfg_EE_HW_HIF->bI2cConfig;
        break;

    case 2: /** Hif-Spi config. */
        LOG_TXT("HIF=SPI\n");
        sHifConfigs.eInterface = E_HIF_SPI;
        sHifConfigs.sConfig.sSpiConfig = gpkphCfg_EE_HW_HIF->bSpiConfig;
        break;

    case 3: /** Hif-Hsu config. */
        LOG_TXT("HIF=HSUC\n");
        sHifConfigs.eInterface = E_HIF_HSU;
        sHifConfigs.sConfig.sHsuConfig.bIsHsuBoot =
                gpkphCfg_EE_HW_HIF->bIsHsuBoot;
        sHifConfigs.sConfig.sHsuConfig.bEOF = gpkphCfg_EE_HW_HIF->bEndOfFrame;
        sHifConfigs.sConfig.sHsuConfig.bStopBits = gpkphCfg_EE_HW_HIF->bStopBits;
        sHifConfigs.sConfig.sHsuConfig.bDummyBytes =
                gpkphCfg_EE_HW_HIF->bDummyBytes;
        sHifConfigs.sConfig.sHsuConfig.bBaudRate =
                (phhalHif_Hsu_BaudRate_t) gpkphCfg_EE_HW_HIF->bBaudRate;

        /** Required to set ATXA pins with SLEW RATE for higher baudrates.*/
        if (sHifConfigs.sConfig.sHsuConfig.bBaudRate > 7) {
            PH_REG_SET_BIT(PCR_PAD_ATX_A_REG, ATX_A_SLEW_RATE);
        } else {
            /** Added to support coding Guidelines. */
        }
        break;

    default:
        /** It is not expected to reach here. */
        PH_USER_ASSERT(0)
        ;
        break;
    }

    /** Host Interface Initialize */
    PH_USER_ASSERT(
            phhalHif_Init(&sHifConfigs, &phExHif_Hw_Generic_CallBack) == PH_ERR_SUCCESS);

    /** Configure Receive Buffer0 with 1024 max length for data reception */
    PH_USER_ASSERT(
            phhalHif_InitRxBuffer(E_RX_BUFFER_ID0, (PH_EXHIF_HW_BUFFER_SIZE * 4), gphExHif_Hw_RxBuffer,\
                    &phExHif_Hw_RX0_CallBack)== PH_ERR_SUCCESS);

    return bHifInterface;
}

/** This API reads the Egress ports and configures the corresponding interface for data transmission. */
uint8_t phExHif_Hw_OutPort_Init(void) {
    uint8_t bCommChannel;
    uint8_t bBaudRate;
    phhalCt_InitParam_t sInitParams;
    sInitParams.bAutoCTDeactivationEnable = 0;
    sInitParams.bCardPresConnectorStatus = gpkphCfg_EE_Boot_CT->bConnectorType;
    sInitParams.bPullupSetStatus = gpkphCfg_EE_Boot_CT->bPullUp;
    sInitParams.bSlewRateConfig = gpkphCfg_EE_Boot_CT->bSlewRate;

    /** Select the communication Channel by reading values from Egress Ports.. */
    bCommChannel =
            (uint8_t) (PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_COMM_PORT_L)
                    | (PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_COMM_PORT_M)
                            << 1)
                    | (PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_COMM_PORT_H)
                            << 2));

    switch (bCommChannel) {
    case PH_EXHIF_HW_OUT_PORT_I2CM:
    case PH_EXHIF_HW_OUT_PORT_SPIM:
    case PH_EXHIF_HW_OUT_PORT_I2CM_SPIM:
        if (PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_COMM_PORT_L)) {
            /** Init and Configure I2CM as Egress communication channel */
            bBaudRate = (uint8_t) ((PH_EXHIF_HW_CRYSTAL_CLK
                    / gpkphCfg_EE_HW_I2CM->dwDataRateHz)
                    - PH_EXHIF_HW_CRYSTAL_CLK_MHZ);

            PH_USER_ASSERT(
                    phhalI2CM_Init(PH_EXHIF_HW_I2CM_TX_FIFO_THRES,PH_EXHIF_HW_I2CM_RX_FIFO_THRES,   \
                            PH_EXHIF_HW_I2CM_TIMEOUT, PH_EXHIF_HW_I2CM_RETRY_CNT) == PH_ERR_SUCCESS);

            /* SDA_HOLD field is 0x0, the I2C Master IP will internal take care of hold time. */
            PH_USER_ASSERT(
                    phhalI2CM_Config(bBaudRate, 0, E_I2CM_7BIT_ADDR_MODE) == PH_ERR_SUCCESS);
        }
        if (PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_COMM_PORT_M)) {
            /** Init and Configure SPIM as Egress communication channel */
            PH_USER_ASSERT(
                    phhalSPIM_Init(PH_EXHIF_HW_SPIM_TIMEOUT) == PH_ERR_SUCCESS);

            PH_USER_ASSERT(
                    phhalSPIM_Configure(PH_EXHIF_HW_SPIM_SLAVE, PH_EXHIF_HW_SPIM_MSB_FIRST,                 \
                            (phhalSPIM_Modes_t) gpkphCfg_EE_HW_SPIM->bModes, gpkphCfg_EE_HW_SPIM->bDataRate,  \
                            PH_EXHIF_HW_SPIM_NSSPULSE, PH_EXHIF_HW_SPIM_NSSPOL) == PH_ERR_SUCCESS);
        }
        break;
    case PH_EXHIF_HW_OUT_PORT_EEPROM:
        /** EEPROM HAL Initialization */
        PH_USER_ASSERT((phhalEeprom_Init(false)) == PH_ERR_SUCCESS)
        ;

        /** When EEPROM memory is Powered up. Normally this takes 100us. The user must not perform any EEPROM access
         *          for a period of 100us after calling the Init API of EEPROM HAL.*/
        phUser_Wait(100);

        break;

    case PH_EXHIF_HW_OUT_PORT_FLASH:

        /** Flash Init using HAL.*/
        PH_USER_ASSERT(phhalFlash_Init(false, true) == PH_ERR_SUCCESS)
        ;

        break;
    case PH_EXHIF_HW_OUT_PORT_CT:

        if (gphExHif_Hw_Ct_Init == false) {
            /** Initialize the CT PAL layer
             * With the performance board configuration as pull down enabled and normally open connector type */
            /** The init parameters should come from the EEPROM */
            PH_USER_ASSERT(phpalCt_Init(& sInitParams) == PH_ERR_SUCCESS);

        }

        /** Wait till the card is available in Main slot*/
        while (phhalCt_CheckCardPres() != (PH_ERR(CT_MAIN_CARD_PRESENT, HAL_CT)))
            ;

        break;

    default:
        break;

    }
    return bCommChannel;
}

uint8_t phExHif_Hw_DeInit(uint8_t bCommChannel)
{
    phStatus_t wStatus;

    switch (bCommChannel) {
    case PH_EXHIF_HW_OUT_PORT_I2CM:
    case PH_EXHIF_HW_OUT_PORT_SPIM:
    case PH_EXHIF_HW_OUT_PORT_I2CM_SPIM:
        if (PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_COMM_PORT_L)) {

            PH_USER_ASSERT(
                    phhalI2CM_DeInit() == PH_ERR_SUCCESS);
        }
        if (PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_COMM_PORT_M)) {

            PH_USER_ASSERT(
                    phhalSPIM_DeInit() == PH_ERR_SUCCESS);
        }
        break;
    case PH_EXHIF_HW_OUT_PORT_EEPROM:
        /** EEPROM HAL De-Initialization */
        /** EEPROM does not have any RTOS Initializations */
        break;

    case PH_EXHIF_HW_OUT_PORT_FLASH:
        /** FLASH HAL De-Initialization */
        /** FLASH does not have any RTOS Initializations */
        break;
    case PH_EXHIF_HW_OUT_PORT_CT:
        /*  Init happening only once, since
            we are checking for the condition every time before calling Init */
        break;

    default:
        break;

    }

    /** HIF De-initializations and reset all buffer registers */
    wStatus = phhalHif_DeInit();
    if (wStatus != PH_ERR_SUCCESS) {
         phLED_SetStatus(LED_R);
    }
    return 0;
}

/** This API receives the data over the HIF by waiting for the event. Returns the number of bytes received. */
phStatus_t phExHif_Hw_HostInterface_Receive(uint16_t* pwLength) {
    uint32_t dwBits;
    uint32_t dwHIF_TicksToWait = 0xFFFFFFFFUL;
    phStatus_t eStatus = PH_ERR_SUCCESS;
    do {
        /**Wait for a HIF events */
        dwBits =
                phRtos_EventGroupWaitBits(gphExHif_Hw_HifEvents, 0
                    | HIFEVT_RX0_EOR
                    | HIFEVT_RX_BUFFER_NOT_AVAILABLE
                    | HIFEVT_RX0_FRAME_UNDERFLOW
                    | HIFEVT_RX0_BUFFER_OVERFLOW
                    | HIFEVT_WATERLEVEL_REACHED
                    | HIFEVT_BUFFER_CFG_CHANGED_ERROR
                    | HIFEVT_HSU_RX_FER,
                    true, false, dwHIF_TicksToWait);

        if (!(dwBits & HIFEVT_RX0_EOR)) {

            LOG_TXT("Reception over HIF Failed: TIMEOUT\n");
            /** Not expected to reach here*/
            PH_USER_ASSERT(0);
        }

        /** Console prints. */
        LOG_TXT(
                RX_STRING[(PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_L) | \
                        ( PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_IN_PORT_H) << 1))]);

        eStatus = phExHif_GetRxDataLength((phhalHif_BuffId_t) E_RX_BUFFER_ID0,
                pwLength);
        PH_BREAK_ON_FAILURE(eStatus);

        eStatus = phhalHif_ReleaseRxBuffer((phhalHif_BuffId_t) E_RX_BUFFER_ID0);
        PH_BREAK_ON_FAILURE(eStatus);

    } while (0);

    return eStatus;

}

/** This API Transmits the data over the selected Egress port.If EEPROM/FLASH write is the selected functionality the
 * data is written and response is transmitted over the selected HIF. */
phStatus_t phExHif_Hw_Transmit(uint8_t bCommChannel, uint8_t bHifInterface,
        uint16_t wLength, uint32_t *pdwData) {
    phExHif_Eeprom_t *psEeprom;

    phStatus_t eStatus = PH_ERR_SUCCESS;
    uint32_t dwFlash_PageNum;

    /** Array to hold the EEPROM data + 1 byte Page Number */
    uint32_t dwphEE_Buf[PHHAL_EEPROM_PAGE_SIZE + 1] = { 0 };

    /** Array to hold the FLASH data to be written */
    uint8_t baFlash_data[PHHAL_FLASH_PAGE_SIZE] = { 0 };

    do {
        switch (bCommChannel) {
        case PH_EXHIF_HW_OUT_PORT_LOOPBACK:

            /** Transmit the data over the HostInterface. Loopback*/
            eStatus = phExHif_HostInterface_Transmit(wLength, pdwData);
            break;

        case PH_EXHIF_HW_OUT_PORT_I2CM:
            /** Transmit the received data on I2CM. */
            eStatus = phhalI2CM_Transmit(gpkphCfg_EE_HW_I2CM->bSlaveAddr,
                    wLength, &(pdwData[1]));
            /* Issue a reset on Arbitration error.*/
            if(eStatus == PH_ERR_ARB_ERROR)
                phhalI2CM_Reset();

            break;

        case PH_EXHIF_HW_OUT_PORT_SPIM:
            /** Transmit the received data on SPIM. */
            eStatus = phhalSPIM_Transmit(PH_EXHIF_HW_SPIM_SLAVE,
            PH_EXHIF_HW_SPIM_INIT_CRC, PH_EXHIF_HW_SPIM_APPEND_CRC,
            PH_EXHIF_HW_SPIM_CRC_INIT, wLength, &(((uint8_t *) pdwData)[0]),
            PH_EXHIF_HW_SPIM_CRC_OFFSET);
            break;

        case PH_EXHIF_HW_OUT_PORT_I2CM_SPIM:
            /** Transmit the received data on I2CM and SPIM. */

            eStatus = phhalI2CM_Transmit(gpkphCfg_EE_HW_I2CM->bSlaveAddr,
                    wLength, &(pdwData[1]));
            /* Issue a reset on Arbitration error.*/
           if(eStatus == PH_ERR_ARB_ERROR)
           {
               phhalI2CM_Reset();
               LOG_X32("I2CM Transmit Error. ERR CODE",eStatus);
           }

            /* Added for synchronization. Since LPC1769 receives over I2C and SPI slave interfaces, ensure
             * small delay for processing the interrupts in LPC1769. */
            phUser_Wait(500000);

            eStatus = phhalSPIM_Transmit(PH_EXHIF_HW_SPIM_SLAVE,
            PH_EXHIF_HW_SPIM_INIT_CRC, PH_EXHIF_HW_SPIM_APPEND_CRC,
            PH_EXHIF_HW_SPIM_CRC_INIT, wLength, &(((uint8_t *) pdwData)[0]),
            PH_EXHIF_HW_SPIM_CRC_OFFSET);
            break;

        case PH_EXHIF_HW_OUT_PORT_EEPROM:
            /** Write the data to EEPROM User Area.Only last two sectors. */

            /** Discarding the direction indicator and making it aligned*/
            phUser_MemCpy((uint8_t*) dwphEE_Buf, &(((uint8_t*) pdwData)[0]),
                    (PHHAL_EEPROM_PAGE_SIZE + 1));
            psEeprom = (phExHif_Eeprom_t *) dwphEE_Buf;

            /** Status Response length. Length(2bytes) + Status(2bytes)*/
            wLength = 4;

            eStatus = (uint16_t) phhalEeprom_WritePage(psEeprom->bPageNum,
                    psEeprom->baData);

            /** Response length is of 5Bytes. 2Bytes length and 2bytes response. */
            ((uint16_t*) pdwData)[0] = 0x0200;
            ((uint16_t *) pdwData)[1] = eStatus;

            /** De-Init the EEPROM after the write operation */
            phhalEeprom_DeInit();

            /** Transmit the response over the selected HIF*/
            eStatus = phExHif_HostInterface_Transmit(wLength, pdwData);

            break;

        case PH_EXHIF_HW_OUT_PORT_FLASH:
            /** Write data to FLASH Area. Write only the last 256bytes(i.e last two pages) */

            /** Discarding the direction indicator and making it aligned*/
            phUser_MemCpy((uint8_t*) baFlash_data, &(((uint8_t*) pdwData)[0]),
                    wLength);

            dwFlash_PageNum = ((PHHAL_FLASH_USER_PAGE_END
                    - PHHAL_FLASH_USER_PAGE_START) - 1);

            /** Status Response length. Length + Status*/
            wLength = 4;
            eStatus = phhalFlash_WritePage(dwFlash_PageNum, baFlash_data);
            /** Response length is of 5Bytes. 2Bytes length and 2bytes response */
            ((uint16_t*) pdwData)[0] = 0x0200;
            ((uint16_t *) pdwData)[1] = eStatus;

            /** Transmit the response over selected HIF */
            eStatus = phExHif_HostInterface_Transmit(wLength, pdwData);
            break;

        case PH_EXHIF_HW_OUT_PORT_CT:
            /** Transmit the APDU received over HIF to Contact module */
            phExHif_CtTransaction(&(((uint8_t*) pdwData)[0]), wLength);

            /** Transmit the response over selected HIF. Status response is 2Bytes */
            eStatus = phExHif_HostInterface_Transmit(2, pdwData);

            /* PH_USER_ASSERT(phpalCt_DeInit(false) == PH_ERR_SUCCESS); */
            break;

        default:
            /* It is not expected to reach here. */
            PH_USER_ASSERT(0)
            ;
            break;
        }
    } while (0);

    if (eStatus != PH_ERR_SUCCESS) {
        LOG_TXT(" Transmission failed during communication ");
    } else {
        /** Console prints. */
        LOG_TXT(
                TX_STRING[(PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_COMM_PORT_L) | \
                          (PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_COMM_PORT_M) << 1) | \
                          (PH_EXHIF_HW_GET_GPIO_VAL(PH_EXHIF_HW_GPIO_COMM_PORT_H) << 2))]);
        /*Successful transmission */
        phLED_SetStatus(LED_Y);
    }

    return eStatus;
}

/* *****************************************************************************************************************
 * Private Functions
 * *************************************************************************************************************** */

/** Get the length of data received*/
static phStatus_t phExHif_GetRxDataLength(phhalHif_BuffId_t bBuffId,
        uint16_t *wLength) {
    *wLength = (uint16_t) PH_REG_GET_FIELD_NS_SC(
            (HOSTIF_BUFFER_RX0_LEN_REG + ((uint32_t)bBuffId*4)),
            HOSTIF_BUFFER_RX0_LEN_REG_RX0_LENGTH_MASK);
#if HOSTIF_BUFFER_RX0_LEN_REG_RX0_LENGTH_POS != 0
#   error The above logic expects that we are reading from the Lower bytes without shifting.
    /* if the reading position from register is not 0, this has to be handled */
#endif
    return PH_ERR_SUCCESS;
}

/** Transmits the data over the Host interface */
static phStatus_t phExHif_HostInterface_Transmit(uint16_t wTxLength,
        uint32_t *pdwTxData) {
    uint32_t dwBits;
    uint32_t dwHIF_TicksToWait = 0xFFFFFFFFUL;

    phStatus_t Status = PH_ERR_SUCCESS;
    do {
        /* Loop back the received frame on Hif Interface. */
        (void) phhalHif_Transmit(pdwTxData, wTxLength, &phExHif_Hw_TX_CallBack);

        /* assert the IRQ line to sync with master. */
        PH_REG_SET_BIT(PCR_PADOUT_REG, PADOUT_IRQ);

        /* For a HIF events*/
        dwBits =
                phRtos_EventGroupWaitBits(gphExHif_Hw_HifEvents, 0
                        | HIFEVT_EOT
                        | HIFEVT_RX_BUFFER_NOT_AVAILABLE
                        | HIFEVT_RX0_FRAME_UNDERFLOW
                        | HIFEVT_RX0_BUFFER_OVERFLOW
                        | HIFEVT_WATERLEVEL_REACHED
                        | HIFEVT_BUFFER_CFG_CHANGED_ERROR
                        | HIFEVT_HSU_RX_FER,
                        true, false, dwHIF_TicksToWait);

        /* De-assert the IRQ line to sync with master. */
        PH_REG_CLEAR_BIT(PCR_PADOUT_REG, PADOUT_IRQ);

        if (!(dwBits & HIFEVT_EOT)) {
            Status = PH_ERR_TX_NAK_ERROR;
            break;
        }

        dwBits = 0;

    } while (0);

    return Status;
}

/** Contact Interface transactions */
static phStatus_t phExHif_CtTransaction(uint8_t *pbAPDU, uint16_t wAPDULen) {
    phStatus_t phStatus;
    uint16_t dwNoOfResponseBytes;

    do {
        if (gphExHif_Hw_Ct_Init == false) {
            /* Activate the card */
            gphExHif_Hw_DataParams.pbAtrBuffer =
                    (uint8_t *) &gphExHif_Hw_AtrBuffer;
            gphExHif_Hw_DataParams.bSizeOfATRbuffer = 32;

            phStatus = phpalCt_ActivateCard(&gphExHif_Hw_DataParams);
            if (phStatus != PH_ERR_SUCCESS) {
                LOG_X32("Card activation failed!! ", phStatus);
                break;
            }

            gphExHif_Hw_Ct_Init = true;
            LOG_TXT("Card activated successfully\n");

            /**
             * Select the protocol depending on the supported protocol from the card
             */
            LOG_AU8("ATR", gphExHif_Hw_DataParams.pbAtrBuffer,
                    gphExHif_Hw_DataParams.bAtrReceivedLength);
            if (E_PHPAL_CT_T1 == gphExHif_Hw_DataParams.bCardSupportedProt) {
                phpalCt_SetProtocol(E_PHPAL_CT_T1);
                LOG_TXT("Card supported protocol T=1\n");
            } else if (E_PHPAL_CT_T0
                    == gphExHif_Hw_DataParams.bCardSupportedProt) {
                phpalCt_SetProtocol(E_PHPAL_CT_T0);
                LOG_TXT("Card supported protocol T=0\n");
            } else {
                /* Only for QA warnings */
            }
        }

        phStatus = phpalCt_Transceive( pbAPDU,
                (uint32_t) wAPDULen, bApduRespBuffer, &dwNoOfResponseBytes);
        PH_BREAK_ON_FAILURE(phStatus);

        /* Check the status of the command execution */
        if ((bApduRespBuffer[dwNoOfResponseBytes - 2] != 0x90)
                || (bApduRespBuffer[dwNoOfResponseBytes - 1] != 0x00)) {
            LOG_TXT("Apdu response status bytes are not 9000!!!\n");
            phLED_SetStatus(LED_R);
        } else {
            LOG_AU8("APDU Response", bApduRespBuffer,
                    (dwNoOfResponseBytes - 2));
            LOG_TXT("Apdu processed successfully");
            phLED_SetStatus(LED_Y);
        }
        /** Return the received response */
        ((uint16_t*) pbAPDU)[0] =
                (uint16_t) (bApduRespBuffer[dwNoOfResponseBytes - 2]);

    } while (0);

    return phStatus;
}

/** Generic Call back Function for HIF */
static void phExHif_Hw_Generic_CallBack(uint32_t dwIntStatus, void *pvParameter) {
    uint32_t hifEvtStatus = 0;

    if (dwIntStatus & HOSTIF_INT_STATUS_REG_BUFFER_CFG_CHANGED_ERROR_STATUS_MASK )
        hifEvtStatus |= HIFEVT_BUFFER_CFG_CHANGED_ERROR;
    if (dwIntStatus & HOSTIF_INT_STATUS_REG_EOT_STATUS_MASK )
        hifEvtStatus |= HIFEVT_EOT;
    if (dwIntStatus & HOSTIF_INT_STATUS_REG_HSU_RX_FER_STATUS_MASK )
        hifEvtStatus |= HIFEVT_HSU_RX_FER;
    if (dwIntStatus & HOSTIF_INT_STATUS_REG_RX0_BUFFER_OVERFLOW_STATUS_MASK )
        hifEvtStatus |= HIFEVT_RX0_BUFFER_OVERFLOW;
    if (dwIntStatus & HOSTIF_INT_STATUS_REG_RX0_EOR_STATUS_MASK )
        hifEvtStatus |= HIFEVT_RX0_EOR;
    if (dwIntStatus & HOSTIF_INT_STATUS_REG_RX0_FRAME_UNDERFLOW_STATUS_MASK )
        hifEvtStatus |= HIFEVT_RX0_FRAME_UNDERFLOW;
    if (dwIntStatus & HOSTIF_INT_STATUS_REG_RX_BUFFER_NOT_AVAILABLE_STATUS_MASK )
        hifEvtStatus |= HIFEVT_RX_BUFFER_NOT_AVAILABLE;
    if (dwIntStatus & HOSTIF_INT_STATUS_REG_WATERLEVEL_REACHED_STATUS_MASK )
        hifEvtStatus |= HIFEVT_WATERLEVEL_REACHED;

    if (phRtos_EventGroupSetBits(gphExHif_Hw_HifEvents, hifEvtStatus)
            != phRtos_Success) {
        PH_USER_ASSERT(0); /* It is not expected to reach here. */
    }
}

/** Receive interrupt Call back Function for HIF */
static void phExHif_Hw_RX0_CallBack(uint32_t wLength, void *pvParameter) {
    if (phRtos_EventGroupSetBits(gphExHif_Hw_HifEvents,
            HIFEVT_RX0_EOR) != phRtos_Success) {
        PH_USER_ASSERT(0); /* It is not expected to reach here. */
    }
}

/** Transmit interrupt Call back Function for HIF */
static void phExHif_Hw_TX_CallBack(uint32_t wLength, void *pvParameter) {
    if (phRtos_EventGroupSetBits(gphExHif_Hw_HifEvents,
            HIFEVT_EOT) != phRtos_Success) {
        PH_USER_ASSERT(0); /* It is not expected to reach here. */
    }
}

/** End of Call back Function*/
