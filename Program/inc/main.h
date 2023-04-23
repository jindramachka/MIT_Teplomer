#ifndef __MAIN_H__
#define __MAIN_H__

#define DHT11_PORT GPIOC
#define READ_1 GPIO_PIN_6
#define READ_2 GPIO_PIN_3
#define TRIGGER_1 GPIO_PIN_7
#define TRIGGER_2 GPIO_PIN_4

#define BTN_PORT GPIOD
#define BTN_1 GPIO_PIN_3
#define BTN_2 GPIO_PIN_2

#define LOW(PIN) GPIO_WriteLow(DHT11_PORT, PIN)
#define HIGH(PIN) GPIO_WriteHigh(DHT11_PORT, PIN)
#define REVERSE(PIN) GPIO_WriteReverse(DHT11_PORT, PIN)

#define READ(PIN) GPIO_ReadInputPin(DHT11_PORT, PIN)
#define PUSH(PIN) (GPIO_ReadInputPin(BTN_PORT, PIN)==RESET)

#define Mindex 99

#endif