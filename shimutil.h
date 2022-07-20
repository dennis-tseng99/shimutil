/*
 * shimutil - header file of shimutil.c
 *
 * Copyright openSUSE, Inc
 * Author: Dennis Tseng
 * Date  : 2022/07/15
 */

#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <efi.h>
#include <efilib.h>

#define SBAT_VAR_SIG "sbat,"
#define SBAT_VAR_VERSION "1,"
#define SBAT_VAR_ORIGINAL_DATE "2021030218"
#define SBAT_VAR_ORIGINAL SBAT_VAR_SIG SBAT_VAR_VERSION \
        SBAT_VAR_ORIGINAL_DATE "\n"

#define SBAT_VAR_PREVIOUS_DATE SBAT_VAR_ORIGINAL_DATE
#define SBAT_VAR_PREVIOUS_REVOCATIONS
#define SBAT_VAR_PREVIOUS SBAT_VAR_SIG SBAT_VAR_VERSION \
        SBAT_VAR_PREVIOUS_DATE "\n" SBAT_VAR_PREVIOUS_REVOCATIONS

#define SBAT_VAR_LATEST_DATE "2022052400"
#define SBAT_VAR_LATEST_REVOCATIONS "shim,2\ngrub,2\n"
#define SBAT_VAR_LATEST SBAT_VAR_SIG SBAT_VAR_VERSION \
        SBAT_VAR_LATEST_DATE "\n" SBAT_VAR_LATEST_REVOCATIONS

#define SBAT_VAR_NAME L"SbatLevel"
#define UEFI_VAR_NV_BS \
        (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)
#define SBAT_VAR_ATTRS UEFI_VAR_NV_BS

typedef EFI_STATUS (*FUNCPTR)();

typedef struct {
	CHAR16 *cmd;
	FUNCPTR func;
} tCMD_FUNC;

#define _CMD_TBL_IDX_(entry,t,table) \
	for(entry=0; table[entry].cmd[0] && StrnCmp(table[entry].cmd,t,StrLen(t)); entry++);

EFI_STATUS print_abc(CHAR16 *argv[], int argc);
EFI_STATUS secure_boot_state(CHAR16 *argv[], int argc);
EFI_STATUS sbat_state(CHAR16 *argv[], int argc);
