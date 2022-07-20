/*
 * shimutil - utilities running in UEFI shell for shim
 *
 * Copyright openSUSE, Inc
 * Author: Dennis Tseng
 * Date  : 2022/07/15
 */

#include "shimutil.h"

tCMD_FUNC cmd_tbl[] = {
	{ L"sb-state", secure_boot_state },
	{ L"sbat-state", sbat_state },
	{ L"", NULL }
};

EFI_GUID SHIM_LOCK_GUID = { 0x605dab50, 0xe046, 0x4300, {
                       		0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23 } };

static EFI_SYSTEM_TABLE *systab;
static EFI_HANDLE global_image_handle;

/*-----------------------------------------------------------------
 * parse_argv
 *----------------------------------------------------------------*/
void parse_argv(CHAR16 *str, int len, CHAR16 *argv[], int *argc)
{
	int  i,vi=0;

	for(argv[vi]=str,i=0; i<len; i++) {
		//str[i] = tolower(str[i]);
		//Print(L"vi=%d,str[%d]=%c\n",vi,i,str[i]);
		//gBS->Stall(300000); sleep 0.3 sec
		if (str[i] == ' ') {
			str[i] = '\0';
			//Print(L"\n*** argv[%d]=%s\n",vi,argv[vi]);
			for(i++; str[i] == ' '; i++);
			vi++;
			argv[vi] = &str[i];
		}
	}
	str[i] = '\0';
	//Print(L"\n*** argv[%d]=%s\n",vi,argv[vi]);
	*argc = vi+1;
}

/********************************************************************
 * secure_boot_state
 * IN : argv
 * OUT: state
 * RET: efi-status
 ********************************************************************/
void help_func(void)
{
	Print(L"Usage:\n");
	Print(L"  shimutil OPTIONS <args...>\n");
	Print(L"Options:\n");
	Print(L"  sb-state: show secure-boot state\n");
	Print(L"  sbat-state [get/set] [latest/previous/delete]: apply sbat\n");
	Print(L"\n");
}

/********************************************************************
 * secure_boot_state
 * IN : argv
 * OUT: state
 * RET: efi-status
 ********************************************************************/
EFI_STATUS
secure_boot_state(CHAR16 *argv[], int argc)
{
	EFI_GUID GLOBAL_GUID = EFI_GLOBAL_VARIABLE;
	UINT8 SecureBoot = 0; /* return false if variable doesn't exist */
	UINTN DataSize;
	EFI_STATUS efi_status;

	if (argc != 2) {
		Print(L"Error! invalid parameter for %s\n",argv[1]);
		return EFI_INVALID_PARAMETER;
	}

	DataSize = sizeof(SecureBoot);
	SecureBoot = 0;
	efi_status = RT->GetVariable(L"SecureBoot", &GLOBAL_GUID, NULL,
			     &DataSize, &SecureBoot);
	if (EFI_ERROR(efi_status)) {
		Print(L"Error! cannot get Secure-boot variable\n");
		return efi_status;
	}
	(SecureBoot ? Print(L"Secure-boot is enabled\n") : Print(L"Secure-boot is disabled\n"));
	return efi_status;
}

/********************************************************************
 * sbat_state
 * IN : argv
 * IN : argc
 * OUT: state
 * RET: efi-status
 ********************************************************************/
EFI_STATUS
sbat_state(CHAR16 *argv[], int argc)
{
	char  		*sbat_var = NULL;
	EFI_STATUS 	efi_status;
	char		data[256];
	char		buffer[512];
	UINT32 		attributes;
	UINTN		len,i;

	if (argc < 3) {
		Print(L"Error! invalid parameter for %s\n",argv[1]);
		help_func();
		return EFI_INVALID_PARAMETER;
	}

	if (StrnCmp(argv[2],L"get",StrLen(argv[2])) && StrnCmp(argv[2],L"set",StrLen(argv[2]))) {
		Print(L"Error! invalid parameter(%s)\n",argv[2]);
		return EFI_INVALID_PARAMETER;
	}

	if (StrnCmp(argv[2],L"get",StrLen(argv[2])) == 0) {
		if (argc != 3) {
			Print(L"Error! invalid parameter for %s\n",argv[1]);
			return EFI_INVALID_PARAMETER;
		}

		efi_status = RT->GetVariable(L"SbatLevel", &SHIM_LOCK_GUID, 
					&attributes, &len, data);
		if (EFI_ERROR(efi_status)) {
			Print(L"variable(%s) reading failed: %r\n",SBAT_VAR_NAME,efi_status);
			return efi_status;
		}

		Print(L"variable(%s) reading success: %r\n",SBAT_VAR_NAME,efi_status);
		Print(L"len=%d,variable=",len);
		for(i=0; i<len; i++) {
			Print(L"%c",data[i]);
		}
		Print(L"\n");
	}
	else if (StrnCmp(argv[2],L"set",StrLen(argv[2])) == 0) {
		if (argc != 4) {
			Print(L"Error! invalid parameter for %s\n",argv[1]);
			return EFI_INVALID_PARAMETER;
		}

		if (StrnCmp(argv[3],L"latest",StrLen(argv[3])) && 
		    StrnCmp(argv[3],L"previous",StrLen(argv[3])) && 
			StrnCmp(argv[3],L"delete",StrLen(argv[3]))) {
			Print(L"Error! invalid parameter(%s)\n",argv[3]);
			return EFI_INVALID_PARAMETER;
		}

		(StrnCmp(argv[3],L"latest",StrLen(argv[3])) == 0 ? sbat_var = SBAT_VAR_LATEST : (
		 StrnCmp(argv[3],L"previous",StrLen(argv[3])) == 0 ? sbat_var = SBAT_VAR_PREVIOUS : (
		 StrnCmp(argv[3],L"delete",StrLen(argv[3])) == 0 ? sbat_var = SBAT_VAR_ORIGINAL : NULL)));

		efi_status = RT->SetVariable(L"SbatLevel", &SHIM_LOCK_GUID, 
				EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
	            strlen(sbat_var), sbat_var);
		if (EFI_ERROR(efi_status)) {
			Print(L"%s variable writing failed: %r\n",SBAT_VAR_NAME,efi_status);
			return efi_status;
		}
		Print(L"variable(%s)/len(%d) writing success: %r\n",
			SBAT_VAR_NAME,strlen(sbat_var),efi_status);

		//---------------- double check after writing ------------------
		efi_status = RT->GetVariable(L"SbatLevel", &SHIM_LOCK_GUID, 
					&attributes, &len, buffer);
		if (EFI_ERROR(efi_status)) {
			Print(L"variable(%s) reading failed: %r\n",SBAT_VAR_NAME,efi_status);
			return efi_status;
		}

		Print(L"variable(%s) reading success: %r\n",SBAT_VAR_NAME,efi_status);
		Print(L"len=%d,variable=",len);
		for(i=0; i<len; i++) {
			Print(L"%c",buffer[i]);
		}
		Print(L"\n");	
	}

	return EFI_SUCCESS;
}

/**************************************************************************
 * efi_main
 **************************************************************************/
EFI_STATUS
efi_main (EFI_HANDLE passed_image_handle, EFI_SYSTEM_TABLE *passed_systab)
{
	EFI_LOADED_IMAGE *loaded_image;
	EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
	CHAR16 *argstr,*argv[16];
	int  len,cmd_idx,argc;

	EFI_STATUS efi_status;
	EFI_HANDLE image_handle;

	systab = passed_systab;
	image_handle = global_image_handle = passed_image_handle;

	/*
	 * Ensure that gnu-efi functions are available
	 */
	InitializeLib(image_handle, systab);

	efi_status = gBS->HandleProtocol(image_handle,&loaded_image_protocol,(void**)&loaded_image);
	argstr = (CHAR16*)loaded_image->LoadOptions;
	len = StrLen((CHAR16*)loaded_image->LoadOptions);
	parse_argv(argstr,len,argv,&argc);
	if (argc <= 1) {
		Print(L"Error! Please input enough parameters\n");
		help_func();
		return efi_status;
	}

	_CMD_TBL_IDX_(cmd_idx,argv[1],cmd_tbl);
	//Print(L"cmd_idx=%d, argv[1]=%s\n",cmd_idx,argv[1]);
	if (cmd_tbl[cmd_idx].cmd[0])
		cmd_tbl[cmd_idx].func(argv,argc);
	else {
		Print(L"Error! unknown request(%s)\n",argv[1]);	
		help_func();
	}
	return efi_status;
}
