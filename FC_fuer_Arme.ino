volatile unsigned int PASZaehler = 0;         // wird von Timer0 inkrementiert
volatile unsigned int PAS = 64000;            // Timertics zwischen zwei PAS-interrupts
volatile unsigned int PASTimeOut = 32000;     // Timertics bis zum Abschalten des Ausgang
volatile unsigned int SlowLoopZaehler = 0;    // wird von Timer0 inkrementiert
volatile unsigned int Torque_kumuliert = 0;   // aufsummiertes Drehmomentsignal
volatile unsigned int Torque_min = 90;        // ADC-Wert bei Drehmoment = 0 (0,5V -> muss an tatsächlichen Wert angepasst werden)
volatile unsigned int Torque_max = 860;       // ADC-Wert bei maximalen Drehmoment  (4,2V -> muss an tatsächlichen Wert angepasst werden)
volatile unsigned long Spannung_Ausgang = 0;  // Wert PWM-Ausgang 0 ... 255

boolean PASFlag=0;

void setup() {
  pinMode(1, OUTPUT);           // Ausgang für Gassignal
  pinMode(2, INPUT_PULLUP);     // Input PAS, external interrupt INT0
  pinMode(3, INPUT);            // Eingang Poti für Unterstützungsstärke
  pinMode(4, INPUT);            // Eingang Drehmomentsignal

  // Timer0 on PB0 and PB1
  // Fast PWM, TOP=0xFF (WGM01, WGM00)

  TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
  TCCR0B = _BV(CS00);          //Prescaler 256 @ 16MHz -> 62,5 kHz PWM Frequenz
  OCR0B = 0;                   //Ausgang auf 0 setzen (LED aus)

  TIMSK |= (1 << TOIE0); //enable Timer0 overflow interrupt
  sei();
  attachInterrupt(0, PAS_Puls, CHANGE); //INT0 ist Interruptquelle 2. Change: Interrupt bei jedem Flankenwechsel also 32 mal pro Kurbelumdrehung
}



void loop() {

  if(PASFlag){              //Bei jedem PAS-Puls Drehmomentsignal einlesen und über eine Kurbeldrehung mitteln
    PASFlag=0;
    Torque_kumuliert -= Torque_kumuliert>>5;                                        //kumulierten Wert um 1/32stel reduzieren
    Torque_kumuliert += map(analogRead(2), Torque_min, Torque_max, 0, 1024);        //aktuellen Meßwert auf 10bit-Bereich skaliert auf kumulierten Wert addieren. Ergibt Mittelung über eine Kurbeldrehung
  }

  if(SlowLoopZaehler>640){  // Ausgang mit 100Hz aktualisieren
    SlowLoopZaehler=0;
    Spannung_Ausgang = (analogRead(3) * Torque_kumuliert) / PAS;                    //eigentliche Berechnung der Ausgangsspannung, hier muß wahrscheinlich noch skaliert werden um sinnvollen Wertebereich zu bekommen
    if (Spannung_Ausgang>255) Spannung_Ausgang = 255;                               //PWM Ausgang hat nur 8Bit Breite, daher Wert auf max. 255 beschränken
    if (PASZaehler>PASTimeOut) {                                                    
      Spannung_Ausgang = 0;                                                         //Wenn sich Pedale nicht bewegen, Ausgang auf Null setzen
      if (Torque_kumuliert>0)Torque_kumuliert--;                                    //Torque_kumuliert langsam reduzieren, damit nach längerem Stand wieder bei Null gestartet wird
    }
    
   OCR0B = Spannung_Ausgang;                                                       //PWM Ausgang auf Pin1 (PB1) aktualisieren 
         
  }

}




ISR(TIMER0_OVF_vect)    //Interruptroutine Timer0
{
  if(PASZaehler<64000)PASZaehler++;
  SlowLoopZaehler++;
}

void PAS_Puls(){        //Interruptroutine IN0 (PAS)
  PAS = PASZaehler;
  PASZaehler = 0;
  PASFlag = 1;
}
