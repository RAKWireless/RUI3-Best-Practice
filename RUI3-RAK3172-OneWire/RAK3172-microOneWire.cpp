#include "RAK3172-microOneWire.h"
#ifdef __AVR__
#define MOW_CLI()           \
    uint8_t oldSreg = SREG; \
    cli();
#define MOW_SEI() SREG = oldSreg
#else
#define MOW_CLI()
#define MOW_SEI()
#endif

#define GPIO_NUMBER 16
#define GPIO_BASE_ADDR 0x48000000UL
#define GPIO_BASE_OFFSET 0x00000400UL

GPIO_TypeDef *PinToGPIOx(uint32_t pin)
{
    if (pin % GPIO_NUMBER == 0 && pin != 0)
        return (GPIO_TypeDef *)(GPIO_BASE_ADDR + ((pin / GPIO_NUMBER) * GPIO_BASE_OFFSET));
    return (GPIO_TypeDef *)(GPIO_BASE_ADDR + ((pin / GPIO_NUMBER) * GPIO_BASE_OFFSET));
}

uint16_t PinToGPIO_Pin(uint32_t pin)
{
    uint8_t pin_number = pin % GPIO_NUMBER;
    return 1 << pin_number;
    // return pow(2, pin_number);
}

void pinModeOutput(uint8_t pin)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = PinToGPIO_Pin(pin);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PinToGPIOx(pin), &GPIO_InitStruct);
}

void pinModeInput(uint8_t pin)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = PinToGPIO_Pin(pin);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULL_UP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PinToGPIOx(pin), &GPIO_InitStruct);
}

void delayMicros(int m)
{
    uint32_t d = 0;
    while (d < 9 * m)
    {
        asm("\t nop");
        d++;
    }
}

bool oneWire_reset(uint8_t pin)
{
    HAL_GPIO_WritePin(PinToGPIOx(pin), PinToGPIO_Pin(pin), GPIO_PIN_RESET);

    pinModeOutput(pin);
    delayMicros(600);
    pinModeInput(pin);
    MOW_CLI();
    delayMicros(60);
    bool pulse = HAL_GPIO_ReadPin(PinToGPIOx(pin), PinToGPIO_Pin(pin)) == GPIO_PIN_SET;
    MOW_SEI();
    delayMicros(600);
    return !pulse;
}

void oneWire_write(uint8_t data, uint8_t pin)
{
    for (uint8_t i = 8; i; i--)
    {
        pinModeOutput(pin);
        HAL_GPIO_WritePin(PinToGPIOx(pin), PinToGPIO_Pin(pin), GPIO_PIN_RESET);
        MOW_CLI();
        if (data & 1)
        {
            delayMicros(5);
            pinModeInput(pin);
            delayMicros(60);
        }
        else
        {
            delayMicros(60);
            pinModeInput(pin);
            delayMicros(5);
        }
        MOW_SEI();
        data >>= 1;
    }
}

uint8_t oneWire_read(uint8_t pin)
{
    uint8_t data = 0;
    for (uint8_t i = 8; i; i--)
    {
        data >>= 1;
        MOW_CLI();
        HAL_GPIO_WritePin(PinToGPIOx(pin), PinToGPIO_Pin(pin), GPIO_PIN_RESET);

        pinModeOutput(pin);
        delayMicros(2);
        pinModeInput(pin);
        delayMicros(8);
        if (HAL_GPIO_ReadPin(PinToGPIOx(pin), PinToGPIO_Pin(pin)) == GPIO_PIN_SET)
            data |= (1 << 7);
        delayMicros(60);
        MOW_SEI();
    }
    return data;
}