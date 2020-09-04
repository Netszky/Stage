#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hsm_plugin/matrix_barcodeid.h"
#include "hsm_plugin/matrix_plugin.h"
#include "hsm_plugin/matrix_format_plugin.h"
#include "hsm_plugin/matrix_beep_led.h"

#include "SafestorFormat.h"

/*===========================================================================================================
End of Header file declarations
===========================================================================================================*/

void printhex(void* input, int len){
        int i;
        unsigned char* pByte=(unsigned char*)input;
        for(i=0; i<len; i++){
                printf("0x%02x, ", *pByte);
                pByte++;
        }
}

/*===========================================================================================================
Start of Macro declarations
===========================================================================================================*/

/*===========================================================================================================
Plug-in properties should be defined by Plug-in developer.
Plug-in developers have to contact Honeywell to get PLUGIN_GUID,CERTIFICATE and CERTIFICATE_TIME for
their plugin.
===========================================================================================================*/
#define PLUGIN_NAME					SafestorFormat
#define COMPANY_NAME				Safestor.
#define MAJOR_VERSION				5
#define MINOR_VERSION				3
#define BUILD_NUMBER				37
#define CERTIFICATE					102148
#define CERTIFICATE_TIME			2010/10/06 10:05:00
#define PLUGIN_GUID					abcd1234
#define FILE_NAME					SafestorFormat.plugin


//Macros defined for message lengths
#define MESSAGE_LENGTH				 100
/*===========================================================================================================
End of Macro declarations
===========================================================================================================*/

/*===========================================================================================================
Start of Global variable definitions
===========================================================================================================*/
int MatrixPluginCheckLicense(char *SN);


/*===========================================================================================================
Format Plugin APIs definition. Fill the fields with user defined APIs.
===========================================================================================================*/
static DataEditApi apis = {
	.RevisionNumber = 1,
	.DataEdit = MatrixPluginDataEdit,
	.ProcessingBarcode = MatrixPluginProcessingBarcode,
	.CheckLicense = MatrixPluginCheckLicense,
	.GetVersion = MatrixPluginGetVersion,
};

/*===========================================================================================================
End of Global variable definitions
===========================================================================================================*/

/*===========================================================================================================
Start of function declarations
===========================================================================================================*/

/*===========================================================================================================
We should declare the plug-in by using 'DECLARE_PLUGIN' macro in order that Matrix can recognize the
plug-in.
===========================================================================================================*/

int init_plugin(HONPluginRawInfo *plugin);
void cleanup_plugin(void);

DECLARE_PLUGIN(init_plugin, cleanup_plugin, HON_PLUGIN_FORMAT, MenuID);

/*===========================================================================================================
End of function declarations
===========================================================================================================*/
/*===========================================================================================================
Start of function definitions
===========================================================================================================*/


/*===========================================================================================================
FUNCTION NAME	  : MatrixPluginDataEdit

TIME OF CALL	  :	This function will be called by Firmware whenever a data barcode is read.

PURPOSE		  :	This MatrixPluginProcessingBarcode() is called to perform data format

RETURN VALUES   :	Returns 0 if the formatting is successful else returns -1.

ADDITIONAL INFO :	The plug-in developer should implement this routine by himself and set address of this
function to the "DataEdit" field of the	"DataEditApi" structure.

In this Safestor plug-in,

// Symbology	Length	Accepted		Structure		Rejected	Transmit
// Code 39		13		A..Z, 0..9		n/a				All Others	13
// Code 128		13		A..Z, 0..9		n/a				All Others	13
// Code 128		25		A..Z, 0..9		1-11: A..Z,0..9	All Others	25
//										12,13: A..Z
//										14-25: A..Z,0..9
// Code 128		28		A..Z, 0..9, %	1: %			All Others	28
//										2-28: A..Z,0..9	
// All Others	any										All			None

===========================================================================================================*/

int MatrixPluginDataEdit(DataEditParam *pFormatParam)
{
	// Add your Format code here and copy the result back to pFormatParam->message.
	printf("\r\nMatrixPluginDataEdit ID=%d Len=%d Char=%d\r\n", pFormatParam->HHPcodeID, pFormatParam->length, pFormatParam->CharSize );

	printf("\r\n");
	printhex(pFormatParam->message , pFormatParam->length * pFormatParam->CharSize);
	printf("\r\n");
	int i=0;
	int j=1;
	int ret=0;
	int cs = pFormatParam->CharSize;

	//check if that is really 4 byte/char
	if(cs==4){
		//check for cs2 string end (0x00, 0x00)
		if(
		   pFormatParam->message[(pFormatParam->length * 2) + 1] ==0 && 
		   pFormatParam->message[ pFormatParam->length * 2] ==0 ) 
		{
			printf("this is a fake 4 bytes/char file! Setting CharSize=2\n");
			cs=2;
		}
	}

	// if code 39,length is 13 and characters are A-Z or 0-9, return read code
	if ((pFormatParam->HHPcodeID == WA_CODELETTER_CODE39)&& (pFormatParam->length == 13))
	{
		while(ret==0&&(i<pFormatParam->length))
		{
			if ((((pFormatParam->message[i*cs] >='A')&& (pFormatParam->message[i*cs] <='Z'))||((pFormatParam->message[i*cs] >='0')&& (pFormatParam->message[i*cs] <='9')))) 
			{
				i++;
				ret = 0;
			}
			else 
			{
				ret = 1;
				pFormatParam->length=0;
				printf ("Code 39 -13 mal\r\n");                            
				printf ("code retour = -1\r\n");
				return -1;

			}
		}
		if (ret==0)
		{
			printf ("Code 39-13 bon\r\n");                            
			return 0;
		}
	}
	else
		// if code 128, length is 13 and characters are A-Z or 0-9, return read code
		if ((pFormatParam->HHPcodeID == WA_CODELETTER_CODE128)&& (pFormatParam->length == 13))
		{

			while(ret==0&&(i<pFormatParam->length))
			{
				if ((((pFormatParam->message[i*cs] >='A')&& (pFormatParam->message[i*cs] <='Z'))||((pFormatParam->message[i*cs] >='0')&& (pFormatParam->message[i*cs] <='9')))) 
				{
					i++;
					ret = 0;
				}
				else 
				{
					ret = 1;
					pFormatParam->length=0;
					printf ("Code 128-13 mal\r\n");                            
					printf ("code retour = -1\r\n");
					return -1;
				}
			}
			if (ret==0)  
			{
				printf ("Code 128-13 bon\r\n");                            
				return 0;
			}
		}   
		else
			// if code 128, and length is 28, check for first character is %

			if ((pFormatParam->HHPcodeID == WA_CODELETTER_CODE128)&&(pFormatParam->length ==28)&&(pFormatParam->message[0] =='%'))
			{
				while(ret==0&&(j<pFormatParam->length))
				{
					if ((((pFormatParam->message[j*cs] >='A')&& (pFormatParam->message[j*cs] <='Z'))|| ((pFormatParam->message[j*cs] >='0')&& (pFormatParam->message[j*cs] <='9')))) 
					{
						j++;
						ret = 0;
					}
					else 
					{
						ret =1;
						pFormatParam->message[0] = '#';
						printf ("Code 128-28 mal\r\n");                            
						printf ("code retour = -1\r\n");
						return -1;
					}
				}
				if (ret==0)  
				{
					printf ("Code 128-28 bon\r\n");                            
					return 0;
				}
			}
			else 
				// if code 128, and length is 25, check for character 12 and 13 are ALPHA
				if ((pFormatParam->HHPcodeID == WA_CODELETTER_CODE128)&&(pFormatParam->length ==25)&&((pFormatParam->message[11*cs] >='A')&& (pFormatParam->message[11*cs] <='Z'))&& ((pFormatParam->message[12*cs] >='A')&& (pFormatParam->message[12*cs] <='Z')))
				{
					while(ret==0&&(i<pFormatParam->length))
					{
						if ((((pFormatParam->message[i*cs] >='A')&& (pFormatParam->message[i*cs] <='Z'))||((pFormatParam->message[i*cs] >='0')&& (pFormatParam->message[i*cs] <='9')))) 
						{
							i++;
							ret = 0;
						}
						else 
						{
							ret = 1;
							pFormatParam->length=0;
							printf ("Code 128-25 mal\r\n");                            
							printf ("code retour = -1\r\n");
							return -1;
						}
					}
					if (ret==0)          
					{
						printf ("Code 128-25 bon\r\n");                            
						return 0;
					}
				}
				else 
					pFormatParam->length=0;

	printf ("Mauvais Code\r\n");
	printf ("code retour = -1\r\n");
	return -1;
}

/*===========================================================================================================
FUNCTION NAME	  : MatrixPluginProcessingBarcode

TIME OF CALL	  :	This function will be called by Firmware whenever programming barcodes are scanned or
provided as menu commands.

PURPOSE		  :	The MatrixPluginProcessingBarcode API is called when plug-in needs to be configured
by scanning a programming barcode.

RETURN VALUES   :	Returns 0 if a valid programming barcode is scanned or sent else returns -1.
Device will beep error or success according to the return value

ADDITIONAL INFO :	A set of commands needs to be sent in sequence to configure the	plug-in.
1. Enable plug-in
2. Start configuration
3. Data fields need to be read along with required formats and separators.
4. End configuration

Provide codes for each data field,separator and format supported.

License Key can be provided as a menu command in the following format.
9902FF0AAKSERIAL_NUMBER, where FF0AA is the Plug-in ID.
ex: if serail number of the device is 10071A022E then the license key menu comamnd is
9902FF0AAK10071A022E
===========================================================================================================*/

int MatrixPluginProcessingBarcode(char *pMenuData, int DataLength)
{
	printf("Safestorformat - MatrixPluginProcessingBarcode\r\n");
	printf("Safestorformat - Menu Data is: ");
	printf(pMenuData);
	printf("\r\nSafestorformat - length: %d\r\n",DataLength);
	printf("\r\n");

	beep_led_io(MenuDifferentBeepSeq, 6);
	return 0;
}


/*===========================================================================================================
FUNCTION NAME	  : MatrixPluginCheckLicense

TIME OF CALL	  :	This function will be called by Firmware when the plug-in is loaded or the device is
connected to the host PC.

PURPOSE		  :	The MatrixPluginCheckLicense API is used to validate if the plug-in is licensed or not

RETURN VALUES   :	Returns 0 if Plug-in is licensed else returns -1

ADDITIONAL INFO :	License Key for the Plug-in can be provided in three ways.
1. License Key file as a moc file
2. License Key as a serial menu command
3. License Key as a barcode

MatrixPluginCheckLicense() function will check for the license	if the License Key is
provided in a file as a moc file.
MatrixPluginProcessingBarcode() function will be called if License Key is provided
as Serial Menu Command or Barcode.
===========================================================================================================*/

int MatrixPluginCheckLicense(char *SN)
{
	return 0;
}

/*===========================================================================================================
FUNCTION NAME	  : MatrixPluginGetVersion

TIME OF CALL	  :	None - Not supported by Firmware.

PURPOSE		  :	This function is used to retrieve plug-in version information. The version information must
needs to filled in pInfo structure.

RETURN VALUES   :	returns 0 for success and -1 for failure

ADDITIONAL INFO :	None - Not supported by Firmware.

===========================================================================================================*/

int MatrixPluginGetVersion(VersionInfo *pInfo)
{
	printf("Safestorformat - MatrixPluginGetVersion\r\n");
	return 0;
}

/*===========================================================================================================

FUNCTION NAME	  : init_plugin

TIME OF CALL	  :	This function will be called by firmware whenever the device is connected to the host PC.

PURPOSE		  :	This is the initial function of plug-in call routine.

RETURN VALUES   :	Returns 0 to indicate success else returns -1.

ADDITIONAL INFO :	This routine should be defined by the plug-in developers to initilize plug-in and
register plug-in APIs.
===========================================================================================================*/

int init_plugin(HONPluginRawInfo *plugin)
{
	int				ret					= 0;

	printf("/******************************/\r\n");
	printf("     Safestor Plugin\r\n");
	printf("       _cs4_v002_\r\n");
	printf("/******************************/\r\n");

	//Register the plug-in APIs
	ret = register_apis(plugin, &apis);
	if(ret)
	{
		printf("Safestor - Plugin APIs Registration Failed\r\n");
		return -1;
	}

	printf("Safestor - Plugin MenuID = %05X\r\n", plugin->MenuIdentifier);

	return 0;
}

/*===========================================================================================================
FUNCTION NAME	  : cleanup_plugin

TIME OF CALL	  :	None - Not supported by Firmware.

PURPOSE		  :	This is the cleanup function of plug-in

RETURN VALUES   :	None

ADDITIONAL INFO :	None - Not supported by Firmware.
===========================================================================================================*/

void cleanup_plugin(void)
{
	printf("Safestor Plugin - cleanup_plugin \r\n");
	return;
}

