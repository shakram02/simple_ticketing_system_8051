#include <at89c51xd2.h>
#include <stdio.h>

#define KEYPAD_NO_NEW_DATA '-'
#define INCREMENT_KEY '#'
#define DECREMENT_KEY '-'
#define CLEAR_KEY '+'

sbit r0 = P1^7;
sbit r1 = P1^6;
sbit r2 = P1^5;
sbit r3 = P1^4;

sbit c0 = P1^3;
sbit c1 = P1^2;
sbit c2 = P1^1;
sbit c3 = P1^0;

sbit seg_select = P3^7;
unsigned long overflow_count = 0;
char disp_vals[] = {0,0};
bit mux_index=0;

// Obtained from data sheet and modified a little bit
// for better shapes
char seven_seg_map[] ={126,48,109,121,51,91,95,112,127,123};
char pressed_key;

static char LAST_VALID_KEY_G = KEYPAD_NO_NEW_DATA;

char disp_seven_seg(char);
char scan_keypad();
char disp_vals_to_int();

void int_to_disp_vals(char);
void setup_timer();
void timer0_isr();
void setup_serial();
void setup_seven_seg();
void send_char(unsigned char);
void debounce();

void main()
{
  bit result;
	
	P1 = 0x0F;
	
	r0 = 1;
	r1 = 1;
	r2 = 1;
	r3 = 1;

	setup_timer();
	setup_serial();
	setup_seven_seg();
 
	printf("Testing Keypad program\r\n");
	while(1)
	{
		result = scan_keypad();	
		
		if(result == 1)
		{
			// Rotate the 7-seg displayed character
			char old_disp_val = disp_vals[0];
			
			if (pressed_key >= '0' && pressed_key <= '9'){
				disp_vals[0] = pressed_key;
				disp_vals[1] = old_disp_val;
			
				send_char(pressed_key);	
			}
			else {
				// inc / dec
				char num = disp_vals_to_int();
				char op_result = 0;
				if (pressed_key == INCREMENT_KEY ){
					// inc, modulo is used to return the value to 0 once it's 100
					op_result = (num+1) % 100;
				}
				else if (pressed_key == DECREMENT_KEY){
					// dec, return to 99 if the decrementation result is -1
					op_result = (num - 1) < 0 ? 99 : (num - 1);
				}
				
				int_to_disp_vals(op_result);
			}
			
		}
	}
}

/* Converts the ASCII input to 7-seg value ready for display */
char disp_seven_seg(char in){
	char c = in - '0';
	return (c < 0 || c > 9) ? 0xFF : ~seven_seg_map[c];
}

/* Initialize seven segment to display 00 */
void setup_seven_seg(){
	disp_vals[0] = '0';
	disp_vals[1] = '0';
}

void timer0_isr() interrupt 1
{
	overflow_count++;
	TH0 = 0xFC;	// 0.5 msec
	TL0 = 0x65;
	
	if (overflow_count == 500){
		seg_select = ~seg_select;	/* Change the 7-seg */
		
		P0 = disp_seven_seg(disp_vals[mux_index]);
		mux_index=~mux_index;
		
		overflow_count = 0;
	}
}

/* Returns the decimal for the displayed ASCII chars */
char disp_vals_to_int(){
	char units = disp_vals[0] - '0';
	char tens = disp_vals[1] - '0';
	
	return (tens*10) + units;
}

/* Converts the input number to ascii*/
void int_to_disp_vals(char in){
	char units = in % 10;
	char tens = in / 10;
	
	disp_vals[0] = units + '0';
	disp_vals[1] = tens + '0';
}

/* Sends a character using serial uart */
void send_char(unsigned char ch)
{
    SBUF = ch;
		while(TI==0);
		TI = 0;
}

/* Set timer 0 as interrupt to fire each 0.5 msec */
void setup_timer(){
	TMOD |=  0x01;
	
	TH0 = 0xFC;	// 0.5 msec
	TL0 = 0x65;
	
	ET0 = 1; /* Enable timer interrupts */
	TR0 = 1; /* Run timer 0 */
	
	EA = 1;	/* Global interrupt enable*/
}

/* Setup the serial port for 9600 baud at 22.1184MHz.*/
void setup_serial()
{
    SCON  = 0x50;		        		/* SCON: mode 1, 8-bit UART, enable rcvr      */
    TMOD 	|=  0x20;             /* TMOD: timer 1, mode 2, 8-bit reload        */
    TH1   = 250;                /* TH1:  reload value for 9600 baud @ 22.1184MHz   */
    TR1   = 1;                  /* TR1:  timer 1 run                          */
		TI = 1;											/* Setup serial timer interrupt */
}

/* Get the debounced keypad character */
char scan_keypad()
{
	  static char old_key;
	  char key = KEYPAD_NO_NEW_DATA;

	  r0 = 0;
		     if(c3 == 0) {key = 'A'; }
		else if(c2 == 0) {key = '3'; }
		else if(c1 == 0) {key = '2'; }
		else if(c0 == 0) {key = '1'; }
		r0 = 1;


		r1 = 0;
			   if(c3 == 0) {key =  'B'; }
		else if(c2 == 0) {key =  '6'; }
		else if(c1 == 0) {key =  '5'; }
		else if(c0 == 0) {key =  '4'; }		
		r1 = 1;


		r2 = 0;
		if(c3 == 0) {key = 'C'; }
		if(c2 == 0) {key = '9'; }
		if(c1 == 0) {key = '8'; }
		if(c0 == 0) {key = '7'; }		
		r2 = 1;

		r3 = 0;
		     if(c3 == 0) {key = 'D'; }
		else if(c2 == 0) {key = '#'; }
		else if(c1 == 0) {key = '0'; }
		else if(c0 == 0) {key = '*'; }
		
		r3 = 1;
		debounce();
		
		
		if(key == KEYPAD_NO_NEW_DATA)
		{
			old_key = KEYPAD_NO_NEW_DATA;
			LAST_VALID_KEY_G = KEYPAD_NO_NEW_DATA;
			return 0;			
		}
		
		if(key == old_key)
		{
			if(key != LAST_VALID_KEY_G)
			{
				pressed_key = key;				
				LAST_VALID_KEY_G = key;				
				return 1;
			}
		}
		old_key = key	;			
		return 0;
}

/* Debounce keypad */
void debounce()
{
	int i = 100;
	int j = 1000;

	for(;i>0;i--)
		for(; j>0;j--)
			;
}
