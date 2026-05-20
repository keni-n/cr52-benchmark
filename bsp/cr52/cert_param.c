/*******************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only
 * intended for use with Renesas products. No other uses are authorized. This
 * software is owned by Renesas Electronics Corporation and is protected under
 * all applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
 * LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
 * TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
 * ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
 * FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
 * ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
 * BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software
 * and to discontinue the availability of this software. By using this software,
 * you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 * Copyright 2015-2022 Renesas Electronics Corporation All rights reserved.
 *******************************************************************************/

/*******************************************************************************
 * DESCRIPTION   : Certificate header parameter
 ******************************************************************************/
/******************************************************************************
 * @file          cert_param.c
 * - Version      : 0.05
 * @brief         Set certificate header parameter.
 * .
 *****************************************************************************/
/******************************************************************************
 * History : DD.MM.YYYY Version  Description
 *         : 15.10.2021 0.01     First Release
 *         : 15.12.2021 0.02     Add V4H support
 *         : 24.02.2022 0.03     Unify variables for S4 and V4H.
 *         : 25.03.2022 0.04     Modify magic number to definition.
 *         : 09.11.2022 0.05     License notation change.
 *****************************************************************************/

#define FLASH_WRITER_TOP_ADDR   (0xEB210000U)
#define FLASH_WRITER_SIZE       (0x00010000U)   /* Flash Writer size is 256KiB (0x10000 * 4) */
#define FLASH_WRITER_SIZE_CERT  (FLASH_WRITER_SIZE * 4U)

/* For R-Car S4/V4H */
/* 0xEB20300C */
const unsigned int __attribute__ ((section (".cert_offset"))) g_reserved = 0x00000000;
/* 0xEB203154 */
const unsigned int __attribute__ ((section (".cert_addr"))) g_cert_addr = FLASH_WRITER_TOP_ADDR;
/* 0xEB203264 */
/* This parameter is not referred by BootROM when SCIF download mode. */
const unsigned int __attribute__ ((section (".cert_size"))) g_cert_size = FLASH_WRITER_SIZE_CERT;

