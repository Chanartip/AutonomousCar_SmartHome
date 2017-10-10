/*
Name: Chanartip Soonthornwan
ID: 014353883
Email: Chanartip.Soonthornwan@student.csulb.edu
Purpose: To demonstrate traffic lights LEDs working
         on TM4C123 using PortE for input from Toggled switches
         and PortB for 3 LEDs
*/


// ***** 0. Documentation Section *****
// TableTrafficLight.c for Lab 10
// Runs on LM4F120/TM4C123
// Index implementation of a Moore finite state machine to operate a traffic light.  
// Daniel Valvano, Jonathan Valvano
// January 15, 2016

// east/west red light connected to PB5
// east/west yellow light connected to PB4
// east/west green light connected to PB3
// north/south facing red light connected to PB2
// north/south facing yellow light connected to PB1
// north/south facing green light connected to PB0
// pedestrian detector connected to PE2 (1=pedestrian present)
// north/south car detector connected to PE1 (1=car present)
// east/west car detector connected to PE0 (1=car present)
// "walk" light connected to PF3 (built-in green LED)
// "don't walk" light connected to PF1 (built-in red LED)

// ***** 1. Pre-processor Directives Section *****
#include "TExaS.h"

#define GPIO_PORTB_DATA_R       (*((volatile unsigned long *)0x400053FC))
#define GPIO_PORTB_DIR_R        (*((volatile unsigned long *)0x40005400))
#define GPIO_PORTB_AFSEL_R      (*((volatile unsigned long *)0x40005420))
#define GPIO_PORTB_PUR_R        (*((volatile unsigned long *)0x40005510))
#define GPIO_PORTB_DEN_R        (*((volatile unsigned long *)0x4000551C))
#define GPIO_PORTB_LOCK_R       (*((volatile unsigned long *)0x40005520))
#define GPIO_PORTB_CR_R         (*((volatile unsigned long *)0x40005524))
#define GPIO_PORTB_AMSEL_R      (*((volatile unsigned long *)0x40005528))
#define GPIO_PORTB_PCTL_R       (*((volatile unsigned long *)0x4000552C))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))

#define GPIO_PORTE_DATA_R       (*((volatile unsigned long *)0x400243FC))
#define GPIO_PORTE_DIR_R        (*((volatile unsigned long *)0x40024400))
#define GPIO_PORTE_AFSEL_R      (*((volatile unsigned long *)0x40024420))
#define GPIO_PORTE_PUR_R        (*((volatile unsigned long *)0x40024510))
#define GPIO_PORTE_DEN_R        (*((volatile unsigned long *)0x4002451C))
#define GPIO_PORTE_LOCK_R       (*((volatile unsigned long *)0x40024520))
#define GPIO_PORTE_CR_R         (*((volatile unsigned long *)0x40024524))
#define GPIO_PORTE_AMSEL_R      (*((volatile unsigned long *)0x40024528))
#define GPIO_PORTE_PCTL_R       (*((volatile unsigned long *)0x4002452C))
    
// ***** 2. Global Declarations Section *****
#define SW0 0x01
#define SW1 0x02

// FUNCTION PROTOTYPES: Each subroutine defined
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

void PortB_Init (void);       // Initializing Port B
void PortE_Init (void);       // Initializing Port E
void Delay(void);

// ***** 3. Subroutines Section *****

int main(void){ 
    
  unsigned long In; // input from PORTE
  unsigned char current_LED = 4;  //Red by default
    
  TExaS_Init(SW_PIN_PE210, LED_PIN_PB543210,ScopeOff); // activate grader and set system clock to 80 MHz
  PortB_Init();
  PortE_Init();
  EnableInterrupts();
    

  GPIO_PORTB_DATA_R = 0x04; //change back to Red
    
  while(1){
      
      In = GPIO_PORTE_DATA_R;
      
      // Positive logic
      
      if(In == SW0){ // read PE0:sw0 into In
        //move to the next led
          current_LED = current_LED+1;
          if(current_LED == 1)
              GPIO_PORTB_DATA_R = 0x02; //change LED to Yellow
          else if(current_LED == 2)
              GPIO_PORTB_DATA_R = 0x01; //change LED to GREEN
          else{
              current_LED = 0; //reset current LED to red
              GPIO_PORTB_DATA_R = 0x04; //change back to Red
          }
      }
      if (In == SW1){ // read PE1:sw1 into In
        //turn LEDs off
          GPIO_PORTB_DATA_R = 0x00; 
          Delay();
          
        //flash the current LED
          if(current_LED == 1)
              GPIO_PORTB_DATA_R = 0x02; //change LED to Yellow
          else if(current_LED == 2)
              GPIO_PORTB_DATA_R = 0x01; //change LED to GREEN
          else{
              GPIO_PORTB_DATA_R = 0x04; //change back to Red
          }
      }
      

      Delay();
  }
}

void PortB_Init(void){              // LED
  volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x00000002;     // 1) B clock
  delay = SYSCTL_RCGC2_R;           // delay   
  //GPIO_PORTB_LOCK_R = 0x4C4F434B; // 2) unlock PortF PF0  
  GPIO_PORTB_CR_R = 0x07;           // allow changes to PF4-0     
  GPIO_PORTB_AMSEL_R = 0x00;        // 3) disable analog function
  GPIO_PORTB_PCTL_R = 0x00000000;   // 4) GPIO clear bit PCTL  
  GPIO_PORTB_DIR_R = 0x07;          // 5) PF4,PF0 input, PF3,PF2,PF1 output   
  GPIO_PORTB_AFSEL_R = 0x00;        // 6) no alternate function
  //GPIO_PORTB_PUR_R = 0x11;          // enable pullup resistors on PF4,PF0       
  GPIO_PORTB_DEN_R = 0x07;          // 7) enable digital pins PF4-PF0        
}

void PortE_Init(void){              // SW
  volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x00000010;     // 1) E clock
  delay = SYSCTL_RCGC2_R;           // delay   
  //GPIO_PORTE_LOCK_R = 0x4C4F434B; // 2) unlock PortF PF0  
  GPIO_PORTE_CR_R = 0x03;           // allow changes to PF4-0     
  GPIO_PORTE_AMSEL_R = 0x00;        // 3) disable analog function
  GPIO_PORTE_PCTL_R = 0x00000000;   // 4) GPIO clear bit PCTL  
  GPIO_PORTE_DIR_R = 0x00;          // 5) PF4,PF0 input, PF3,PF2,PF1 output   
  GPIO_PORTE_AFSEL_R = 0x00;        // 6) no alternate function
  //GPIO_PORTE_PUR_R = 0x11;        // enable pullup resistors on PF4,PF0       
  GPIO_PORTE_DEN_R = 0x03;          // 7) enable digital pins PF4-PF0        
}

// Subroutine to wait 0.1 sec
// Inputs: None
// Outputs: None
// Notes: ...
void Delay(void){
  unsigned long volatile time;
  time = 727240*200/91;// 0.1sec
  while(time){
		time--;
  }
}
