/* ******************************* CECS 347 *******************************
 *  Name   :    Chanartip Soonthornwan (014353883)
 *         :    Samuel Poff (...)
 *  Email  :    Chanartip.Soonthornwan@gmail.com
 *         :    Spoff42@gmail.com
 *  File   :    TableTrafficLight.c
 *  Purpose:    Demonstrating Traffic light by using PLL for timing
 *              and intterupt
 *              etc. (describing how the leds and switches work).
 *  Date   :    September 14, 2017
 *  Version:    1.1      
 *              Revision Date:     September 12, 2017
 *              Note: using PLL instead of systick timer            
 *
 * ************************************************************************/
 
// east facing red light connected to PB5
// east facing yellow light connected to PB4
// east facing green light connected to PB3
// north facing red light connected to PB2
// north facing yellow light connected to PB1
// north facing green light connected to PB0
// north facing car detector connected to PE1 (1=car present)
// east facing car detector connected to PE0 (1=car present)

#include "PLL.h" 
#include "SysTick.h"

//____________________________GPIO PORT ADDRESS___________________________//

// PORT B for Traffic lights
#define LIGHT                   (*((volatile unsigned long *)0x400053FC)) // PortB Data[5:0]
#define GPIO_PORTB_DIR_R        (*((volatile unsigned long *)0x40005400))
#define GPIO_PORTB_AFSEL_R      (*((volatile unsigned long *)0x40005420))
#define GPIO_PORTB_DEN_R        (*((volatile unsigned long *)0x4000551C))
#define GPIO_PORTB_AMSEL_R      (*((volatile unsigned long *)0x40005528))
#define GPIO_PORTB_PCTL_R       (*((volatile unsigned long *)0x4000552C))
#define SYSCTL_RCGC2_GPIOB      0x02  // port B Clock Gating Control

// PORT E for Three switches
#define GPIO_PORTE_DIR_R        (*((volatile unsigned long *)0x40024400))
#define GPIO_PORTE_AFSEL_R      (*((volatile unsigned long *)0x40024420))
#define GPIO_PORTE_PUR_R        (*((volatile unsigned long *)0x40024510))
#define GPIO_PORTE_PDR_R        (*((volatile unsigned long *)0x40024514))
#define GPIO_PORTE_DEN_R        (*((volatile unsigned long *)0x4002451C))
#define GPIO_PORTE_AMSEL_R      (*((volatile unsigned long *)0x40024528))
#define GPIO_PORTE_PCTL_R       (*((volatile unsigned long *)0x4002452C))
#define SYSCTL_RCGC2_GPIOE      0x10  // port E Clock Gating Control
// PORT E Interrupts
#define GPIO_PORTE_IS_R         (*((volatile unsigned long *)0x40024404))
#define GPIO_PORTE_IBE_R        (*((volatile unsigned long *)0x40024408))
#define GPIO_PORTE_IEV_R        (*((volatile unsigned long *)0x4002440C))
#define GPIO_PORTE_IM_R         (*((volatile unsigned long *)0x40024410))
#define GPIO_PORTE_RIS_R        (*((volatile unsigned long *)0x40024414))
#define GPIO_PORTE_ICR_R        (*((volatile unsigned long *)0x4002441C))
// PORT E Interrupt NVIC
#define NVIC_PRI1_R             (*((volatile unsigned long *)0xE000E404))
#define NVIC_EN0_R              (*((volatile unsigned long *)0xE000E100))

// PORT F for walk/don't walk lights
#define P_Led                   (*((volatile unsigned long *)0x40025028)) // PortF Data3, 1
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_LOCK_R       (*((volatile unsigned long *)0x40025520))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))
#define SYSCTL_RCGC2_GPIOF      0x20  // port F Clock Gating Control

// SYSTICK CONTROL REGISTER
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))

//_________________________ END GPIO PORT ADDRESS__________________________//

// Linked data structure
struct State {
  unsigned long Out;       // Traffic lights output
  unsigned long P_Out;     // Pedestrian lights output
  unsigned long Time;      // Time to wait before switch to the next state
  unsigned long Next[8];   // There are 8 next states + 1 init state = 9 states
}; 

typedef const struct State STyp;

// State instantiations  
#define goN       0
#define waitN     1
#define goE       2
#define waitE     3
#define goP       4   
#define waitPOn1  5
#define waitPOff1 6
#define waitPOn2  7
#define waitPOff2 8
	
STyp FSM[9]={ 
  /*
   *      out = 0x00_RY G_RYG
   *   pedout = 0x0000_G_0_R_0
   *     time in millisecond
   */
//out, pedout, time,next state NEP (North/ East/ Pedrestrian)
//                  000        001        010        011        100        101        110        111
 {0x21, 0x02, 3000,{goN,       waitN,     waitN,     waitN,     goN,       waitN,     goN,       waitN    }}, //gon
 {0x22, 0x02, 500 ,{waitN,     goP,       goE,       goP,       waitN,     goP  ,     waitN,     goP      }}, //waitn
 {0x0C, 0x02, 3000,{goE,       waitE,     goE,       waitE,     waitE,     waitE,     goE,       waitE    }}, //goe
 {0x14, 0x02, 500 ,{waitE,     goP,       waitE,     goP,       goN,       goP,       waitE,     goP      }}, //waite
 {0x24, 0x08, 2500,{goP,       goP,       waitPOn1,  goP,       waitPOn1,  goP,       waitPOn1,  goP      }}, //goP
 {0x24, 0x02, 500 ,{waitPOff1, waitPOff1, waitPOff1, waitPOff1, waitPOff1, waitPOff1, waitPOff1, waitPOff1}}, //waitPon1
 {0x24, 0x00, 500 ,{waitPOn2,  waitPOn2,  waitPOn2,  waitPOn2,  waitPOn2,  waitPOn2,  waitPOn2,  waitPOn2 }}, //watipoff1
 {0x24, 0x02, 500 ,{waitPOff2, waitPOff2, waitPOff2, waitPOff2, waitPOff2, waitPOff2, waitPOff2, waitPOff2}}, //waitpon2
 {0x24, 0x00, 500 ,{waitPOff2, waitPOff2, goE,       waitPOff2, goN,       waitPOff2, goN,       waitPOff2}}  //waitpoff2
};   


unsigned long S;        // index to the current state 
unsigned char Input; 
unsigned long Count;

void PortB_Init(void);
void PortE_Init(void);
void PortF_Init(void);
void updateInput(void);
void EnableInterrupts(void);

void GPIOPortE_Handler(void){
    Input |= GPIO_PORTE_RIS_R;
    GPIO_PORTE_ICR_R = 0x07;
}

// Interrupt service routine
// Executed every 25ns*(period)
void SysTick_Handler(void){
    Count = Count +1;
}


int main(void){ 
  
  PortB_Init();
  PortE_Init();
  PortF_Init();
  PLL_Init();       // 40 MHz
  SysTick_Init(40000);
  EnableInterrupts();
  S = goN;  //Initialize to go-north
    
  while(1){
    // Setup LEDs
    LIGHT = FSM[S].Out;  
    P_Led = FSM[S].P_Out;
    
    // Since Count is incremented every 250 ns (40 MHz)
    // and 1ms/250ns = 4000 times for each 1 ms, 
    // this loop will count up until FSM[S].Time (in milliseconds).
    while(Count < FSM[S].Time);
    Count = 0;  // Then reset the counter.
      
    // Update Next State
    S = FSM[S].Next[Input];  
    updateInput();  
     
  }
}

void updateInput(void){
    if      ( S == goP ) 
			Input &= ~0x01;
    else if ( S == goE )
        Input &= ~0x02;
    else if ( S == goN ) 
        Input &= ~0x04;
    else if ( S == waitPOff2 ) 
        Input &= ~0x01;
}

// PORTB for Traffic Lights
void PortB_Init(void){ volatile unsigned long delay;
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB; // 1) B clock
  delay = SYSCTL_RCGC2_R;               // 2) delay
  GPIO_PORTB_AMSEL_R &= ~0x3F;          // 3) disable analog function on PB5-0
  GPIO_PORTB_PCTL_R  &= ~0x00FFFFFF;    // 4) clear to enable regular PORT
  GPIO_PORTB_DIR_R   |=  0x3F;          // 5) set 1 for outputs on PB5-0
  GPIO_PORTB_AFSEL_R &= ~0x3F;          // 6) regular function on PB5-0
  GPIO_PORTB_DEN_R   |=  0x3F;          // 7) enable digital on PB5-0
}

// PORTE for buttons
void PortE_Init(void){ volatile unsigned long delay;
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOE; // 1) E clock
  delay = SYSCTL_RCGC2_R;               // 2) delay
  GPIO_PORTE_AMSEL_R &= ~0x07;          // 3) disable analog function on PE2-0
  GPIO_PORTE_PCTL_R  &= ~0x00000FFF;    // 4) clear to enable regular PORT
  GPIO_PORTE_DIR_R   &= ~0x07;          // 5) set 0 for inputs on PE2-0
  GPIO_PORTE_AFSEL_R &= ~0x07;          // 6) regular function on PE2-0
  GPIO_PORTE_PDR_R   |=  0x07;          // enable pullup resistor on PE2-0  
  GPIO_PORTE_DEN_R   |=  0x07;          // 7) enable digital on PE2-0
  // Interrupts
  GPIO_PORTE_IS_R    &= ~0x07;          // Clear for Edge Sensitive
  GPIO_PORTE_IBE_R   &= ~0x07;          // Clear for not Both Edge
  GPIO_PORTE_IEV_R   |=  0x07;          // Clear for Falling Edge
  GPIO_PORTE_ICR_R    =  0x07;          // Clear flags
  GPIO_PORTE_IM_R    |=  0x07;          // ARM interrupt
  // Set Interrupt Priority
  NVIC_PRI1_R   =   (NVIC_PRI1_R & 0xFFFFFF1F) | 0x00000080; // clear NVIC_PRI1_R then set Priority 2 on pin 5-7
  NVIC_EN0_R    =   0x00000010;         // Enable interrupt 4 in NVIC
  
  Count = 0;
}

// PORTF for PED lights
void PortF_Init(void){ volatile unsigned long delay;
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // 1) F clock
  delay = SYSCTL_RCGC2_R;               // delay  
  GPIO_PORTF_LOCK_R   =  0x4C4F434B;    // 2) unlock PortF    
  GPIO_PORTF_AMSEL_R &= ~0x0A;          // 3) disable analog function
  GPIO_PORTF_PCTL_R  &= ~0x0000F0F0;    // 4) clear to enable regular PORT  
  GPIO_PORTF_DIR_R   |=  0x0A;          // 5) PF3,1 as output 
  GPIO_PORTF_AFSEL_R &= ~0x0A;          // 6) no alternate function      
  GPIO_PORTF_DEN_R   |=  0x0A;          // 7) enable digital pins PF3,1        
}

