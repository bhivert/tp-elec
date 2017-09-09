/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: noxx <noxx@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2017/09/09 11:14:30 by noxx              #+#    #+#             */
/*   Updated: 2017/09/09 11:16:22 by noxx             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <xc.h>

// DEVCFG3
// USERID = No Setting

// DEVCFG2
#pragma config FPLLIDIV = DIV_12        // PLL Input Divider (12x Divider)
#pragma config FPLLMUL = MUL_24         // PLL Multiplier (24x Multiplier)
#pragma config FPLLODIV = DIV_256       // System PLL Output Clock Divider (PLL Divide by 256)

// DEVCFG1
#pragma config FNOSC = FRCDIV           // Oscillator Selection Bits (Fast RC Osc w/Div-by-N (FRCDIV))
#pragma config FSOSCEN = ON             // Secondary Oscillator Enable (Enabled)
#pragma config IESO = ON                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = OFF            // Primary Oscillator Configuration (Primary osc disabled)
#pragma config OSCIOFNC = ON            // CLKO Output Signal Active on the OSCO Pin (Enabled)
#pragma config FPBDIV = DIV_8           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/8)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disable, FSCM Disabled)
#pragma config WDTPS = PS1048576        // Watchdog Timer Postscaler (1:1048576)
#pragma config FWDTEN = ON              // Watchdog Timer Enable (WDT Enabled)

// DEVCFG0
#pragma config DEBUG = OFF              // Background Debugger Enable (Debugger is disabled)
#pragma config ICESEL = ICS_PGx2        // ICE/ICD Comm Channel Select (ICE EMUC2/EMUD2 pins shared with PGC2/PGD2)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled

#define PWM_PERIODE             5
#define PWM_PERIODEfact1SEC     675

static char         state;
static char         tmp;
static int          timer1_count;
static int          timer2_count;
static int          pwm_count;
static int          pwm_periode_count;
static int          pwm_sec_count;
static int          pwm_delta;
static int          index;
static const int    limits[] = {32, 16, 8, 2, 1};

void init(void) {
    // Input/Output
    TRISFbits.TRISF1 ^= TRISFbits.TRISF1;
    TRISDbits.TRISD8 = 0x1;

    // TIMER 1 SETTING (~16Hz)
    T1CON ^= T1CON;
    PR1 = 0xFFFF;
    TMR1 = 49920;

    // TIMER 1 INTERRUPT SETTING
    IFS0bits.T1IF ^= IFS0bits.T1IF;
    IEC0bits.T1IE = 0x1;
    IPC1bits.T1IP = 0x3;

    // TIMER 2 SETTING (~16Hz)
    T2CON ^= T2CON;
    PR2 = 0xFFFF;
    TMR2 = 49920;

    // TIMER 2 INTERRUPT SETTING
    IFS0bits.T2IF ^= IFS0bits.T2IF;
    IEC0bits.T2IE = 0x1;
    IPC2bits.T2IP = 0x3;

    // TIMER 3 SETTING FOR PWN (~120kHz)
    T3CON ^= T3CON;
    PR3 = 0xFFFF;
    TMR3 = 65534;

    //TIMER 3 INTERRUPT SETTING
    IFS0bits.T3IF ^= IFS0bits.T3IF;
    IEC0bits.T3IE = 0x1;
    IPC3bits.T3IP = 0x3;

    // RD8/INT1 INTERRUPT SETTING
    IFS0bits.INT1IF ^= IFS0bits.INT1IF;
    IEC0bits.INT1IE = 0x1;
    IPC1bits.INT1IP = 0x3;

    // VARIABLE INIT
    state = 0x1;
    tmp = PORTDbits.RD8;
    timer1_count ^= timer2_count;
    timer2_count ^= timer2_count;
    index ^= index;

    // INTERRUPT GENERAL SETTING
    INTCON ^= INTCON;
    INTCONbits.MVEC = 0x1;
    __asm("ei");

    // TIMER 1 START
    T1CONbits.ON = 0x1;
}

void    __attribute__((vector(_EXTERNAL_1_VECTOR), interrupt(IPL3AUTO))) _EXTERNAL1_HANDLER(void) {
    IFS0bits.INT1IF ^= IFS0bits.INT1IF;
    timer2_count ^= timer2_count;
    T2CONbits.ON = 0x1;

    if (!state || state == 0x2) {
        state = 0x1;
        index = -1;
    }
    if (state == 0x1) {
        T1CONbits.ON = 0x1;
        index = (index + 1) % (sizeof(limits) / sizeof(int));
    }
}

void    __attribute__((vector(_TIMER_1_VECTOR), interrupt(IPL3AUTO))) _TIMER1_HANDLER(void) {
    IFS0bits.T1IF ^= IFS0bits.T1IF;
    TMR1 = 49920;
    ++timer1_count;

    if (state == 0x1) {
        if (timer1_count >= limits[index]) {
            timer1_count ^= timer1_count;
            LATFbits.LATF1 = ~LATFbits.LATF1;
        }
    } else {
        T1CONbits.ON ^= T1CONbits.ON;
        index ^= index;
    }
}

void    __attribute__((vector(_TIMER_2_VECTOR), interrupt(IPL3AUTO))) _TIMER2_HANDLER(void) {
    IFS0bits.T2IF ^= IFS0bits.T2IF;
    TMR2 = 49920;
    ++timer2_count;

    if (PORTDbits.RD8) {
        T2CONbits.ON ^= T2CONbits.ON;
    }
    if (timer2_count >= 64 && state != 0x2) {
        T1CONbits.ON ^= T1CONbits.ON;
        T2CONbits.ON ^= T2CONbits.ON;
        state = 0x2;
        pwm_count ^= pwm_count;
        pwm_periode_count ^= pwm_periode_count;
        pwm_sec_count ^= pwm_sec_count;
        pwm_delta = -1;
        T3CONbits.ON = 0x1;
    }
}

static inline int  pwm_getDelta(int periode_count, int sec, int sec_max) {
    return (((periode_count * PWM_PERIODE + (PWM_PERIODE * PWM_PERIODEfact1SEC * sec)) * 100 / (PWM_PERIODEfact1SEC * PWM_PERIODE * sec_max)) * PWM_PERIODE / 100);
}

void    __attribute__((vector(_TIMER_3_VECTOR), interrupt(IPL3AUTO))) _TIMER3_HANDLER(void) {
    IFS0bits.T3IF ^= IFS0bits.T3IF;
    TMR3 = 65534;
    ++pwm_count;

    if (state != 0x2) {
        T3CONbits.ON ^= T3CONbits.ON;
        return ;
    }
    if (pwm_count >= PWM_PERIODE) {
        pwm_count ^= pwm_count;
        ++pwm_periode_count;
        pwm_delta = -1;
        if (pwm_periode_count >= PWM_PERIODEfact1SEC) {
            pwm_periode_count ^= pwm_periode_count;
            ++pwm_sec_count;
        }
    }
    if (pwm_sec_count >= 10)
        pwm_sec_count ^= pwm_sec_count;
    if (pwm_delta == -1) {
        pwm_delta = pwm_getDelta(pwm_periode_count, \
                ((pwm_sec_count < 5) ? pwm_sec_count : 10 - pwm_sec_count), \
                5);
    }
    if (pwm_count < pwm_delta)
        LATFbits.LATF1 = 0x1;
    else
        LATFbits.LATF1 ^= LATFbits.LATF1;
}

int main() {
    init();
    while (1)
        WDTCONbits.WDTCLR = 0x1;
    return (0);
}
