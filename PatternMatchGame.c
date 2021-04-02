/*
	Author: Connor Guard
	Student No: N10690352
*/

#include <stdint.h>
#include <stdio.h>
#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>

#define LED1 PD2
#define LED2 PD3
#define LED3 PD4
#define LED4 PD5
#define LED5 PD6
#define LED6 PD7

#define Button1G PB0
#define Button2G PB1
#define Button3G PB2
#define Button4G PB3
#define Button5G PB4
#define Button6G PB5

//timer definitions
#define FREQ (16000000.0)
#define PRESCALE (1024.0)

const int LEDS[] = {LED1, LED2, LED3, LED4, LED5, LED6};
const int Buttons[] = {Button1G, Button2G, Button3G, Button4G, Button5G, Button6G};
const char masks[] = {1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7};

unsigned int adc_value = 0;//variable to store adc value
unsigned int flag_update = 0;//flag to control updation of led

int level = 3;
int released = -1;
int guess=0;
int current =-1;
int last = 0;
char wake = 0;

//interrupts
volatile int overflow_counter = 0;
volatile int Btn_counter = 0;

//function declerations
void adcSetup(void);
int adc_set(void);
int adc_read(void);
void adc_get(void);\
void Buzz(int buzzLength, double tone);
void Wrong_Buzz(void);
void Correct_Buzz(void);
void wakeUp_Buzz(void);
void makeAGuess(int btn);
void StartGame(void);
double getTime(void);
int getRandom(void);
void LED_on(int mask);
void reset_leds(void);
void displaySequence(int sequence[], int length);
int rand(void);
void srand(unsigned int seed);

void setup(void){ 
 	for(int i =0; i<8;i++) //LEDS as output
    {
    	DDRD |= (masks[i]);
    	PIND &= ~(masks[i]);
  	}
  
  for(int i =0; i<6;i++) DDRB &= ~(Buttons[i]); //Button as Input 
    
  //start clock
  TCCR0A = 0;
  TCCR0B = 5; 
  TIMSK0 = 1; 
  
  PCICR |= (1<<PCIE0); //enable pin change interrupts on port B

  PCMSK0 |= (0B111111);  //enable PINB as interrupts in PCMSK0
  
  DDRC |= (1<<0); //buzzer on
  
  adcSetup(); //ADC on
    
  sei();//start interrupts
}

void adcSetup(void){
  int adc_set();// function to start ADC 
  DDRC &= ~(1<<PC5);//set Analog pin A5 as input
  ADMUX |= ((1<<MUX0)|(1<<MUX2)|(1<<REFS0));//Select channel 6 as ADC channel and the Vref is given
  SREG |= (1<<PD7);//enabling Global Interrupt
  ADCSRA |= ((1<<ADEN)|(1<<ADIE));// Enabling ADC and it's interrupt
}

//timer0
ISR(TIMER0_OVF_vect) {
	overflow_counter ++;
  	Btn_counter++;
    adc_get();   
    if(Btn_counter==10) reset_leds(); //turn off led after delay
}

//button / pin change interrupt
ISR(PCINT0_vect){     
  //On wake up
  if(released == -1){
    //seed random number on wake
    srand(TCNT0); 	
    //start value -2
    released=-2;
  	wakeUp_Buzz();
    return;
  }else if(released == -2){
  	released = 0;
    wake = 1;
    return;
  }
 
  //which button?
 for(int btn=0; btn<6; btn++)
    { 
    	if (PINB & masks[btn])
        { 
            released=btn;
          	Btn_counter=0;
          	//reset leds
  			reset_leds();              
          	//turn on LED
          	LED_on(btn+2);
          	//sound
          	Buzz(100,(0.5+(adc_value/1023.0*2))*(btn/10.0)+1 );    
            return;
        } 
    }
  //when button is released
  makeAGuess(released+1);
}

//ISR of ADC
ISR(ADC_vect)
{
  flag_update = 1;
}

//MAIN
int main(void){ 
  
  setup();
 
  
  while(!wake) //wake on button press
  {

  }
  
  StartGame();
}

void StartGame(void){
  //main program loop
  while(1){
    
    _delay_ms(1000); //wait 1 second
        
    int sequence[level]; //init sequence
        
    for(int i=0; i<level;i++)
    {
      sequence[i] = getRandom(); //generate a random sequence
    }
      
    displaySequence(sequence,level);//display sequence
       
    while(1) //guess sequence loop 
    {     
      current = sequence[guess]; //current button being guessed
      if(level == guess)break; //final guess?
    } 
        
    level++; //new level
    guess=0;
    
    if(level>3)//correct sequence entered 
    {     
      _delay_ms(100);
      Correct_Buzz();
    }
  }
}

void makeAGuess(int btn){
  if(btn==current){
  	guess++;
  }else{
    //wrong btn
    level=2;
  	guess = level;
    _delay_ms(50);
    Wrong_Buzz();
  }
}

double getTime(void){
	return (double) TCNT0 * PRESCALE / FREQ;
}


void LED_on(int mask){  
  PORTD |= masks[mask]; //turn on
}

void reset_leds(void){
 for(int i=2; i<8; i++){
  	PORTD &= ~(masks[i]); //turn off all leds
 } 
}

int getRandom(void){
  int random; //random int between 1 and 6 
  	do{
  		random = (rand() % 6)+1;
  	}while(random == last);
  	last = random;
  	return random;
}

void displaySequence(int sequence[], int length){
  for(int i = 0; i<length;i++){
  	LED_on(sequence[i]+1);
    //sound
    Buzz(100,((0.5+(adc_value/1023.0*2))*(sequence[i]-1)/10.0)+1);
    //reset
  	reset_leds();
  }
}

//ADC
int adc_set(void)
{
  ADCSRA |= (1<<ADSC);//starting the ADC
  return 1;
}

int adc_read(void)
{
  //reading the value from ADCH and ADCL to adc_value
  adc_value = ADCH;
  adc_value &= ((1<<0)|(1<<1));
  adc_value = (adc_value<<8);
  adc_value |= ADCL;
  return 1;
}

//gets the adc value
void adc_get(void){
	if(flag_update == 0)
      adc_set(); 
    if(flag_update == 1)
    {
      adc_read();
      flag_update = 0;
    }    
}

//makes sound
void Buzz(int buzzLength, double tone){
 for(int i = 0;i<buzzLength;i++){
     PORTC |= (1<<0);
   for(int t = 0;t<tone*10;t++){
     _delay_ms(0.05);
     
   } 
     PORTC &= ~(1<<0);
   for(int t = 0;t<tone*10;t++){
     _delay_ms(0.05);
     
   }
  }  
}

void Wrong_Buzz(void){
  //plays a sound that symbolises an incorrect entry
  Buzz(20, (9/10.0)+1);
  Buzz(30, (11/10.0)+1);
}

void Correct_Buzz(void){
  //plays a sound that symbolises an correct entry
  Buzz(50, 0.8);
  Buzz(150, 0.5);
}

void wakeUp_Buzz(void){
//plays a sound that symbolises waking up
  Buzz(100, 0.9);
  Buzz(100, 0.8);
  Buzz(100, 0.7);
  Buzz(100, 0.5);
}


