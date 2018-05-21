#define TRUE   1
#define FALSE  0
#define NULL   0

#define NUM_CONFIGURATIONS      1
#define NUM_INTERFACES          1

#define ConfiguredUSB() while ((USWSTAT&0x3)!=CONFIG_STATE) {}
#define STALL_EP0  EP_STALL0 = 1
#define Send_0Len_pkt			BD0IBC = 0;	BD0IST = 0xc8;	USB_dev_req = NULL

static volatile unsigned char bank3	USW_STAT	@ 0x197;

#define POWERED_STATE	        0x00
#define DEFAULT_STATE	        0x01
#define ADDRESS_STATE	        0x02
#define CONFIG_STATE	        0x03

#define ENDPT_DISABLED	        0x00
#define ENDPT_IN_ONLY	        0x01
#define ENDPT_OUT_ONLY	        0x02
#define ENDPT_CONTROL	        0x06	/* enable for in, out and setup */
#define ENDPT_NON_CONTROL		0x0E	/* enable for in, and out */

#define TOKEN_OUT	(0x01<<2)
#define TOKEN_ACK	(0x02<<2)
#define TOKEN_IN	(0x09<<2)
#define TOKEN_SETUP	(0x0D<<2)

#define CLEAR_FEATURE	        0x01
#define GET_CONFIGURATION		0x08
#define GET_DESCRIPTOR	        0x06
#define GET_INTERFACE	        0x0A
#define GET_STATUS	        	0x00
#define SET_ADDRESS	        	0x05
#define SET_CONFIGURATION		0x09
#define SET_FEATURE	        	0x03
#define SET_INTERFACE	        0x0B
#define HID_SET_REPORT			0x21
#define HID_GET_REPORT			0xA1
#define VEND_SET_MEMORY	        0x80

#define HOSTTODEVICE            0x00
#define HOSTTOINTERFACE         0x01
#define HOSTTOENDPOINT          0x02
#define DEVICETOHOST            0x80
#define INTERFACETOHOST         0x81
#define ENDPOINTTOHOST          0x82

#define EP0OUT                  0
#define EP0IN                   1
#define EP1OUT                  2
#define EP1IN                   3
#define EP2OUT                  4
#define EP2IN                   5

// Descriptor Types
#define DEVICE                  1
#define CONFIGURATION           2
#define STRING                  3
#define INTERFACE               4
#define ENDPOINT                5



