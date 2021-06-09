////+++++++++++++++++++++++++++++++++++++| LIBRARIES / HEADERS |+++++++++++++++++++++++++++++++++++++
#include "device_config.h"
#include <xc.h>
#include <math.h>
#include <stdint.h>
#include <time.h>

//+++++++++++++++++++++++++++++++++++++| DIRECTIVES |+++++++++++++++++++++++++++++++++++++
#define _XTAL_FREQ 1000000
#define SWEEP_FREQ 20
#define ONE_SECOND 1000
#define LCD_DATA_R          PORTB
#define LCD_DATA_W          LATB
#define LCD_DATA_DIR        TRISB
#define LCD_RS              LATDbits.LATD2
#define LCD_RS_DIR          TRISDbits.TRISD2
#define LCD_RW              LATDbits.LATD1
#define LCD_RW_DIR          TRISDbits.TRISD1
#define LCD_E               LATDbits.LATD0
#define LCD_E_DIR           TRISDbits.TRISD0

//+++++++++++++++++++++++++++++++++++++| DATA TYPES |+++++++++++++++++++++++++++++++++++++
enum por_dir{ output, input };              // output = 0, input = 1
enum por_ACDC { digital, analog };          // digital = 0, analog = 1
enum resistor_state { set_ON, res_ON };     // set_ON = 0, res_ON = 1
enum led_state { led_OFF, led_ON };         // led_OFF = 0, led_ON = 1
enum butto_state { pushed, no_pushed };     // pushed = 0, no_pushed = 1

//+++++++++++++++++++++++++++++++++++++| ISRs |+++++++++++++++++++++++++++++++++++++
// ISR for high priority
void __interrupt ( high_priority ) high_isr( void );
// ISR for low priority
void __interrupt ( low_priority ) low_isr( void ); 

//+++++++++++++++++++++++++++++++++++++| FUNCTION DECLARATIONS |+++++++++++++++++++++++++++++++++++++
void LCD_rdy(void);
void LCD_init(void);
void LCD_cmd(char);
void send2LCD(char);
void portsInit(void);
char int_to_char_d1(int);
char int_to_char_d2(int);
char int_to_char_d3(int);
void delay_1s(void);		//Funcion de retardo 1s
unsigned int periodo1 = 0;		//Guarda primera captura
unsigned int periodo2 = 0;
unsigned int periodo_total = 0;
int resultado = 0;

//+++++++++++++++++++++++++++++++++++++| MAIN |+++++++++++++++++++++++++++++++++++++
void main( void ){
    LCD_init();
    LCD_DATA_DIR = 0x00;
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_E  = 0;
    
    periodo1 = 0;		//Guarda primera captura
    periodo2 = 0;
    periodo_total = 0;
    resultado = 0;            //Para un despligue en watch
    portsInit();

    while(1){
        CCP1CON = 0X05;
        CCPTMRS = 0x00;
        //Configuramos el Timer 1 (T1CON), 16 bits
        T1CONbits.TMR1CS = 0b00;        //Fuente de reloj FOSC/4
        T1CONbits.T1CKPS = 0b11;        //Prescalador 1:8
        T1CONbits.T1SOSCEN = 0b0;       //Oscilador secundario deshabilitado
        T1CONbits.T1SYNC = 0b1;         //No sincronizar con reloj externo
        //Configuramos registro del Gate
        T1GCONbits.TMR1GE = 0;          //No requerimos control del gate

        //Arrancar el timer 1
        T1CONbits.TMR1ON = 1;

        while(PIR1bits.CCP1IF == 0);		//Esperamos evento de transicion
        periodo1 = CCPR1;                      //Leer valor capturado
        PIR1bits.CCP1IF = 0;			//Borramos bandera de evento

        while(PIR1bits.CCP1IF == 0);	       //Esperamos evento de transicion
        periodo2 = CCPR1;                      //Leer valor capturado
        PIR1bits.CCP1IF = 0;			//Borramos bandera de evento
        T1CONbits.TMR1ON = 0;		       //Apagamos el timer 1

        periodo_total = periodo2 - periodo1;   //Diferencia me da el total.

        //Para despliegue en un watch de var.
        resultado = 31250/periodo_total;
        LCD_E = 1;
        __delay_ms(25);
        send2LCD('F');
        __delay_ms(25);
        send2LCD('r');
        __delay_ms(25);
        send2LCD('e');
        __delay_ms(25);
        send2LCD('c');
        __delay_ms(25);
        send2LCD('u');
        __delay_ms(25);
        send2LCD('e');
        __delay_ms(25);
        send2LCD('n');
        __delay_ms(25);
        send2LCD('c');
        __delay_ms(25);
        send2LCD('i');
        __delay_ms(25);
        send2LCD('a');
        __delay_ms(25);
        send2LCD(':');
        __delay_ms(25);
        LCD_cmd(0xC0);          // Set cursor to 2 line 
        char disp1 = int_to_char_d3(resultado);
        __delay_ms(25);
        send2LCD(disp1);
        char disp2 = int_to_char_d1(resultado);
        __delay_ms(25);
        send2LCD(disp2);
        char disp3 = int_to_char_d2(resultado);
        send2LCD(disp3);
        __delay_ms(25);
        send2LCD('H');
        __delay_ms(25);
        send2LCD('z');
        __delay_ms(25);
        LCD_E = 0;
        delay_1s();
        LCD_cmd(0x01);           // Clear display and move cursor home
    }
}

//+++++++++++++++++++++++++++++++++++++| FUNCTIONS |+++++++++++++++++++++++++++++++++++++
// Function to wait until the LCD is not busy
void LCD_rdy(void){
    char test;
    // configure LCD data bus for input
    LCD_DATA_DIR = 0b11111111;
    test = 0x80;
    while(test){
        LCD_RS = 0;         // select IR register
        LCD_RW = 1;         // set READ mode
        LCD_E  = 1;         // setup to clock data
        test = LCD_DATA_R;
        Nop();
        LCD_E = 0;          // complete the READ cycle
        test &= 0x80;       // check BUSY FLAG 
    }
    LCD_DATA_DIR = 0b00000000;
}

// LCD initialization function
void LCD_init(void){
    LATC = 0;               // Make sure LCD control port is low
    LCD_E_DIR = 0;          // Set Enable as output
    LCD_RS_DIR = 0;         // Set RS as output 
    LCD_RW_DIR = 0;         // Set R/W as output
    LCD_cmd(0x38);          // Display to 2x40
    LCD_cmd(0x0F);          // Display on, cursor on and blinking
    LCD_cmd(0x01);          // Clear display and move cursor home
}

// Send command to the LCD
void LCD_cmd(char cx) {
    LCD_rdy();              // wait until LCD is ready
    LCD_RS = 0;             // select IR register
    LCD_RW = 0;             // set WRITE mode
    LCD_E  = 1;             // set to clock data
    Nop();
    LCD_DATA_W = cx;        // send out command
    Nop();                  // No operation (small delay to lengthen E pulse)
    //LCD_RS = 1;             // 
    //LCD_RW = 1;             // 
    LCD_E = 0;              // complete external write cycle
}

// Function to display data on the LCD
void send2LCD(char xy){
    LCD_RS = 1;
    LCD_RW = 0;
    LCD_E  = 1;
    LCD_DATA_W = xy;
    Nop();
    Nop();
    LCD_E  = 0;
}

// Ports initialization function
void portsInit(void){
    TRISCbits.TRISC2 = 1;     // Input
    ANSELCbits.ANSC2 = 0;     // Digital
}


// Funcion que convierte el primer digito del resultado de un entero a un caracter
char int_to_char_d3(int N){
    int A = N/100;
    char p3;
    switch(A){
        case 0:
            p3 = '0';
        break;
        case 1:
            p3 = '1';
            resultado = resultado - 100;
        break;
        case 2:
            p3 = '2';
            resultado = resultado - 200;
        break;
        case 3:
            p3 = '3';
            resultado = resultado - 300;
        break;
        case 4:
            p3 = '4';
            resultado = resultado - 400;
        break;
        case 5:
            p3 = '5';
            resultado = resultado - 500;
        break;
        case 6:
            p3 = '6';
            resultado = resultado - 600;
        break;
        case 7:
            p3 = '7';
            resultado = resultado - 700;
        break;
        case 8:
            p3 = '8';
            resultado = resultado - 800;
        break;
        case 9:
            p3 = '9';
            resultado = resultado - 900;
        break;
        default:
        break;
    }
    return p3;
}

// Funcion que convierte el primer digito del resultado de un entero a un caracter
char int_to_char_d1(int N){
    int A = N/10;
    char p1;
    switch(A){
        case 0:
           p1 = '0';
        break;
        case 1:
            p1 = '1';
        break;
        case 2:
            p1 = '2';
        break;
        case 3:
            p1 = '3';
        break;
        case 4:
            p1 = '4';
        break;
        case 5:
            p1 = '5';
        break;
        case 6:
            p1 = '6';
        break;
        case 7:
            p1 = '7';
        break;
        case 8:
            p1 = '8';
        break;
        case 9:
            p1 = '9';
        break;
        default:
        break;
    }
    return p1;
}

// Funcion que convierte el segundo digito del resultado de un entero a un caracter
char int_to_char_d2(int N){
    int B = N%10;
    char p2;
    switch(B){
        case 0:
            return p2 = '0';
        break;
        case 1:
            return p2 = '1';
        break;
        case 2:
            return p2 = '2';
        break;
        case 3:
            return p2 = '3';
        break;
        case 4:
            return p2 = '4';
        break;
        case 5:
            return p2 = '5';
        break;
        case 6:
            return p2 = '6';
        break;
        case 7:
            return p2 = '7';
        break;
        case 8:
            return p2 = '8';
        break;
        case 9:
            return p2 = '9';
        break;
        default:
        break;
    }
}

void delay_1s(void){
        //Count 40536 or 0x9E58 ticks
	TMR0H = 0x0B;			//High byte 0x0BDC
	TMR0L = 0xDC;			//Low bte de 0x0BDC
	INTCONbits.TMR0IF = 0; 	//Clear the timer overflow flag
	//Configure the timer
	//16 bits
	//Set a 16 pre-scaler
	T0CON = 0b10000011;
	while(INTCONbits.TMR0IF == 0); //Wait for overflow
	T0CON = 0X00;		           //Stop the timer
}
