/*
 * NfcrdlibEx11_ISO10373_PCD.h
 *
 *  Created on: Oct 25, 2016
 *      Author: nxp95127
 */

#ifndef INTFS_NFCRDLIBEX11_ISO10373_PCD_H_
#define INTFS_NFCRDLIBEX11_ISO10373_PCD_H_
#include <ph_Status.h>

#ifdef NXPBUILD__PHHAL_HW_PN7462AU
#define PH_ISO10373_MAX_RXBUFFSIZE    508       /**< Maximum RX buffer size for CLIF HAL. */
#define PH_ISO10373_MAX_TXBUFFSIZE    260       /**< Maximum TX buffer size for CLIF HAL. */
#else
#define PH_ISO10373_MAX_RXBUFFSIZE    600       /**< Maximum RX buffer size for CLIF HAL. */
#define PH_ISO10373_MAX_TXBUFFSIZE    600       /**< Maximum TX buffer size for CLIF HAL. */
#endif

#ifdef PH_OSAL_FREERTOS
#ifdef PHOSAL_FREERTOS_STATIC_MEM_ALLOCATION
#define ISO10373_PCD_TASK_STACK              (2000/4)
#else /* PHOSAL_FREERTOS_STATIC_MEM_ALLOCATION */
#ifdef __PN74XXXX__
#define ISO10373_PCD_TASK_STACK              (2000/4)
#else /*  __PN74XXXX__*/
#define ISO10373_PCD_TASK_STACK              (2000)
#endif /*  __PN74XXXX__*/
#endif /* PHOSAL_FREERTOS_STATIC_MEM_ALLOCATION */
#define ISO10373_PCD_TASK_PRIO                4
#endif /* PH_OSAL_FREERTOS */

#ifdef PH_OSAL_LINUX
#define ISO10373_PCD_TASK_STACK                  0x20000
#define ISO10373_PCD_TASK_PRIO                 0
#endif /* PH_OSAL_LINUX */

#endif /* INTFS_NFCRDLIBEX11_ISO10373_PCD_H_ */
