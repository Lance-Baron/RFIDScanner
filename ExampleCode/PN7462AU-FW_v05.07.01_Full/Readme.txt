  _______________________________________________________________
                   NXP Semiconductors
        NXP NFC Reader Library v05.07.01 - Full
            PN7462AU HAL version  4.11.0.2         
                        Readme.txt
  _______________________________________________________________
  
  

Table of Contents
_________________

1 Document Purpose
2 Description of the package
3 Restrictions
4 Configurations
5 Package Contents
6 Mandatory materials (not included)
7 Hardware Configuration
8 Software Configuration
9 Steps to follow
10 List of supported NFC Reader Boards/ICs
11 Changelog

1 Document Purpose
==================

  This document describes the content of the NXPNfcRdLib package and steps how 
  to import it to the MCUXpresso IDE as well, known problems and restrictions.


2 Description of the package
============================

  - The package contains implementation of the NXPNfcReaderLibrary support
    and examples to demonstrate how to use library.
  - This package is preconfigured for PN7462AU


3 Restrictions
==============

  - Package contains implementation for PN7462AU only and it does not provide 
    support for any other platform.
  - Application projects are prepared for MCUXpresso IDE. 
  

4 Configurations 
================
 
  - Package is preconfigured for PN7462AU.
  
  
5 Package Contents
==================

 - Readme.txt
    + This readme file
  - NXP_SLDA.pdf 
    + NXP Software License and Distribution Agreement    
  - FreeRTOS_Library 
    + FreeRTOS support
  - PN7462AU 
    + SDK support
  - Nfcrdlib_SimplifiedAPI_EMVCo 
    + project package of the example
  - Nfcrdlib_SimplifiedAPI_ISO 
    + project package of the example
  - NfcrdlibEx1_BasicDiscoveryLoop 
    + project package of the example
  - NfcrdlibEx2_AdvancedDiscoveryLoop
    + project package of the example 
  - NfcrdlibEx3_NFCForum 
    + project package of the example      
  - NfcrdlibEx4_MIFAREClassic 
    + project package of the example
  - NfcrdlibEx5_ISO15693 
    + project package of the example
  - NfcrdlibEx7_EMVCo_Polling 
    + project package of the example
  - NfcrdlibEx8_HCE_T4T
    + project package of the example
  - NfcrdlibEx9_NTagI2C
    + project package of the example
  - NfcrdlibEx11_ISO10373_PCD 
    + project package of the example
  - PN7462AU_ex_phExCcid
    + project package of the example  
  - PN7462AU_ex_phExCt
    + project package of the example  
  - PN7462AU_ex_phExCT7816
    + project package of the example  
  - PN7462AU_ex_phExCTEMVCo
    + project package of the example  
  - PN7462AU_ex_phExDoorAccess
    + project package of the example  
  - PN7462AU_ex_phExEMVCo
    + project package of the example  
  - PN7462AU_ex_phExHif
    + project package of the example  
  - PN7462AU_ex_phExMain
    + project package of the example  
  - PN7462AU_ex_phExNFCCcid
    + project package of the example  
  - PN7462AU_ex_phExPos
    + project package of the example  
  - PN7462AU_ex_phExRf
    + project package of the example  
  - PN7462AU_ex_phExRfPCDA
    + project package of the example  
  - PN7462AU_ex_phExRfPoll
    + project package of the example  
  - PN7462AU_ex_phExSystemServices
    + project package of the example  
  - PN7462AU_ex_phExVCom 
    + project package of the example 
  - NxpNfcRdLib
    + NXPNfcReaderLibrary package

  
6 Mandatory materials (not included)
====================================

  - MCUXpresso IDE:
    [http://www.nxp.com/products/software-and-tools/run-time-software/mcuxpresso-software-and-tools/mcuxpresso-integrated-development-environment-ide:MCUXpresso-IDE?tid=vanMCUXPRESSO/IDE]
  - LPC-Link2 stand-alone debug probe	
  - PNEV7462B Customer Demo Board




7 Hardware Configuration
========================

  Before starting this application, HW Changes may be required for the used
  board.  Refer to the following User Manuals / Application notes before
  starting with this example.

  - UM10883: PN7462AU Quick Start Guide - Customer Board
  
  
8 Software Configuration
========================
  
  - The Software can be compiled for PN7462AU NFC controller.
  
  
9 Steps to follow
=================

  Steps for importing the package in MCUXpresso IDE (For MCUXpressio versions before 
  MCUXpressoIDE_10.0.2 PN7462 plugin needs to be installed): 
  
  
  1) For running this application, connect the board listed in
     Section-10

  2) Create a new MCUXpresso workspace and import this project and it's
     dependencies:

     For PN7462AU : NxpNfcRdLib, PN7462AU and FreeRTOS_Library needs to be imported
        
      Steps to import the project:
      
      - Select "Import Project(s)" in Quickstart panel
      
      - Browse to the PN7462AU-FW_Full.zip file
      
      - Keep default setings in the wizard and finish it.
       

  3) Ensure MCU Type in "Project Properties" --> "C/C++ Build" --> "MCU
     Settings" for this project and its dependencies as listed in Step-2 is
     set Correctly:

     For PN7462AU : PN74xxxx --> PN7462AU-C3-00

  4) Ensure Build Configuration for this project is selected correctly.
     Where allowed, set the build configuration of dependent projects also.

     For PN7462AU : DebugPN7462AU / ReleasePN7462AU

  5) Build the project.

  6) Start the "Debug" session of IDE which will first flash the executable
     and start the task.

  7) When this application is running on the target MCU setup and when NFC cards
     or phone in proximity, the example application will detect and report the NFC 
	 technology types detected.
  

10 List of supported NFC Reader Boards/ICs
==========================================

  1) PNEV7462B v2.1/v2.2 Customer Demo Board

  
11 Changelog
=============
05.07.01
- PN7462 iAR and Keil project are updated

05.07.00
- PN5180 HAL Optimization
    HAL Exchange Optimization:
    HAL PN5180 was optimized to reduce EMVCo Transaction time by reducing number
    of BAL Exchanges with PN5180 FE which are not required for EMVCo Transaction.
    Inventory Read and Inventory Page Read Error:
    When multiple SLIX (ISO15693) card are placed in the field, PN5180 fails to detect 
    the collision when Inventory Read and Inventory Page Read commands are sent, 
    as PN5180 returns Timeout instead of Collision error.
    HAL PN5180 error check logic is changed to give preference to Collision error over Integrity error.
    Poll Guard Time Optimization:
    Poll guard time optimization was done in NFC Reader Library generically to meet 
    EMVCo Poll guard time requirement on different platforms (Host Microcontroller + NXP FE’s)
    with constant guard time values. As the Poll guard time was made constant across Platforms,
    when PN5180 is used in Linux environment with i.MX6-UL or similar controller, 
    EMVCo Poll guard time test cases are inconsistent.
    Instead of using CLIF timer, DAL timer is used to wait for Poll guard time to expire before sending Poll command.
- Discovery Loop
    EMVCo polling loop is optimized by moving generic code that needs to be executed first time, 
    instead of executing in a loop. Also, the FDT of HLTA is been considered while configuring GTB before sending WupB command.
- PAL 15693 FDT configuration did not consider the ASK configuration of HAL. 
    PAL 15693 is now updated to configure FDT based on the ASK configuration of HAL as per ISO15693 Specification.
    
05.02.00
– OSAL update
    OSAL can now be used as an independent library and is not coupled with NFC
    Reader Library, which enables the use of OSAL in systems which may or may not
    use NFC Reader Library. If any other system (ex: mPOS, Secure Card Reader)
    where NFC Reader Library is optional and is using OSAL for OS abstraction, then
    there is no overhead in the integration of NFC Reader Library within the system.
    After OSAL changes, if the customer needs to port NFC Reader Library on any other
    RTOS or different different controller then NFC Reader Library does not need to be
    changed any more but only changes are required in OSAL which is outside NFC Reader 
    Library which makes porting easier than before without the need of changing 
    NFC Reader Library code base.
– DAL (Driver Abstraction Layer):   
    DAL provides API’s to abstract Microcontroller GPIO functionalities like configuring
    a pin to input/output/interrupt, reading and writing to pins. DAL also provides generic
    BAL abstraction layer required for NFC Reader Library.
    If NFC Reader Library needs to be ported on any other Host Microcontroller or
    Microcontroller SDK then NFC Reader Library need not be changed but changes are
    only required to be done in DAL.
- PN5180 Digital Delay:
    NFC Reader Library PN5180 HAL will read the FW version of PN5180 during HAL
    initialization and uses this FW version information to either apply digital delay inside
    HAL of NFC Reader Library or PN5180 FW.
    On PN5180 firmwrae version 3.6 and below the digital delay is applied by host software
    (NFC Reader Library), on firmware version 3.8 and newer the digital delay is applied by
    PN5180 firmware.
- AL ICode component updated
- AL ICode DNA component added
- Type 5 Tag support updated
- Polling guard time setting fixed

4.06.00
- CLRC663 (plus) - Low Power Card Detection (LPCD) improvements
  Additional options are provided to configure different I/Q thresholds when LPCD Filter
  is enabled to choose either between stable detection with power saving (Option1) or higher detection range (Option2).
- Antenna related configuration for CLRC663 (plus) is moved into separate header file
- ISO18000-3 mode 3 (Icode-ILT) tag detection improved (CLRC663 (plus) only).

4.050.03.011702
- ICODE DNA Command Support
- MIFARE DESFIRE EV2 Command Support
- MIFARE PLUS EV1 Command Support
- EMVCo 2.6 Updates
- CLRC663OB/V1A LPCD Algorithm
- CLRC663OB/V1A 65*65 Antenna RF Settings in EEPROM Support
- NDEF Operations on MIFARE Classic Tag
- Removed PN512 support. For PN512 support refer to versions before version 4.050.03.001651
- Removed support for LPC11U68.
- Pin connections for Kinetis FRDM-K82F and Raspberry Pi have been updated. Please
  refer to the Start-up Guides respectively. AN11908, AN11802   

  
  ---------------------------------------------------------------------------

  For updates of this example, see
  [http://www.nxp.com/pages/:NFC-READER-LIBRARY]