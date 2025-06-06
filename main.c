
#include "MKL05Z4.h"
#include "frdm_bsp.h"
#include "lcd1602.h"
#include "leds.h"
#include "DAC.h"
#include "klaw.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define	ZYXDR_Mask	1<<3	// Maska bitu ZYXDR w rejestrze STATUS
#define DIV_CORE	8192	// Przerwanie co 0.120ms - fclk=8192Hz df=8Hz
#define MASKA_10BIT	0x03FF

static uint8_t arrayXYZ[6];
static uint8_t AccSens; 
static uint8_t status;
double X, Y, Z;

volatile uint32_t tickCounter=0;
volatile uint8_t timerCall=0;		
volatile uint8_t isAlarmOn=0;	

volatile uint8_t BtnFlags[4] = {0, 0, 0, 0}; // Tablica do przechowywania stanu przycisków

volatile int16_t temp;
volatile uint16_t slider = 50;
volatile uint16_t dac;
volatile int16_t Sinus[1024];
volatile uint16_t faza, mod, freq, df;
volatile uint16_t nyquist;
volatile uint8_t toggleFreq=0;
volatile float wynik, volt_coeff = 0.01455;


volatile uint8_t correctPassword[4] = {1,1,1,1};
volatile uint8_t enteredPassword[4];
volatile uint8_t passwordIndex = 0;
volatile uint8_t incorectAttempts = 0;

volatile uint8_t isOptionSelected = 0;

enum State{
	STANDBY,
	ALARM,
	ADMIN
};
volatile enum State currentState = STANDBY;

enum MenuScreen{
	MENU_MAIN,
	MENU_PASSWORD,
	MENU_VOLUME,
	MENU_SENSITIVITY
};
volatile enum MenuScreen currentMenu = MENU_MAIN;





void Debug(void)
{
	LCD1602_ClearAll();
	LCD1602_Print("Debug");
}



void SysTick_Handler(void)	
{ 
	dac=(Sinus[faza]/100)*slider+0x0800;					// Przebieg sinusoidalny
	DAC_Load_Trig(dac);
	faza+=mod;		// faza - generator cyfrowej fazy
	faza&=MASKA_10BIT;	// rejestr sterujący przetwornikiem, liczący modulo 1024 (N=10 bitów)

	tickCounter+=1;	
	if(tickCounter==3277)	// 0.120ms*3277=393ms
	{
		tickCounter=0;
		timerCall=1;
	}
}

void PORTB_IRQHandler(void)	
{
	uint32_t buf;
	buf=PORTB->ISFR & S1_MASK;

	if(buf == S1_MASK)	// Sprawdzenie, czy naciśnięto przycisk S1
	{
		DELAY(100)
		if(!(PTB->PDIR&S1_MASK))		// Minimalizacja drgań zestyków
		{
			DELAY(100)
			if(!(PTB->PDIR&S1_MASK))	// Minimalizacja drgań zestyków (c.d.)
			{
				if(!BtnFlags[0])
				{
					BtnFlags[0]=1;
				}
			}
		}
	}
	PORTB->ISFR |=  S1_MASK;	// Kasowanie wszystkich bitów ISF
	NVIC_ClearPendingIRQ(PORTB_IRQn);
}

void PORTA_IRQHandler(void)	
{
	uint32_t buf;
	buf=PORTA->ISFR & (/*S1_MASK | */S2_MASK | S3_MASK | S4_MASK);

	switch(buf)
	{
		// case S1_MASK:	DELAY(100)
		// 							if(!(PTA->PDIR&S1_MASK))		// Minimalizacja drgań zestyków
		// 							{
		// 								DELAY(100)
		// 								if(!(PTA->PDIR&S1_MASK))	// Minimalizacja drgań zestyków (c.d.)
		// 								{
		// 									if(!BtnFlags[0])
		// 									{
		// 										BtnFlags[0]=1;
		// 									}
		// 								}
		// 							}
		// 							break;
		case S2_MASK:	DELAY(100)
									if(!(PTA->PDIR&S2_MASK))		// Minimalizacja drgań zestyków
									{
										DELAY(100)
										if(!(PTA->PDIR&S2_MASK))	// Minimalizacja drgań zestyków (c.d.)
										{
											if(!BtnFlags[1])
											{
												BtnFlags[1]=1;
											}
										}
									}
									break;
		case S3_MASK:	DELAY(100)
									if(!(PTA->PDIR&S3_MASK))		// Minimalizacja drgań zestyków
									{
										DELAY(100)
										if(!(PTA->PDIR&S3_MASK))	// Minimalizacja drgań zestyków (c.d.)
										{
											if(!BtnFlags[2])
											{
												BtnFlags[2]=1;
											}
										}
									}
									break;
		case S4_MASK:	DELAY(100)
									if(!(PTA->PDIR&S4_MASK))		// Minimalizacja drgań zestyków
									{
										DELAY(100)
										if(!(PTA->PDIR&S4_MASK))	// Minimalizacja drgań zestyków (c.d.)
										{
											if(!BtnFlags[3])
											{
												BtnFlags[3]=1;
											}
										}
									}
									break;
		default:			break;
	}	
	PORTA->ISFR |=  S1_MASK | S2_MASK | S3_MASK | S4_MASK;	// Kasowanie wszystkich bitów ISF
	NVIC_ClearPendingIRQ(PORTA_IRQn);
}


void Speaker_Init()
{
	wynik = (float)50*volt_coeff;
	df=DIV_CORE/MASKA_10BIT;	// Rozdzielczość generatora delta fs
	nyquist=DIV_CORE/(3*df);
	DAC_Init();		// Inicjalizacja prztwornika C/A
	for(faza=0;faza<1024;faza++)
		Sinus[faza]=(sin((double)faza*0.0061359231515)*2047.0); // Ładowanie 1024-ech sztuk, 12-bitowych próbek funkcji sisnus do tablicy
	faza=0;		// Ustawienie wartości początkowych generatora cyfrowej fazy i modulatora fazy
	mod=64;
	freq=mod*df;
	NVIC_SetPriority(SysTick_IRQn, 0);
}

uint8_t CheckPassword(uint8_t *password)
{
	return (memcmp(password, correctPassword, 4) == 0) ? 1 : 0;
}

void ClearPassword()
{
	for (uint8_t i = 0; i < sizeof(enteredPassword)/sizeof(enteredPassword[0]); i++)
	{
		enteredPassword[i] = 0; 
	}
	passwordIndex = 0;
}

void ChangePassword(uint8_t *password)
{
	for (uint8_t i = 0; i < sizeof(correctPassword)/sizeof(correctPassword[0]); i++)
	{
		correctPassword[i] = password[i]; 
	}
}


void StartAlarm(void)
{
	SysTick_Config(SystemCoreClock/DIV_CORE);
	PTB->PTOR|=LED_R_MASK;


	LCD1602_ClearAll();
	LCD1602_Print("    ALARM!!!");
	currentState = ALARM;
}

void StopAlarm(void)
{
	SysTick_Config(1);
	PTB->PSOR|=LED_R_MASK;
	PTB->PSOR|=LED_B_MASK;
	if (currentState == 1)
	{
		currentState = STANDBY;
	}
	
}

void ArmAlarm(void)
{
	LCD1602_ClearAll();
	LCD1602_Print("Enter password:");
	LCD1602_SetCursor(0,1);
	ClearPassword();

	currentState = STANDBY;
}

void CheckAccel()
{
	I2C_ReadReg(0x1d, 0x0, &status);
	status&=ZYXDR_Mask;
	if(status)	// Czy dane gotowe do odczytu?
	{
		I2C_ReadRegBlock(0x1d, 0x1, 6, arrayXYZ);
		X=((double)((int16_t)((arrayXYZ[0]<<8)|arrayXYZ[1])>>2)/(4096>>AccSens));
		Y=((double)((int16_t)((arrayXYZ[2]<<8)|arrayXYZ[3])>>2)/(4096>>AccSens));
		Z=((double)((int16_t)((arrayXYZ[4]<<8)|arrayXYZ[5])>>2)/(4096>>AccSens));

		if(fabs(X)+fabs(Y)+fabs(Z)>2.0)
		{
			if(currentState == 0)
			{
				StartAlarm();
			}
		}
	}
}




void MainMenu(void)
{
	currentMenu = MENU_MAIN;

	LCD1602_ClearAll();
	LCD1602_Print("   Admin Mode");
	LCD1602_SetCursor(0,1);
	LCD1602_Print("1 ^ 2 v 3 < 4 >");
}

void VolumeMenu(void)
{
	currentMenu = MENU_VOLUME;

	LCD1602_ClearAll();
	LCD1602_Print("    Volume");
	LCD1602_SetCursor(0,1);
	LCD1602_Print("1 ^ 2 v 3 < 4 >");
}

void SensitivityMenu(void)
{
	currentMenu = MENU_SENSITIVITY;

	LCD1602_ClearAll();
	LCD1602_Print("  Sensitivity");
	LCD1602_SetCursor(0,1);
	LCD1602_Print("1 ^ 2 v 3 < 4 >");
}

void PasswordMenu(void)
{
	currentMenu = MENU_PASSWORD;

	LCD1602_ClearAll();
	LCD1602_Print("Change Password");
	LCD1602_SetCursor(0,1);
	LCD1602_Print("1 ^ 2 v 3 < 4 >");
}


void Back(void)
{
	if (isOptionSelected)
	{
		isOptionSelected = 0;
	}
	
	if (currentMenu != 0)
	{
		MainMenu();
	}

}


void Forward(void)
{
	if (currentMenu == 0)
	{
		return;
	}

	isOptionSelected = 1;
}

void Up(void)
{
	switch (currentMenu)
	{
	case MENU_MAIN:
		SensitivityMenu();
		break;
	case MENU_PASSWORD:
		MainMenu();
		break;
	case MENU_VOLUME:
		PasswordMenu();
		break;
	case MENU_SENSITIVITY:
		VolumeMenu();
		break;
	default:
		break;
	}
}

void Down(void)
{
	switch (currentMenu)
	{
		case MENU_MAIN:
			PasswordMenu();
			break;
		case MENU_PASSWORD:
			VolumeMenu();
			break;
		case MENU_VOLUME:
			SensitivityMenu();
			break;
		case MENU_SENSITIVITY:
			MainMenu();
			break;
		default:
			break;
	}
}



void EnterAdminMode()
{
	incorectAttempts = 0;
	currentState = ADMIN;
	StopAlarm();
	MainMenu();
}




void CheckButtons(void)
{
	if (currentState != 2) //If not in admin mode
	{
		for (uint8_t i = 0; i < sizeof(BtnFlags)/sizeof(BtnFlags[0]); i++)
		{
			
			if (BtnFlags[i])
			{
				BtnFlags[i] = 0;

				enteredPassword[passwordIndex] = i + 1; // Store the button number (1-4)
				passwordIndex++;

				LCD1602_SetCursor(passwordIndex+5, 1);
				LCD1602_Print("*");
			}

			if (passwordIndex >= 4) // If 4 digits entered
			{
				if (CheckPassword(enteredPassword))
				{
					EnterAdminMode();
				}
				else
				{
					incorectAttempts++;
					if (incorectAttempts >= 3)
					{
						incorectAttempts = 0;
						StartAlarm();
					}
					else
					{
						LCD1602_ClearAll();
						LCD1602_Print("   Incorrect!");
					}
				}
				ClearPassword(); // Reset for next input
			}
			
		}
		
		return;
	}

	if (currentState == 2)  //If in admin mode
	{
		if (BtnFlags[0])
		{
			BtnFlags[0] = 0;
			Up(); 
		}
		if (BtnFlags[1])
		{
			BtnFlags[1] = 0;
			Down();
			
		}
		if (BtnFlags[2])
		{
			BtnFlags[2] = 0;
			Back();
		}
		if (BtnFlags[3])
		{
			BtnFlags[3] = 0;
			Forward();
		}
		return;
	}
}




void Alarm(void)
{
	PTB->PTOR|=LED_R_MASK;	
	PTB->PTOR|=LED_B_MASK;
	
	if (toggleFreq)
	{
		temp=mod;
		temp+=4;
		if(temp>nyquist)
			temp=nyquist;
		mod=temp;
		freq=mod*df;

		toggleFreq=0;
	}
	else
	{
		temp=mod;
		temp-=4;
		if(temp<1)
			temp=1;
		mod=temp;
		freq=mod*df;

		toggleFreq=1;
	}
}





int main (void) 
{
	char display[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};

	
	Klaw_Init();
	Klaw_S2_4_Int();
	LED_Init();	
	LCD1602_Init();	
	LCD1602_Backlight(TRUE); 


	AccSens=0;	// Wybór czułości: 0 - 2g; 1 - 4g; 2 - 8g
	I2C_WriteReg(0x1d, 0x2a, 0x0);
	I2C_WriteReg(0x1d, 0xe, AccSens);	
	I2C_WriteReg(0x1d, 0x2a, 0x29);	

	Speaker_Init();	// Inicjalizacja głośnika

	StopAlarm();
	

	ArmAlarm();	


	while(1)		
	{
		if(currentState == 0)
		{
			CheckAccel();
		}


		if(timerCall && currentState == 1)
		{
			Alarm();
			timerCall=0;
		}

		CheckButtons();

	}	
}
