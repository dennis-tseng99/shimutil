#include <stdio.h>
#include <stdint.h>
#include <include/console.h>

/*----------------------------------------------------
 * CHAR16_TO_CHAR8
 *---------------------------------------------------*/
void  CHAR16_TO_CHAR8(CHAR16 *imsg, char *omsg)
{
	UINTN i,j;
	char *cp=(char*)imsg;

	for(i=0,j=0; cp[i]; i+=2,j++) {
        omsg[j] = cp[i]; 
	} 
	omsg[j] = 0;
}

/********************************************************************
 * PRINT_MESSAGE()
 *
 ********************************************************************/
void  PRINT_MESSAGE(unsigned char *msg, int len)
{
	int  row_cnt,rows,rest_bytes,hex_cnt,ch_cnt,cnt,xi,ci;
	
	if (NULL == msg){
		console_print(L"PRINT_MESSAGE(): NULL message ?\n");
		return;
	}
	
	/*if ((len*5) > 2048){ // 5 format bytes for one raw data byte
		console_print("Too large[len(%d) > max(%d)] to print out!\n",len,2048);
		return;
	}*/
	
	rest_bytes = len % 16;
	rows = len / 16;
	ci = xi = 0;
	
	for(row_cnt=0; row_cnt<rows; row_cnt++){
		/*------------- print label for each row --------------*/
		console_print(L"%04x:  ",row_cnt<<4);
		
		/*------------- print hex-part --------------*/
		for(hex_cnt=1; hex_cnt<=8; hex_cnt++){
		    if (hex_cnt < 8)
		        console_print(L"%02x ",msg[xi++]); /* Must be unsigned, otherwise garbage displayed */
		    else
		        console_print(L"%02x",msg[xi++]); /* Must be unsigned, otherwise garbage displayed */
		}
		
		/* delimiters space for each 8's Hex char */
		console_print(L"  ");
		
		for(hex_cnt=9; hex_cnt<=16; hex_cnt++){
		    if (hex_cnt < 16)
		        console_print(L"%02x ",msg[xi++]);
		    else
		        console_print(L"%02x",msg[xi++]);
		}
		
		/* delimiters space bet. Hex and Character row */
		console_print(L"    ");
		
		/*------------- print character-part --------------*/
		for(ch_cnt=1; ch_cnt<=16; ch_cnt++,ci++){
			if (msg[ci]>0x20 && msg[ci]<=0x7e){
		    	console_print(L"%c",msg[ci]);
		    }
		    else{
		    	console_print(L".");
			}
		}
		console_print(L"\n");
	} //for
	
	/*================ print the rest bytes(hex & char) ==================*/
	if (rest_bytes == 0) {
		console_print(L"\n");
		return;
	}
	
	/*------------- print label for last row --------------*/
	console_print(L"%04x:  ",(row_cnt+1)<<4);
		
	/*------------- print hex-part(rest) --------------*/
	if (rest_bytes < 8){
	    for(hex_cnt=1; hex_cnt<=rest_bytes; hex_cnt++){
	        console_print(L"%02x ",msg[xi++]);
	    }
	
	    /* fill in the space for 16's Hex-part alignment */
	    for(cnt=rest_bytes+1; cnt<=8; cnt++){ /* from rest_bytes+1 to 8 */
	        if (cnt < 8)
	            console_print(L"   ");
	        else
	            console_print(L"  ");
	    }
	
	    /* delimiters bet. hex and char */
	    console_print(L"  ");
	
	    for(cnt=9; cnt<=16; cnt++){
	        if (cnt < 16)
	            console_print(L"   ");
	        else
	            console_print(L"  ");
	    }
	    console_print(L"    ");
	}
	else if (rest_bytes == 8){
	    for(hex_cnt=1; hex_cnt<=rest_bytes; hex_cnt++){
	        if (hex_cnt < 8)
	            console_print(L"%02x ",msg[xi++]);
	        else
	            console_print(L"%02x",msg[xi++]);
	    }
	    console_print(L"  ");
	
	    for(cnt=9; cnt<=16; cnt++){
	        if (cnt < 16)
	            console_print(L"   ");
	        else
	            console_print(L"  ");
	    }
	    console_print(L"    ");
	}
	else{ /* rest_bytes > 8 */
	    for(hex_cnt=1; hex_cnt<=8; hex_cnt++){
	        if (hex_cnt < 8)
	            console_print(L"%02x ",msg[xi++]);
	        else
	            console_print(L"%02x",msg[xi++]);
	    }
	
	    /* delimiters space for each 8's Hex char */
	    console_print(L"  ");
	
	    for(hex_cnt=9; hex_cnt<=rest_bytes; hex_cnt++){ /* 9 - rest_bytes */
	        if (hex_cnt < 16)
	            console_print(L"%02x ",msg[xi++]);
	        else
	            console_print(L"%02x",msg[xi++]);
	    }
	
	    for(cnt=rest_bytes+1; cnt<=16; cnt++){
	        if (cnt < 16)
	            console_print(L"   ");
	        else
	            console_print(L"  ");
	    }
	    /* delimiters space bet. Hex and Character row */
	    console_print(L"    ");
	} /* else */
	
	/*------------- print character-part --------------*/
	for(ch_cnt=1; ch_cnt<=rest_bytes; ch_cnt++,ci++){
		if (msg[ci]>0x20 && msg[ci]<=0x7e){
	        console_print(L"%c",msg[ci]);
	    }
	    else
	        console_print(L".");
	}
	console_print(L"\n");
}

