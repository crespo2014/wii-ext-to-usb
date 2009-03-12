/*
 * Wii extension controller to USB Hid project
 * kero905
 * v 0.01a
 * License: GNU General Public License Version 2 (GPL)
 */

#include <avr/io.h>		// include I/O definitions (port names, pin names, etc)
#include <avr/interrupt.h>	// include interrupt support
#include <util/delay.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>   /* required by usbdrv.h */

#include "usbdrv/usbdrv.h"

	// Get declaration for f(int i, char c, float x)
#include "includes/global.h"		// include our global settings
#include "includes/uart.h"		// include uart function library
#include "includes/rprintf.h"

#include "twi/twi.h"		// include i2c support

#define Ob(x)  ((unsigned)Ob_(0 ## x ## uL))
#define Ob_(x) (x & 1 | x >> 2 & 2 | x >> 4 & 4 | x >> 6 & 8 |		\
	x >> 8 & 16 | x >> 10 & 32 | x >> 12 & 64 | x >> 14 & 128)


/* ------------------------------------------------------------------------- */
/* ----------------------------- Controller stuff -------------------------- */
/* ------------------------------------------------------------------------- */

unsigned char nunchuck_buf[7];
unsigned char nunchuck_cmd_buf[3];
unsigned char i2c_return;
unsigned char status[6];
unsigned char buttons[2];


// set by: nunchuck_detect()
unsigned char controller_type = 0; // 0 = Nunchuck  1 = Classic Controller

unsigned char offset_x = 115;
unsigned char offset_y = 122;

// following not used might be used for calibrating in the future
// set by: nunchuck_calibrate()
int centered_x = 0;
int centered_y = 0;
int r_stick_x_zero = 0;
int r_stick_y_zero = 0;
int l_stick_x_zero = 0;
int l_stick_y_zero = 0;
/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM char usbHidReportDescriptor[91] = { // USB report descriptor, size must match usbconfig.h
	    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
	    0x09, 0x05,                    // USAGE (Joystick)
	    0xa1, 0x01,                    // COLLECTION (Application)


	    0x05, 0x09,                    //     USAGE_PAGE (Button)
	    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
	    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
	    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
	    0x29, 0x0D,                    //     USAGE_MAXIMUM (Button 13)
	    0x75, 0x01,                    //     REPORT_SIZE (1)
	    0x95, 0x0D,                    //     REPORT_COUNT (13)
	    0x81, 0x02,                    //     INPUT (Data,Var,Abs)

	    0x95, 0x01, 					//report count 1
	    0x75, 0x03,						// Blank
	    0x81, 0x01, 					//Input something or other


	    0x05, 0x01,					// USAGE_PAGE (Generic Desktop)
	    0x25, 0x07, 					//   LOGICAL_MAXIMUM (7)
	    0x46, 0x3b, 0x01, 				//physical maximum 315
	    0x75, 0x04, 					//Report size 4    --39
	    0x95, 0x01, 					//Report count 1
	    0x65, 0x14,						//unit: English angular position
	    0x09, 0x39, 					//usage: hat switch
	    0x81, 0x02,						//     INPUT (Data,Var,Abs)
        0x65, 0x00, 					//Unit:none    --49
  	    0x95, 0x01, 					//report count 1
	    0x75, 0x04,
	    0x81, 0x01, 					//Input something or other

	    0x09, 0x01,                    //   USAGE (Pointer)
	    0xa1, 0x00,                    //   COLLECTION (Physical)

/* a failed attempt to add 4-bits of data for right sholder button pressure
 * 		0x09, 0x34,					   //     USAGE (Ry)
		0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
		0x26, 0x10,              	   //     LOGICAL_MAXIMUM (16)
		0x75, 0x04,                    //     REPORT_SIZE (4)
		0x95, 0x01,                    //     REPORT_COUNT (1)
		0x81, 0x02,                    //     INPUT (Data,Var,Abs)
*/
	    0x09, 0x30,                    //     USAGE (X)
	    0x09, 0x31,                    //     USAGE (Y)
	    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
	    0x26, 0xff, 0x00,              //     LOGICAL_MAXIMUM (255)
	    0x75, 0x08,                    //     REPORT_SIZE (8)
	    0x95, 0x02,                    //     REPORT_COUNT (2)
	    0x81, 0x02,                    //     INPUT (Data,Var,Abs)

	    0x09, 0x32,                    //     USAGE (Z)
		0x09, 0x33,					   //     USAGE (Rx)
	    0x09, 0x35,                    //     USAGE (Rz)
	    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
	    0x26, 0xff, 0x00,              //     LOGICAL_MAXIMUM (255)
	    0x75, 0x08,                    //     REPORT_SIZE (8)
	    0x95, 0x03,                    //     REPORT_COUNT (3)
	    0x81, 0x02,                    //     INPUT (Data,Var,Abs)

	    0xc0,                          //     END_COLLECTION
	    0xc0                          // END_COLLECTION
};


static uchar    reportBuffer[8];    // buffer for HID reports
static uchar    idleRate;   // repeat rate for keyboards, never used for mice
static uchar but_arr1 = 0;
static uchar but_arr2 = 1;
static uchar hat_arr = 2;
static uchar lx_arr = 3;
static uchar ly_arr = 4;
static uchar rx_arr = 5;
static uchar ry_arr = 7;
static uchar shouldl = 6;


usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    // The following requests are never used. But since they are required by
     // the specification, we implement them in this example.

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    // class request type
//        DBG1(0x50, &rq->bRequest, 1);   // debug output: print our request
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  // wValue: ReportType (highbyte), ReportID (lowbyte)
            // we only have one report type, so don't look at wValue
    		reportBuffer[0]= 0;
    		reportBuffer[1]= 0;
    		reportBuffer[2]= 0;
    		reportBuffer[3]= 0;
    		reportBuffer[4]= 0;
    		reportBuffer[5]= 0;
    		reportBuffer[6]= 0;
    		reportBuffer[7]= 0;

            usbMsgPtr = (void *)&reportBuffer;
            return sizeof(reportBuffer);
        }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = &idleRate;
            return 1;
        }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];
        }
    }else{
        // no vendor specific requests implemented
    }
    return 0;   // default for not implemented requests: return no data back to host
}

void delayms(uint16_t millis) {
  while ( millis ) {
    _delay_ms(1);
    millis--;
  }
}


static void nunchuck_init()
{
	nunchuck_cmd_buf[0] = 0x40;
	nunchuck_cmd_buf[1] = 0x00;
	i2c_return = twi_writeTo(0x52, nunchuck_cmd_buf, 2, 1);
}


/*
 * Check address 0xFE and 0xFF,
 * Nunchuk = 0xFEFE (encrypted)
 * ClassicC = 0xFDFD (encrypted)
*/
static void nunchuck_detect()
{
	nunchuck_cmd_buf[0] = 0xFE;
	delayms(5);
	i2c_return = twi_writeTo(0x52, nunchuck_cmd_buf, 1, 1);
	delayms(10);
	i2c_return = twi_readFrom(0x52, nunchuck_buf, 2);
	if ((nunchuck_buf[0] == 0xFE)&&(nunchuck_buf[1] == 0xFE)) {
		rprintf("detect nunchuck ");
		controller_type = 0;
	} else if ((nunchuck_buf[0] == 0xFD)&&(nunchuck_buf[1] == 0xFD)) {
		rprintf("detect ClassicC ");
		controller_type = 1;
	}
}

// Dump return data from nunchuck for debugging purpose
static void nunchuck_dump_data()
{
	int cnt = 0;
	rprintf("detect ret: ");
	for (cnt = 0; cnt <= 255 ; cnt++) {
		nunchuck_cmd_buf[0] = cnt;
		delayms(5);
		i2c_return = twi_writeTo(0x52, nunchuck_cmd_buf, 1, 1);
		delayms(20);
		i2c_return = twi_readFrom(0x52, nunchuck_buf, 1);
		rprintfNum(10, 4, FALSE, ' ', cnt);
		rprintf(" : ");
		rprintfNum(16, 3, FALSE, ' ', nunchuck_buf[0]);
		rprintf("\r\n ");
	}
}

static void nunchuck_send_request()
{
    nunchuck_cmd_buf[0] = 0x00;// sends one byte
    i2c_return = twi_writeTo(0x52, nunchuck_cmd_buf, 1, 1);

}

static char nunchuk_decode_byte (char x)
{
    x = (x ^ 0x17) + 0x17;
    return x;
}
// get data from nunchuck
static int nunchuck_get_data()
{
	int cnt = 0;
	i2c_return = twi_readFrom(0x52, nunchuck_buf, 6); // request data from nunchuck
	delayms(13);  //wait some time before sending request.. raise time if controller doesn't respond
 //   if (i2c_return == 6) {
    	for (cnt = 0; cnt <= 5 ; cnt++) {
            if (cnt < 4) {
                status[cnt] = nunchuk_decode_byte (nunchuck_buf[cnt]);
            } else {
                //lastButtons[cnt-4] = buttons[cnt-4];
            	status[cnt] = nunchuk_decode_byte (nunchuck_buf[cnt]);
                buttons[cnt-4] = nunchuk_decode_byte (nunchuck_buf[cnt]);
            }
    	}
 //   }
    nunchuck_send_request();
    return 1;   // success
}

// returns zbutton state: 1=pressed, 0=notpressed
static int nunchuck_zbutton()
{
	return !(status[5] & Ob(00000001) );
}

// returns zbutton state: 1=pressed, 0=notpressed
static int nunchuck_cbutton()
{
	return !( (status[5] & Ob(00000010) ) >> 1);
}


static int PressedRowBit(uchar row, uchar bit) {
	uchar mask = (1 << bit);
	if (!(buttons[row] & mask ) ) {
    	return 1;
	} else {
		return 0;
	}
}

void press_x() { reportBuffer[but_arr1] = reportBuffer[but_arr1] | (0x01 << 3); // x
}
void press_y() { reportBuffer[but_arr1] = reportBuffer[but_arr1] | 0x01; // y
}
void press_a() {reportBuffer[but_arr1] = reportBuffer[but_arr1] | (0x01 << 2); // a
}
void press_b() { reportBuffer[but_arr1] = reportBuffer[but_arr1] | (0x01 << 1); //b
}
void press_l() { reportBuffer[but_arr1] = reportBuffer[but_arr1] | (0x01 << 4); //LS
}
void press_r() { reportBuffer[but_arr1] = reportBuffer[but_arr1] | (0x01 << 5); //RS
}

// Todo: calibrate
void nunchuck_calibrate() {
	r_stick_x_zero = ((((status[0] & 0xc0) >> 3) + ((status[1] & 0xc0) >> 5) +  ((status[2] & 0x80) >> 7)) <<3) - 128;
	r_stick_y_zero = ((status[2] & 0x1f) <<3) - 128 ;
	l_stick_x_zero = ((status[0] & 0x3f) <<2) - 128;
	l_stick_y_zero = ((status[1] & 0x3f) <<2) - 128;

}

void make_sense_classic() {
	reportBuffer[but_arr1]= 0;
	reportBuffer[but_arr2]= 0;
	reportBuffer[hat_arr]= 0x08;
	reportBuffer[lx_arr]= 0x80;
	reportBuffer[ly_arr]= 0x80;
	reportBuffer[rx_arr]= 0x80;
	reportBuffer[ry_arr]= 0x80;
	reportBuffer[shouldl]= 0x80;

	int r_stick_x = ((((status[0] & 0xc0) >> 3) + ((status[1] & 0xc0) >> 5) +  ((status[2] & 0x80) >> 7)) << 3);
	unsigned char r_stick_y = ((status[2] & 0x1f) << 3);
	int  l_stick_x = ((status[0] & 0x3f) <<2);
	unsigned char l_stick_y = ((status[1] & 0x3f)<<2);

/*	//Debug
     rprintfNum(10, 5, TRUE, ' ', l_stick_x);
	    rprintfNum(10, 5, TRUE, ' ', l_stick_y);
   rprintfNum(10, 5, TRUE, ' ', r_stick_x);
    rprintfNum(10, 5, TRUE, ' ', r_stick_y);
*/
// Todo: calibrate sticks
	reportBuffer[lx_arr] = l_stick_x;
	reportBuffer[ly_arr] = 255-l_stick_y;
	reportBuffer[rx_arr] = r_stick_x;
	reportBuffer[ry_arr] = 255-r_stick_y;

// Hat switch, there has to be a better way.. took the following from Toodles's UPCB code
// here: http://forums.shoryuken.com/showthread.php?t=131230
	if(PressedRowBit(1,0)) 	{
		reportBuffer[hat_arr]=0x00;
		if(PressedRowBit(0,7)) reportBuffer[hat_arr]=0x01;
		if(PressedRowBit(1,1)) reportBuffer[hat_arr]=0x07;
	} else {
		if(PressedRowBit(0,6)) {
			reportBuffer[hat_arr]=0x04;
			if(PressedRowBit(0,7)) reportBuffer[hat_arr]=0x03;
			if(PressedRowBit(1,1)) reportBuffer[hat_arr]=0x05;
		} else { //neither up nor down
			if(PressedRowBit(0,7)) reportBuffer[hat_arr]=0x02;
			if(PressedRowBit(1,1)) reportBuffer[hat_arr]=0x06;
		}
	}
	// Buttons
// select
	if (PressedRowBit(0,4)) {
		reportBuffer[but_arr2] = reportBuffer[but_arr2] | (0x01 << 0);
		//rprintf("select.");
	}

	// Home doesn't seem to work on the PS3 as the PS button
	if (PressedRowBit(0,3)) {
		reportBuffer[but_arr2] = reportBuffer[but_arr2] | (0x01 << 4);
		//rprintf("home.");
	}
//start
	if (PressedRowBit(0,2)) {
		reportBuffer[but_arr2] = reportBuffer[but_arr2] | (0x01 << 1);
		//rprintf("start.");
	}

	if (PressedRowBit(1,3)) {
		press_x();
	}

	if (PressedRowBit(1,5)) {
		press_y();
	}

	if (PressedRowBit(1,4)) {
		press_a();
	}

	if (PressedRowBit(1,6)) {
		press_b();
	}
	if (PressedRowBit(0,5)) {
		press_l();
	}
	if (PressedRowBit(0,1)) {
		press_r();
	}
	if (PressedRowBit(1,7)) {
		reportBuffer[but_arr1] = reportBuffer[but_arr1] | (0x01 << 6);
	}
	if (PressedRowBit(1,2)) {
		reportBuffer[but_arr1] = reportBuffer[but_arr1] | (0x01 << 7);
	}

	// shoulder button pressure
	reportBuffer[shouldl] = (255 - (((status[2] & 0x60) >> 2) + ((status[3] & 0xe0) >> 5) << 3)); //left

}


void make_sense_nunchuck() {
	reportBuffer[but_arr1]= 0;
	reportBuffer[but_arr2]= 0;
	reportBuffer[hat_arr]= 0x08; // center hat
	reportBuffer[lx_arr]= 0x80;
	reportBuffer[ly_arr]= 0x80;
	reportBuffer[5]= 0;
	reportBuffer[6]= 0;
	reportBuffer[7]= 0;

	// joystick
	reportBuffer[lx_arr] = (status[0]);
	reportBuffer[ly_arr] = (255-status[1]);

	// buttons
	if (nunchuck_zbutton()) {
		reportBuffer[but_arr1] = reportBuffer[but_arr1] | (0x01 << 1);
	}
	if (nunchuck_cbutton()) {
		reportBuffer[but_arr1] = reportBuffer[but_arr1] | (0x01 << 2);
	}

	// accel
	reportBuffer[5] = ((status[2] << 2) + ((status[5] & (3 << 2 ) >> 2)) >> 2);
	reportBuffer[6] = ((status[3] << 2) + ((status[5] & (3 << 4 ) >> 4)) >> 2);
	reportBuffer[7] = ((status[4] << 2) + ((status[5] & (3 << 6 ) >> 6)) >> 2);

}
int main(void) {
	uchar i = 0;
	twi_init();
	uartInit();                 // initialize UART (serial port)
    uartSetBaudRate(115200);      // set UART speed to 9600 baud.. no 115200 ! rawr!
    rprintfInit(uartSendByte);  // configure rprintf to use UART for output

    // Start up nunchuck (or other wii ext)
    nunchuck_init();
    nunchuck_detect();
    //nunchuck_calibrate();

    usbInit();
    usbDeviceDisconnect();  // enforce re-enumeration, do this while interrupts are disabled!
    i = 0;
    while(--i){             // fake USB disconnect for > 250 ms
    	wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    sei();
    for(;;){                // main event loop
    	wdt_reset();
        usbPoll();
        if(usbInterruptIsReady()){
            // called after every poll of the interrupt endpoint
        	nunchuck_get_data();

        	if (controller_type == 0) {
        		make_sense_nunchuck();
        	} else if (controller_type == 1) {
        		make_sense_classic();
        	}
            rprintfCRLF();
            usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
        }
    }
	return 0;

}


