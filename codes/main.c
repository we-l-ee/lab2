// LaunchPad built-in hardware
// SW1 left switch is negative logic PF4 on the Launchpad
// SW2 right switch is negative logic PF0 on the Launchpad
// red LED connected to PF1 on the Launchpad
// blue LED connected to PF2 on the Launchpad
// green LED connected to PF3 on the Launchpad


#include "tm4c123gh6pm.h"
#define PF4                     (*((volatile unsigned long *)0x40025040))
#define PF3                     (*((volatile unsigned long *)0x40025020))
#define PF2                     (*((volatile unsigned long *)0x40025010))
#define PF1                     (*((volatile unsigned long *)0x40025008))
#define PF0                     (*((volatile unsigned long *)0x40025004))

void PortF_Init(void){ volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x00000020;     // 1) activate clock for Port F
  delay = SYSCTL_RCGC2_R;           // allow time for clock to start
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
  // only PF0 needs to be unlocked, other bits can't be locked
  GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog on PF
  GPIO_PORTF_PCTL_R = 0x00000000;   // 4) PCTL GPIO on PF4-0
  GPIO_PORTF_DIR_R = 0x0E;          // 5) PF4,PF0 in, PF3-1 out
  GPIO_PORTF_AFSEL_R = 0x00;        // 6) disable alt funct on PF7-0
  GPIO_PORTF_PUR_R = 0x11;          // enable pull-DOWN on PF0 and PF4
  GPIO_PORTF_DEN_R = 0x1F;          // 7) enable digital I/O on PF4-0
}

#define DOWN 1
#define UP 0
#define ACTIVATE 1
#define DISABLE 0

int volatile force_sos = 0;
unsigned long push_down_count = 0;
int prev_pf0 = 0x01;
int button_handler()
{
	if (prev_pf0 == 0)
	{
		if (PF0 == 0)
		{
				push_down_count++;
				return DOWN;
		}
		else
		{
			prev_pf0 = 0x01;
			return UP;
		}
	}
	else
	{
		if (PF0 == 0)
		{
				push_down_count = 1;
				prev_pf0 = 0;
				return DOWN;
		}
		else
		{
			return UP;
		}
	}
}
/*
*	Senses button, activates response.
*/
inline int delay(float d, int sense_active, int response){
	unsigned long volatile time;
	if (sense_active)
			time = 1454480/5*d;  // 1 sec * delay
	else
		time = 1454480*d;  // 1 sec * delay

  while(time){
		time--;
		//force_sos = PF4 == 0;
		if (sense_active)
		{
			force_sos = force_sos || PF4 == 0;
			int res = button_handler();
			if(response && (force_sos || res))
				return 1;
		}
  }
	return 0;
}

void dot(int sense)
{
	GPIO_PORTF_DATA_R |= 0x02;  // flash LED as WHITE
	delay(0.25f, DISABLE, DISABLE);
	GPIO_PORTF_DATA_R &= ~0x0E;  // clear
	delay(0.25f, sense, DISABLE);
}

void dash(int sense)
{
	GPIO_PORTF_DATA_R |= 0x0E;  // flash LED as RED
	delay(0.5f, DISABLE, DISABLE);
	GPIO_PORTF_DATA_R &= ~0x0E;  // clear
	delay(0.5f, sense, DISABLE);
}

void flash_s()
{

}
void sos()
{
	dot(DISABLE); dot(DISABLE); dot(DISABLE);	
	dash(DISABLE); dash(DISABLE); dash(DISABLE);
	dot(DISABLE); dot(DISABLE); dot(DISABLE);
}
int clear_return()
{
	GPIO_PORTF_DATA_R &= ~0x0E;  // clear
	return 1;
}

int idle()
{
	GPIO_PORTF_DATA_R &= ~0x0E;  // clear
	GPIO_PORTF_DATA_R |= 0x02;  // flash LED as red
	if (delay(0.5f, 1, 1))
		return clear_return();
	GPIO_PORTF_DATA_R &= ~0x0E;  // clear
	GPIO_PORTF_DATA_R |= 0x04;  // flash LED as blue
	if(delay(1.0f, 1, 1))
		return clear_return();
	GPIO_PORTF_DATA_R &= ~0x0E;  // clear
	GPIO_PORTF_DATA_R |= 0x08;  // flash LED as green
	if(delay(1.5f, 1, 1))
		return clear_return();
	GPIO_PORTF_DATA_R &= ~0x0E;  // clear
	
	return 0;
}

int main(void){  
  PortF_Init();  // make PF1 out (PF1 built-in LED)
  while(1){
		if (PF4==0 || force_sos)
		{
			force_sos = 0;
			sos();
			delay(1.0f, ACTIVATE, DISABLE);
			push_down_count = 0;
		}
		else if(button_handler())
		{
			if(push_down_count > 145448*0.4)
			{
				push_down_count = 0;
				dash(ACTIVATE);
				if(push_down_count == 0)
					delay(2.0f, ACTIVATE, ACTIVATE);
			}
		}
		else
		{
			if (push_down_count > 0)
			{
				if(push_down_count < 145448*0.4)
				{
					push_down_count = 0;
					dot(ACTIVATE);
				}
				else
				{
					push_down_count = 0;
					dash(ACTIVATE);
				}
				if(push_down_count == 0) 
					delay(2.0f, ACTIVATE, ACTIVATE);
			}
			else
			{
				idle();
			}
		}
		
  }
}

// Color    LED(s) PortF
// dark     ---    0
// red      R--    0x02
// blue     --B    0x04
// green    -G-    0x08
// yellow   RG-    0x0A
// sky blue -GB    0x0C
// white    RGB    0x0E
// pink     R-B    0x06
