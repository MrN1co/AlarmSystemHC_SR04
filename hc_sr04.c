#include "hc_sr04.h"
#include "frdm_bsp.h"

void HC_SR04_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK; // Włączenie portu B
    PORTB->PCR[TRIG_PIN] = PORT_PCR_MUX(1); // Ustawienie trybu GPIO dla TRIG
    PORTB->PCR[ECHO_PIN] = PORT_PCR_MUX(1); // Ustawienie trybu GPIO dla ECHO
    

    // Ustawienie TRIG jako wyjście
    PTB->PDDR |= (TRIG_MASK);
    // Ustawienie ECHO jako wejście
    PTB->PDDR &= ~(ECHO_MASK);
}

void HC_SR04_Start(void) {
    // Ustawienie TRIG na niski stan
    PTB->PCOR = TRIG_MASK;

    // Opóźnienie 2 mikrosekundy
    DELAY(0.02); 
    
    // Ustawienie TRIG na wysoki stan
    PTB->PSOR = TRIG_MASK;
    // Opóźnienie 10 mikrosekund
    DELAY(0.1);
    
    // Ustawienie TRIG z powrotem na niski stan
    PTB->PCOR = TRIG_MASK;
}

float HC_SR04_GetDistance(void) {
    uint32_t startTime = 0;
    uint32_t endTime = 0;

    // Rozpoczęcie pomiaru
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk; // Wyłączenie przerwań SysTick
    SysTick_Config(SystemCoreClock/10); // Konfiguracja SysTick

    HC_SR04_Start();
    

    // Czekanie na wysoki stan ECHO
    while (!(PTB->PDIR & ECHO_MASK));

    // Zapisanie czasu rozpoczęcia
    startTime = SysTick->VAL;

    // Czekanie na niski stan ECHO
    while (PTB->PDIR & ECHO_MASK);

    // Zapisanie czasu zakończenia
    endTime = SysTick->VAL;
    
    

    // Obliczenie czasu trwania impulsu
    int32_t pulseDuration = startTime - endTime;

	
    //SysTick_Config(1); // Resetowanie SysTick do milisekund
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk; // Włączenie przerwań SysTick

    // Obliczenie odległości w centymetrach (prędkość dźwięku ~ 343 m/s)
    return (pulseDuration * 0.0343) / 2; // Wzór: distance = (time * speed) / 2
}
