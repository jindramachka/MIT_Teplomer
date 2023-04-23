#include "main.h"
#include "stm8s.h"
#include "milis.h"
#include "LCD_I2C.h"

//#include "delay.h"
#include <stdio.h>
#include "uart1.h"

// TODO: přidat komentáře

uint16_t index = 0;
uint16_t times[Mindex];
uint16_t last_counter = 0;
uint64_t data = 0;

uint16_t backlight = 1;
uint16_t displayed_temp_sensor = 1;
uint16_t temp_sensor_allowed_reading = 1;
char temp_sensor_str[16] = "Vnitrni ";
char temp_value_str[16];
char current_value_str[16];
char min_max_str[16];
uint32_t time = 0;
uint32_t lasttime = 0;

struct temperature {
    uint8_t L;
    uint8_t R;
};

struct temperature current_temp_internal = {0, 0};
struct temperature current_temp_external = {0, 0};
struct temperature max_temp_internal = {0, 0};
struct temperature max_temp_external = {0, 0};
struct temperature min_temp_internal = {99, 99};
struct temperature min_temp_external = {99, 99};


void trigger_temp_sensor(GPIO_Pin_TypeDef TRIGGER_PIN);
void read_temp_sensor(GPIO_Pin_TypeDef READ_PIN);
void max_min_temp_update(struct temperature *max_temp, struct temperature *min_temp, struct temperature *current_temp);
void print_to_LCD(struct temperature *max_temp, struct temperature *min_temp, struct temperature *current_temp);
void reading_loop(GPIO_Pin_TypeDef TRIGGER_PIN, struct temperature *max_temp, struct temperature *min_temp, struct temperature *current_temp);


typedef enum { WAKE, DATA, SLEEP } state_t;
state_t state = SLEEP;

INTERRUPT_HANDLER(EXTI_PORTC_IRQHandler, 5)
{
    if (temp_sensor_allowed_reading == 1) {
        read_temp_sensor(READ_1);
    }
    else if (temp_sensor_allowed_reading == 2) {
        read_temp_sensor(READ_2);
    }
}

INTERRUPT_HANDLER(EXTI_PORTD_IRQHandler, 6)
{
    if (PUSH(BTN_2) && displayed_temp_sensor == 1) {displayed_temp_sensor = 2; sprintf(temp_sensor_str , "Vnejsi  ");}
    else if (PUSH(BTN_2) && displayed_temp_sensor == 2) {displayed_temp_sensor = 1; sprintf(temp_sensor_str, "Vnitrni ");}

    if (PUSH(BTN_1) && backlight == 1) {
        backlight = 0;
        LCD_I2C_NoBacklight();
    } 
    else if (PUSH(BTN_1) && backlight == 0) {
        backlight = 1;
        LCD_I2C_Backlight();
    }
}

void setup(void)
{
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);      // taktovani MCU na 16MHz sdf
    GPIO_Init(BTN_PORT, BTN_1, GPIO_MODE_IN_PU_IT);
    GPIO_Init(BTN_PORT, BTN_2, GPIO_MODE_IN_PU_IT);
    GPIO_Init(DHT11_PORT, READ_1, GPIO_MODE_IN_PU_IT);
    GPIO_Init(DHT11_PORT, TRIGGER_1, GPIO_MODE_OUT_OD_HIZ_SLOW);
    GPIO_Init(DHT11_PORT, READ_2, GPIO_MODE_IN_PU_IT);
    GPIO_Init(DHT11_PORT, TRIGGER_2, GPIO_MODE_OUT_OD_HIZ_SLOW);

    LCD_I2C_Init(0x27, 16, 2);
    LCD_I2C_Backlight();

    // nastavení citlivosti externího přerušení přerušení
    EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOC, EXTI_SENSITIVITY_RISE_FALL);
    EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOD, EXTI_SENSITIVITY_FALL_ONLY);
    // nastavení priority přerušení
    ITC_SetSoftwarePriority(ITC_IRQ_PORTD, ITC_PRIORITYLEVEL_1);
    ITC_SetSoftwarePriority(ITC_IRQ_PORTC, ITC_PRIORITYLEVEL_2);

    TIM2_TimeBaseInit(TIM2_PRESCALER_16, 0xFFFF);
    TIM2_Cmd(ENABLE);

    // povolení přeruření
    enableInterrupts();

    init_milis();
    init_uart1();
}

int main(void)
{
    setup();

    while (1) {
        if (milis() - time > 5000) {
            temp_sensor_allowed_reading = 1;
            reading_loop(TRIGGER_1, &max_temp_internal, &min_temp_internal, &current_temp_internal);
            temp_sensor_allowed_reading = 2;
            reading_loop(TRIGGER_2, &max_temp_external, &min_temp_external, &current_temp_external);
        }
    }
}

void reading_loop(GPIO_Pin_TypeDef TRIGGER_PIN, struct temperature *max_temp, struct temperature *min_temp, struct temperature *current_temp) {
    time = milis();
    lasttime = milis();
    TIM2_SetCounter(0);
    last_counter = 0;
    index = 0;
    data = 0LL;
    state = WAKE;
    while (state != SLEEP) {
        switch (state) {

        case WAKE:
            if (milis() - lasttime < 19) {
                LOW(TRIGGER_PIN);
            } else {
                lasttime = milis();
                HIGH(TRIGGER_PIN);
                state = DATA;
            }
            break;

        case DATA:
            if (milis() - lasttime > 6) {
                lasttime = milis();

                // Testování funkce pomocí tisknutí do sériového monitoru připojeného přes UART
                // for (int i = 0; i < index; ++i) {
                //     printf("%d: %d, ", i, times[i]) ;
                // }
                // printf("\ndata: 0b ");
                // uint64_t m = 1LL << 39;
                // uint8_t i = 0;
                // while (m) {
                //     if (data & m) {
                //         putchar('1');
                //     } else {
                //         putchar('0');
                //     }
                //     if (++i % 8 == 0)
                //         putchar(' ');
                //     m >>= 1;
                // }
                // printf("\n");
                // uint8_t humidityH = data >> 32;
                // uint8_t humidityL = data >> 24;
                // uint8_t temperatureL = data >> 16;
                // uint8_t temperatureR = data >> 8;
                // uint8_t checksum = data;
                // printf("data: 0x %8X %8X %8X %8X\n", humidityH, humidityL,
                //         temperatureL, temperatureR);
                // printf("data:    %8d %8d %8d %8d\n", humidityH, humidityL,
                //         temperatureL, temperatureR);
                // printf("checksum: ");
                // printf(humidityH + humidityL + temperatureL +
                //         temperatureR == checksum ? ":-)" : ";-(");
                // printf("\n");
                // printf("vlhkost: %d %%, teplota: %d.%d °C\n", humidityH,
                //         temperatureL, temperatureR);
                // printf("Count: %d\n", count);
                // printf("%d.%d C\n", current_temp_internal.L, current_temp_internal.R);
                // printf("%d.%d C\n", current_temp_external.L, current_temp_external.R);

                current_temp->L = data >> 16;
                current_temp->R = data >> 8;
                max_min_temp_update(max_temp, min_temp, current_temp);
                
                if (displayed_temp_sensor == 1) {
                    print_to_LCD(&max_temp_internal, &min_temp_internal, &current_temp_internal);
                }
                else if (displayed_temp_sensor == 2) {
                    print_to_LCD(&max_temp_external, &min_temp_external, &current_temp_external);
                }

                state = SLEEP;
            }
            break;

        default:
            state = SLEEP;
            break;
        }
    }
}

void trigger_temp_sensor(GPIO_Pin_TypeDef TRIGGER_PIN) {
    if (milis() - lasttime < 19) {
        LOW(TRIGGER_PIN);
    } else {
        lasttime = milis();
        HIGH(TRIGGER_PIN);
        state = DATA;
    }
}

void read_temp_sensor(GPIO_Pin_TypeDef READ_PIN) {
    uint16_t pulse_length;

    pulse_length = TIM2_GetCounter() - last_counter;
    last_counter = TIM2_GetCounter();  //  uložím si na příští měření

    if (READ(READ_PIN) == RESET) {
        if (pulse_length > 15 && pulse_length < 30) {
            data = data << 1;
            times[index++] = pulse_length;
        }
        if (pulse_length > 40 && pulse_length < 74) { // Log 1
            data <<= 1;
            data = data | 1;
            times[index++] = pulse_length;
        }
    }
}

void max_min_temp_update(struct temperature *max_temp, struct temperature *min_temp, struct temperature *current_temp) {
    if (current_temp->L > max_temp->L || (current_temp->L == max_temp->L && current_temp->R > max_temp->R)) {
        max_temp->L = current_temp->L;
        max_temp->R = current_temp->R;
    }
    if (current_temp->L < min_temp->L || (current_temp->L == min_temp->L && current_temp->R < min_temp->R)) {
        min_temp->L = current_temp->L;
        min_temp->R = current_temp->R;
    }
}

void print_to_LCD(struct temperature *max_temp, struct temperature *min_temp, struct temperature *current_temp) {
    sprintf(current_value_str, "%s%d.%dC", temp_sensor_str, current_temp->L, current_temp->R);
    sprintf(min_max_str, "L:%d.%dC H:%d.%dC", min_temp->L, min_temp->R, max_temp->L, max_temp->R);
    LCD_I2C_SetCursor(0, 0);
    LCD_I2C_Print(current_value_str);
    LCD_I2C_SetCursor(0, 1);
    LCD_I2C_Print(min_max_str);
}

/*-------------------------------  Assert -----------------------------------*/
#include "__assert__.h"
