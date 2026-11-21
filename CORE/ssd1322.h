#ifndef __SSD1322_H
#define __SSD1322_H

#include "main.h"
#include <stdio.h>
#include <stdarg.h>

/* 控制脚：普通 GPIO。SCL/SDA 已交给硬件 SPI1（PA5=SCK, PA7=MOSI, AF5），
 * 不再用软件翻转，所以这里没有 OLED_SCL/OLED_SDA 宏了。 */
#define OLED_RES(x)  ((x) ? (GPIOA->BSRR = GPIO_PIN_2) : (GPIOA->BSRR = (GPIO_PIN_2 << 16)))
#define OLED_DC(x)   ((x) ? (GPIOA->BSRR = GPIO_PIN_3) : (GPIOA->BSRR = (GPIO_PIN_3 << 16)))
#define OLED_CS(x)   ((x) ? (GPIOA->BSRR = GPIO_PIN_4) : (GPIOA->BSRR = (GPIO_PIN_4 << 16)))

void Soft_SPI_SwapByte(uint8_t byte);
void OLED_GPIO_Init(void);
void OLED_Init(void);
void Oled_Init(void); 
void Oled_Write_cmd(uint8_t cmd);
void Oled_Write_data(uint8_t data);

void OLED_Set_Window(uint16_t x1, uint8_t y1, uint16_t x2, uint8_t y2);
void OLED_Write_Buffer(uint8_t *buf, uint32_t len);
#endif /* __SSD1322_H */


