#ifndef HX711_H
#define HX711_H

#include <stdint.h>

// Variável global para o fator de escala usado em todas as conversões
extern float scale_factor;

/**
 * Realiza a leitura dos dados brutos do sensor HX711
 * 
 * @param gpio_dt Pino de dados conectado ao DOUT do HX711
 * @param gpio_sck Pino de clock conectado ao SCK do HX711
 * @return Leitura bruta de 24 bits do sensor
 */
int32_t hx711_read(uint32_t gpio_dt, uint32_t gpio_sck);

/**
 * Converte a leitura bruta do sensor para o peso real em unidades calibradas
 * 
 * @param reading Leitura bruta do sensor
 * @return Peso em unidades calibradas
 */
float calculate_weight(int32_t reading);

/**
 * Calibra a balança utilizando um peso conhecido como referência
 * 
 * @param known_weight_reading Leitura bruta do sensor com o peso conhecido
 * @param actual_weight O peso real utilizado para calibração
 */
void calibrate_hx711(int32_t known_weight_reading, float actual_weight);

#endif