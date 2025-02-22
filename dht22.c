#include "dht22.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// Constantes de temporização para o protocolo do DHT22
#define DHT22_START_SIGNAL_DELAY 18000 // 18ms em microssegundos
#define DHT22_RESPONSE_WAIT_TIMEOUT 200 // Timeout para resposta em microssegundos
#define DHT22_BIT_THRESHOLD 50         // Limite para distinguir entre bit 0 e bit 1 (em μs)
#define DHT22_MIN_INTERVAL_MS 2000     // Intervalo mínimo entre leituras (2 segundos)

// Estado do driver DHT22
typedef struct {
    uint32_t last_read_time_ms;  // Timestamp da última leitura
    uint32_t pin;                // Pino GPIO usado para comunicação
    bool initialized;            // Indicador de inicialização
} dht22_state_t;

// Estado global do driver
static dht22_state_t dht22_state = {0, 0, false};

// Função auxiliar para esperar até o pino mudar de estado
static inline int wait_for_pin_state(uint32_t pin, bool state, uint32_t timeout_us) {
    uint32_t start = time_us_32();
    while (gpio_get(pin) != state) {
        if ((time_us_32() - start) > timeout_us) {
            return -1; // Timeout
        }
    }
    return 0; // Sucesso
}

// Inicializa o driver DHT22
int dht22_init(uint32_t pin) {
    // Configura o pino GPIO
    gpio_init(pin);
    gpio_set_pulls(pin, true, false); // Habilita pull-up
    
    // Inicializa o estado do driver
    dht22_state.pin = pin;
    dht22_state.last_read_time_ms = 0;
    dht22_state.initialized = true;
    
    return DHT22_OK;
}

// Envia o sinal de inicialização para o sensor
static int dht22_send_start_signal(uint32_t pin) {
    // Configura o pino como saída
    gpio_set_dir(pin, GPIO_OUT);
    
    // Envia o sinal de início (LOW por 18ms, depois HIGH por 30us)
    gpio_put(pin, 0);
    sleep_us(DHT22_START_SIGNAL_DELAY);
    gpio_put(pin, 1);
    sleep_us(30);
    
    // Configura o pino como entrada para receber a resposta
    gpio_set_dir(pin, GPIO_IN);
    
    return DHT22_OK;
}

// Aguarda e verifica a resposta inicial do sensor
static int dht22_wait_for_response(uint32_t pin) {
    // Aguarda a sequência de resposta do sensor
    if (wait_for_pin_state(pin, 0, DHT22_RESPONSE_WAIT_TIMEOUT) != 0) return DHT22_ERROR_TIMEOUT;
    if (wait_for_pin_state(pin, 1, DHT22_RESPONSE_WAIT_TIMEOUT) != 0) return DHT22_ERROR_TIMEOUT;
    if (wait_for_pin_state(pin, 0, DHT22_RESPONSE_WAIT_TIMEOUT) != 0) return DHT22_ERROR_TIMEOUT;
    
    return DHT22_OK;
}

// Lê os 40 bits de dados do sensor
static int dht22_read_data(uint32_t pin, uint8_t *data) {
    // Lê os 40 bits (5 bytes) de dados
    for (int i = 0; i < 40; i++) {
        // Aguarda o início do pulso (LOW para HIGH)
        if (wait_for_pin_state(pin, 1, DHT22_RESPONSE_WAIT_TIMEOUT) != 0) return DHT22_ERROR_TIMEOUT;
        
        // Mede a duração do pulso HIGH
        uint32_t pulse_start = time_us_32();
        if (wait_for_pin_state(pin, 0, DHT22_RESPONSE_WAIT_TIMEOUT) != 0) return DHT22_ERROR_TIMEOUT;
        uint32_t pulse_length = time_us_32() - pulse_start;
        
        // Determina o valor do bit com base na duração do pulso
        if (pulse_length > DHT22_BIT_THRESHOLD) {
            data[i / 8] |= (1 << (7 - (i % 8))); // Define bit 1
        }
        // Para bit 0, não precisa fazer nada (já inicializamos com zeros)
    }
    
    return DHT22_OK;
}

// Verifica o checksum dos dados recebidos
static int dht22_verify_checksum(const uint8_t *data) {
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return DHT22_ERROR_CHECKSUM;
    }
    return DHT22_OK;
}

// Converte os dados brutos em valores de temperatura e umidade
static int dht22_convert_data(const uint8_t *data, float *temperature, float *humidity) {
    // Calcula umidade
    *humidity = ((data[0] << 8) | data[1]) * 0.1;
    
    // Calcula temperatura com sinal
    *temperature = ((data[2] & 0x7F) << 8 | data[3]) * 0.1;
    if (data[2] & 0x80) {
        *temperature *= -1; // Temperatura negativa
    }
    
    // Valida os valores obtidos
    if (*humidity < 0.0 || *humidity > 100.0 || *temperature < -40.0 || *temperature > 80.0) {
        return DHT22_ERROR_INVALID_DATA;
    }
    
    return DHT22_OK;
}

// Função principal para ler temperatura e umidade do DHT22
int dht22_read(float *temperature, float *humidity) {
    int result;
    uint8_t data[5] = {0};
    
    // Verifica se o driver foi inicializado
    if (!dht22_state.initialized) {
        return DHT22_ERROR_NOT_INITIALIZED;
    }
    
    // Verifica o intervalo mínimo entre leituras
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if ((current_time - dht22_state.last_read_time_ms) < DHT22_MIN_INTERVAL_MS && dht22_state.last_read_time_ms != 0) {
        sleep_ms(DHT22_MIN_INTERVAL_MS - (current_time - dht22_state.last_read_time_ms));
    }
    
    // Envia o sinal de início
    result = dht22_send_start_signal(dht22_state.pin);
    if (result != DHT22_OK) return result;
    
    // Aguarda a resposta do sensor
    result = dht22_wait_for_response(dht22_state.pin);
    if (result != DHT22_OK) return result;
    
    // Lê os dados
    result = dht22_read_data(dht22_state.pin, data);
    if (result != DHT22_OK) return result;
    
    // Atualiza o timestamp da última leitura
    dht22_state.last_read_time_ms = to_ms_since_boot(get_absolute_time());
    
    // Verifica o checksum
    result = dht22_verify_checksum(data);
    if (result != DHT22_OK) return result;
    
    // Converte e valida os dados
    return dht22_convert_data(data, temperature, humidity);
}