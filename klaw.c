#include "klaw.h"
#include "frdm_bsp.h"

void Klaw_Init(void)
{
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;		// Wlaczenie portu B
	PORTB->PCR[S1] |= PORT_PCR_MUX(1);	
	PORTB->PCR[S1] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;		// Wlaczenie portu A
	//PORTA->PCR[S1] |= PORT_PCR_MUX(1);
	PORTA->PCR[S2] |= PORT_PCR_MUX(1);
	PORTA->PCR[S3] |= PORT_PCR_MUX(1);
	PORTA->PCR[S4] |= PORT_PCR_MUX(1);
	//PORTA->PCR[S1] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[S2] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[S3] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[S4] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
}
void Klaw_S2_4_Int(void)
{
	PORTB -> PCR[S1] |= PORT_PCR_IRQC(0xa);		//0x8 - poziom "0"; 0x9 - zbocze narastające; 0xa - zbocze opadające; 0xb - obydwa zbocza
	PORTA -> PCR[S2] |= PORT_PCR_IRQC(0xa);		
	PORTA -> PCR[S3] |= PORT_PCR_IRQC(0xa);		
	PORTA -> PCR[S4] |= PORT_PCR_IRQC(0xa);
	
	NVIC_SetPriority(PORTA_IRQn, 3);  
	NVIC_ClearPendingIRQ(PORTA_IRQn);
	NVIC_EnableIRQ(PORTA_IRQn);

	NVIC_SetPriority(PORTB_IRQn, 2);  
	NVIC_ClearPendingIRQ(PORTB_IRQn);
	NVIC_EnableIRQ(PORTB_IRQn);
}
