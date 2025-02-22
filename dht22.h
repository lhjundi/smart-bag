#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>

// Códigos de retorno para as operações do driver DHT22
#define DHT22_OK 0                        // Operação bem-sucedida
#define DHT22_ERROR_CHECKSUM -1           // Erro de checksum (dados corrompidos)
#define DHT22_ERROR_TIMEOUT -2            // Timeout na comunicação com o sensor
#define DHT22_ERROR_INVALID_DATA -3       // Dados fora dos limites físicos válidos
#define DHT22_ERROR_NOT_INITIALIZED -4    // Driver não foi inicializado

/**
   Inicializa o driver DHT22 para o pino especificado.

   Esta função deve ser chamada antes de qualquer operação de leitura.
   Configura o pino GPIO e inicializa o estado interno do driver.

   pin: Número do pino GPIO conectado ao sensor DHT22
   Retorna: DHT22_OK em caso de sucesso
*/
int dht22_init(uint32_t pin);

/**
   Realiza uma leitura completa do sensor DHT22.

   Esta função gerencia todo o processo de comunicação com o sensor,
   incluindo o envio do sinal de início, leitura dos dados, verificação
   de checksum e conversão para valores reais de temperatura e umidade.

   Respeita automaticamente o intervalo mínimo de 2 segundos entre leituras
   recomendado pelo fabricante. Se chamada antes desse intervalo, a função
   aguardará o tempo necessário.

   temperature: Ponteiro para armazenar a temperatura lida (em °C)
   humidity: Ponteiro para armazenar a umidade lida (em %)

   Retorna um dos seguintes códigos:
     DHT22_OK - Leitura bem-sucedida
     DHT22_ERROR_CHECKSUM - Erro de checksum (dados corrompidos)
     DHT22_ERROR_TIMEOUT - Timeout na comunicação
     DHT22_ERROR_INVALID_DATA - Dados fora dos limites válidos
     DHT22_ERROR_NOT_INITIALIZED - Driver não inicializado
*/
int dht22_read(float *temperature, float *humidity);

#endif // DHT22_H