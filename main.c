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
#define LCD_RS              LATCbits.LATC2
#define LCD_RS_DIR          TRISCbits.TRISC2
#define LCD_RW              LATCbits.LATC1
#define LCD_RW_DIR          TRISCbits.TRISC1
#define LCD_E               LATCbits.LATC0
#define LCD_E_DIR           TRISCbits.TRISC0
#define VAR_SIGNAL          PORTAbits.RA4

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
int signaltime(void);
unsigned long t0 = 0;
unsigned unsigned long t1 = 0;
unsigned long deltaT = 0;
double frecuencia = 0;

//+++++++++++++++++++++++++++++++++++++| MAIN |+++++++++++++++++++++++++++++++++++++
void main( void ){
    LCD_init();
    OSCCON = 0b01110110;// Set the internal oscillator to 8MHz and stable
    LCD_DATA_DIR = 0x00;
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_E  = 0;
    portsInit();
    while(1){
        signaltime();
        frecuencia = 1000/deltaT;
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
        char disp1 = int_to_char_d3(frecuencia);
        __delay_ms(25);
        send2LCD(disp1);
        char disp2 = int_to_char_d1(frecuencia);
        __delay_ms(25);
        send2LCD(disp2);
        char disp3 = int_to_char_d2(frecuencia);
        __delay_ms(25);
        send2LCD(disp3);
        LCD_E = 0;
        __delay_ms(3000);
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
    ANSELA = digital;   // Set port A as Digital for keypad driving
    TRISA = 0b00010000;       // For Port A, set pin 1 as an input
    
}

// Function that inits Timer
void init_timer2(void){
    RCONbits.IPEN = 1;              // Enable priority in interruption
    INTCON2bits.INTEDG0 = 1;        // Int0 (RB0)) interrupts on positive transit
    INTCONbits.INT0IF = 0;          // Clear flag INT0
    INTCONbits.INT0IE = 1;          // Enable individual interrupt for INT0
    INTCONbits.GIEH = 1;            // Global enable for High priority interruptd
    INTCONbits.GIEL = 1;            // Global enable for Low Priority interrupt
    /*PIE1bits.TMR2IE = 1;          // Enable interrupt on match for timer2
    IPR1bits.TMR2IP = 1;            // Interrupt is high priority
    PIR1bits.TMR2IF = 0;            // Clear the interrupt flag
    INTCONbits.GIEH = 1;            // Habilitacion global de las de prioridad alta
    INTCONbits.GIEL = 0;            // No hay ninguna asignada por el momento
    PR2 = 124;                      // 125 * 8 * 4 * (4 * 6.25 e -8) = 1e-3
    T2CON = 0b00111101;             // Post scale 8, Timer On y Prescale = 4*/
}

// ISR for Timer2
void __interrupt (high_priority) high_priority_ISR(void){
    //VAR_SIGNAL = LED_D2 ^ 0x01;
    PIR1bits.TMR2IF = 0;            // Apagar bandera
}


int signaltime(void){
  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  t1 = localtime ( &rawtime );
  deltaT = t1 - t0;
  t0 = t1;
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
            frecuencia = frecuencia - 100;
        break;
        case 2:
            p3 = '2';
            frecuencia = frecuencia - 200;
        break;
        case 3:
            p3 = '3';
            frecuencia = frecuencia - 300;
        break;
        case 4:
            p3 = '4';
            frecuencia = frecuencia - 400;
        break;
        case 5:
            p3 = '5';
            frecuencia = frecuencia - 500;
        break;
        case 6:
            p3 = '6';
            frecuencia = frecuencia - 600;
        break;
        case 7:
            p3 = '7';
            frecuencia = frecuencia - 700;
        break;
        case 8:
            p3 = '8';
            frecuencia = frecuencia - 800;
        break;
        case 9:
            p3 = '9';
            frecuencia = frecuencia - 900;
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
