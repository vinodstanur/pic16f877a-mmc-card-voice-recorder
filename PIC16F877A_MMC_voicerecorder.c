/*
------------------------------------------------------------------------------------
PIC16F877A + MMC voice recored (no file system)
------------------------------------------------------------------------------------
COMPILER: HI-TECH C , TARGET uC PIC16F877A
------------------------------------------------------------------------------------
by Vinod.S <vinodstanur@gmail.com>
------------------------------------------------------------------------------------
*/
#include<pic.h>
#define _XTAL_FREQ 20e6
__CONFIG(0x3F3A);
#define CS RC2
#define RS RB2
#define EN RB1
#define fst cmd(0x80)
#define snd cmd(0xc0)

unsigned char readdata, u;
unsigned int count;
unsigned long int arg = 0;
/*-----------------LCD BEGIN------------------------------*/
void LCD_STROBE(void)
{
    EN = 1;
    __delay_us(0.5);
    EN = 0;
}
 
void data(unsigned char c)
{
    RS = 1;
    __delay_us(40);
    PORTD = (c >> 4);
    LCD_STROBE();
    PORTD = (c);
    LCD_STROBE();
}
 
void cmd(unsigned char c)
{
    RS = 0;
    __delay_us(40);
    PORTD = (c >> 4);
    LCD_STROBE();
    PORTD = (c);
    LCD_STROBE();
}
 
void clear(void)
{
    cmd(0x01);
    __delay_ms(2);
}
 
void lcd_init()
{
    __delay_ms(20);
    cmd(0x30);
    __delay_ms(1);
    cmd(0x30);
    __delay_ms(1);
    cmd(0x30);
    cmd(0x28);            // Function set (4-bit interface, 2 lines, 5*7Pixels)
    cmd(0x28);            // Function set (4-bit interface, 2 lines, 5*7Pixels)
    cmd(0x28);            // Function set (4-bit interface, 2 lines, 5*7Pixels)
    cmd(0x0c);            // Make cursorinvisible
    clear();
    clear();            // Clear screen
    cmd(0x6);            // Set entry Mode
}
 
void string(const char *q)
{
    clear();
    while (*q) {
        data(*q++);
    }
}
 
void istring(unsigned int q)
{
    cmd(0x81);
    data(48 + (q / 100));
    q %= 100;
    data(48 + (q / 10));
    q %= 10;
    data(48 + (q));
    __delay_ms(500);
}
 
/*-----------------------LCD END------------------------*/
/*-----------------------USRT BEGIN--------------------*/
void usrt_init()
{
    TRISC6 = 0;
    TXSTA = 0b00100110;
    RCSTA = 0b11010000;
    SPBRG = 10;
}
 
void printf(const char *p)
{
    while (*p) {
        TXREG = *p;
        while (TRMT == 0);
        p++;
    }
}
 
void txd(unsigned char vv)
{
    TXREG = vv;
    while (TRMT == 0);
}
 
/*-----------------------USRT END-----------------------*/
/*----------------------PWM BEGINS--------------------*/
void pwm_init()
{
    TRISC1 = 0;
    T2CKPS1 = 0;
    T2CKPS0 = 0;
    PR2 = 0x50;
    CCPR2L = 0x17;
    TMR2ON = 1;
    CCP2CON = 0b00001100;
}
 
void pwm_disable()
{
    CCP2CON = 0b00000000;
}
 
void pwm_enable()
{
    CCP2CON = 0b00001100;
}
 
/*--------------------PWM END-------------------------*/
/*-------------------MMC BEGIN-----------------------*/
void spi_init()
{
    TRISC4 = 1;
    RC2 = 1;
    RC3 = 0;
    RC5 = 0;
    TRISC2 = TRISC3 = TRISC5 = 0;
    SSPCON = 0b00100010;
    SSPEN = 1;
    SMP = 1;
    CKE = 1;
    CKP = 0;
}
 
void spi_write(unsigned char kk)
{
    SSPBUF = kk;
    while (BF == 0);
}
 
void spi_read()
{
    SSPBUF = 0xff;
    while (BF == 0);
    readdata = SSPBUF;
}
 
void command(char command, unsigned long int fourbyte_arg, char CRCbits)
{
    spi_write(0xff);
    spi_write(0b01000000 | command);
    spi_write((unsigned char) (fourbyte_arg >> 24));
    spi_write((unsigned char) (fourbyte_arg >> 16));
    spi_write((unsigned char) (fourbyte_arg >> 8));
    spi_write((unsigned char) fourbyte_arg);
    spi_write(CRCbits);
    spi_read();
}
 
void mmc_init()
{
    CS = 1;
    for (u = 0; u < 50; u++) {
        spi_write(0xff);
    }
    CS = 0;
    __delay_ms(1);
    command(0, 0, 0x95);
    count = 0;
    while ((readdata != 1) && (count < 1000)) {
        spi_read();
        count++;
    }
    if (count >= 1000) {
        string("CARD ERROR-CMD0 ");
        while (1);
    }
    command(1, 0, 0xff);
    count = 0;
    while ((readdata != 0) && (count < 1000)) {
        command(1, 0, 0xff);
        spi_read();
        count++;
    }
    if (count >= 1000) {
        string("CARD ERROR-CMD1 ");
        while (1);
    }
    command(16, 512, 0xff);
    count = 0;
    while ((readdata != 0) && (count < 1000)) {
        spi_read();
        count++;
    }
    if (count >= 1000) {
        string("CARD ERROR-CMD16");
        while (1);
    }
    string("MMC INITIALIZED!");
    __delay_ms(1000);
    SSPCON = SSPCON & 0b11111101;
}
 
void write()
{
    pwm_disable();
    command(25, arg, 0xff);
    while (readdata != 0) {
        spi_read();
        string("WRITE ERROR");
    }
    string("WRITING MMC");
    while (1) {
        spi_write(0xff);
        spi_write(0xff);
        spi_write(0b11111100);
        for (int g = 0; g < 512; g++) {
            GO = 1;
            while (GO);
            spi_write(ADRESL);
            PORTD = ~ADRESL;
        }
        spi_write(0xff);
        spi_write(0xff);
        spi_read();
        while ((readdata & 0b00011111) != 0x05) {
            spi_read();
        }
        while (readdata != 0xff) {
            spi_read();
        }
        if (RE0 == 1) {
            spi_write(0xff);
            spi_write(0xff);
            spi_write(0b11111101);    //stop token
            spi_read();
            spi_read();
            while (readdata != 0xff) {
                spi_read();
            }
            break;
        }
    }
}
 
void read()
{
    pwm_enable();
    command(18, (arg), 0xff);
    while (readdata != 0) {
        spi_read();
        string("READ ERROR");
    }
    string("READING MMC");
    while (1) {
        while (readdata != 0xfe) {
            spi_read();
        }
        for (int g = 0; g < 512; g++) {
            spi_read();
            __delay_us(16.5);
            CCPR2L = readdata;
            PORTD = ~readdata;
        }
        spi_write(0xff);
        spi_write(0xff);
        if (RE0 == 1) {
            command(12, arg, 0xff);
            spi_read();
            while (readdata != 0) {
                spi_read();
            }
            while (readdata != 0xff) {
                spi_read();
            }
            break;
        }
    }
}
 
/*--------------------mmc end----------------------*/
/*-----------------ADC functions-------------------*/
void adc_init()
{
    TRISA0 = 1;
    ADCON0 = 0b10000001;
    ADCON1 = 0b10001110;
}
 
/*-------------------main function-------------------*/
main()
{
    CS = 1;
    PORTD = 0;
    TRISC4 = 0;
    TRISC5 = 0;
    TRISD = 0;
    TRISB2 = 0;
    TRISB1 = 0;
    TRISE0 = 1;
    lcd_init();
    adc_init();
    usrt_init();
    spi_init();
    mmc_init();
    pwm_init();
    lcd_init();
    count = 0;
    CS = 0;
    arg = 0;
    while (1) {
        arg = 0;
        fst;
        pwm_enable();
        string("READ MODE");
        __delay_ms(1000);
        read();
        arg = 0;
        fst;
        pwm_disable();
        string("WRITE MODE");
        __delay_ms(1000);
        write();
    }
}
