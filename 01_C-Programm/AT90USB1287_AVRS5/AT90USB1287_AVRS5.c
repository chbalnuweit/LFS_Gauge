/*
 * AT90USB1287_AVRS5.c
 *
 * Created: 26.12.2011 20:55:11
 * Author: Christian Balnuweit
 * Project: 
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_serial.h"

// Makros und Funktionsdeklaration
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n)) // siehe CLKPR-Register Doku
void timer_init(void);
void ddr_init(void);
void split_data(char *data);
	
// Anzahl der Zeichen die der Wert in dem Protokoll einnimmt (NC = Number of Chars)
#define RPM_NC   3
#define SPEED_NC 3
#define FUEL_NC  3
#define TEMP_NC  3
#define DATA_NC  17 // 12 Stellen Motorsignale + 4 Dashlights + 1 Gear

// Globale Variablen
volatile uint16_t speed_val = 1;
volatile uint16_t rpm_val   = 1;
uint8_t gear_out[8] = {68, 100, 160, 213, 241, 226, 115, 119};

// Structs
struct dash_light {
	 int i;
	char c[2];
};
	
void ddr_init()
{
	DDRD = (1<<PD0) | (1<<PD1);	// Tank Ausgang (OC0B) und Temp Ausgang (OC2B)
	DDRF = 0xFF;				// gesamten Port auf Ausgang
	PORTF = (1<<PF0) | (1<<PF3); //PIN 1 und 3 auf High, damit Blinker aus.
	DDRB |= (1<<PB5);			// DZM Ausgang (OC1A)		
	DDRC |= (1<<PC6);			// Tacho Ausgang (OC3A)
	DDRA = 0xFF;				// 7-Segment-Ausgang
	PORTA = 100;				// Zeige Neutral bei init
	DDRE = (1<<PE4);			// Handbremse
}


void timer_init()
{
	
	// 16Bit-Timer1 für DZM  
	TCCR1A = (1<<COM1A0);							// Toggle Pin on Compare Match, siehe Datenblatt Tabelle 14-1
	TCCR1B = (1<<WGM12) | (1<<CS10) | (1<<CS11);	// Aktiviere CTC Mode non PWM, siehe Datenblatt Tabelle 14-4
	TIMSK1 = (1<<OCIE1A);							// Aktiviere Interrupt, siehe Datenblatt S.147
	OCR1A = 0x7D00;
	
	// 16Bit-Timer3 für Tacho (Initialisierung wie Timer1)
	TCCR3A = (1<<COM3A0);
	TCCR3B = (1<<WGM32) | (1<<CS30) | (1<<CS31);
	TIMSK3 = (1<<OCIE3A);	
	OCR3A = 0x7D01;
	
	// 8Bit-Timer2 für Temp
	TCCR2A = (0<<COM2B0) | (1<<COM2B1) | (0<<COM2A0) | 
			 (0<<COM2A1) | (1<<WGM20)  | (1<<WGM21);
	TCCR2B = (1<<WGM22)  | (1<<CS22)   | (0<<CS21) | (0<<CS20);
	OCR2A = 255;	// Wert für PWM-Grundfrequenz
	OCR2B = 250;	// Wert für Tastverhältnis bei Initialisierung
	
	// 8Bit-Timer0 für Tank
	TCCR0A = (0<<COM0B0) | (1<<COM0B1) | (0<<COM0A0) |
			 (0<<COM0A1) | (1<<WGM00)  | (1<<WGM01);
	TCCR0B = (0<<WGM02)  | (0<<CS02)   | (0<<CS01) | (1<<CS00);
	OCR0A = 160;
	OCR0B = 60;
	
	sei();											// Aktiviere Interrupts global
}

ISR (TIMER1_COMPA_vect)
{
	OCR1A = (16000000/(2*64*rpm_val))-1;	
}

ISR (TIMER3_COMPA_vect)
{
	OCR3A = (16000000/(2*64*speed_val))-1;
}



void split_data(char *data)
{
	// Variablen anlegen
	char speed_str[SPEED_NC+1] = {};
	char rpm_str[RPM_NC+1]	   = {};
	char temp_str[TEMP_NC+1]   = {};
	char fuel_str[FUEL_NC+1]   = {};
	
	struct dash_light handbrake;
	struct dash_light signal_r;
	struct dash_light signal_l;
	struct dash_light signal_abs;
	struct dash_light gear_in;

	int i = 0;
	int k = 0;

	// Drehzahl auslesen
	for(i=0; i<RPM_NC; i++) 
		{
			rpm_str[i]   = data[k];
			rpm_str[i+1] = '\0';
			k++;
		}
	rpm_val = atoi(rpm_str);

	// Geschwindigkeit auslesen	
	for(i=0; i<SPEED_NC; i++)
	{
		speed_str[i]   = data[k];
		speed_str[i+1] = '\0';
		k++;
	}
	speed_val = atoi(speed_str);


	// Temp auslesen
	for(i=0; i<TEMP_NC; i++)
	{
		temp_str[i]   = data[k];
		temp_str[i+1] = '\0';
		k++;		
	}
	//temp_val = atoi(temp_str);
	OCR2B = atoi(temp_str);

	// Fuel auslesen
	for(i=0; i<FUEL_NC; i++)
	{
		fuel_str[i]   = data[k];
		fuel_str[i+1] = '\0';
		k++;		
	}
	//fuel_val = atoi(fuel_str);
	OCR0B = atoi(fuel_str);

	// Dashlights auslesen
	handbrake.c[0] = data[k];
	handbrake.c[1] = '\0';
	handbrake.i = atoi(handbrake.c);
	k++;
	signal_r.c[0] = data[k];
	signal_r.c[1] = '\0';
	signal_r.i = atoi(signal_r.c);
	k++;
	signal_l.c[0] = data[k];
	signal_l.c[1] = '\0';
	signal_l.i = atoi(signal_l.c);
	k++;
	signal_abs.c[0] = data[k];
	signal_abs.c[1] = '\0';
	signal_abs.i = atoi(signal_abs.c);
	k++;
	gear_in.c[0] = data[k];
	gear_in.c[1] = '\0';
	gear_in.i = atoi(gear_in.c);
	
	// Ausgänge setzen
	PORTE = (handbrake.i<<PE4);
	PORTF =  (signal_r.i<<PF3) | (signal_l.i<<PF0) | (signal_abs.i<<PF1);
	PORTA = gear_out[gear_in.i];
}

// Main-Programm
int main(void)
{
	//Initialisierung
	CPU_PRESCALE(0);
	ddr_init();
	timer_init();
	usb_init();

	while(1)
	{	

		char data[DATA_NC+1] = {};				// Leeren Char-Array erzeugen
		while(!usb_serial_available()) {;}		// Tue nichts, solange keine Daten im Puffer
		cli();									// Deaktiviere Interrupts, damit Lesen.. der Daten nicht unterbrochen wird
		usb_serial_readline(data,DATA_NC);		// Daten aus Puffer auf Char-Array lesen
		data[DATA_NC] = '\0';					// Char-Array terminieren
		split_data(data);						// Daten aus Protkoll extrahieren
		usb_serial_flush_input();				// Puffer löschen
		sei();									// Interrupts wieder aktivieren
    }
}