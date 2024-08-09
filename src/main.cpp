/**
 * @file main.cpp
 * @link https://github.com/gustavors1608/BioControl
 * @author Gustavo R. Stroschon 
 * @brief Código fonte principal para o sistema de controle da fazenda vertical.
 * @version 1.5
 * @date 2024-08-09
 * 
 * @copyright Copyright (c) 2024
 * 
 * @details Este código controla os seguintes atuadores:
 *          - Bomba d'água da aquaponia (com redundância e detecção de falhas).
 *          - Irrigação automatizada.
 *          - Exaustores para controle de temperatura e umidade.
 *          - Sistema de iluminação LED indoor.
 *          - Refletor central indoor.
 *          - Lâmpadas normais.
 * 
 *          O controle desses atuadores é realizado com base em:
 *          - Leituras de temperatura e umidade do ar (sensor DHT11).
 *          - Horário (obtido via NTP).
 *          - Comandos de voz via Amazon Alexa.
 * 
 *          O sistema também possui um display LCD para exibir informações sobre o estado atual da Fazenda vertical.
 * 
 *  @todo melhorias futuras:
 *          - retirar codigo inutilizado
 *          - sistemas de segurança contra travamentos ou erros em cascata
 *          - funcoes de simulacao de clima (alvos de temperatura e umidade, talvez pid etc)
 *          - melhorar controle de exaustores, deixando funcao de controle de temp com funcionamento sincronizado com umidade
 */

#include <Arduino.h>
#include <Thread.h>
#include <DHT.h>
#include <CronOut.h>
#include "fauxmoESP.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

#include <OffTime.cpp>
#include "lcd_extend.cpp"

//------------------------------------------------------------------------------
// Definições Globais
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------

// Defina esta macro para ativar o log. Comente para desativar.
#define ACTIVE_DEBUG

#ifdef ACTIVE_DEBUG 
  //logger("mensagem", "local")
  #define logger(x,y) Serial.println("[" + String(y) + "]: " + String(x))
  #define log_point() Serial.print(".")
  #define new_line() Serial.println()
#else
  #define logger(x,y) // Nada
  #define log_point() // nada tambem
  #define new_line() // mais um nada
#endif

#define BAUND_RATE 115200

//------------------------------------------------------------------------------
// Configurações de Rede e API
//------------------------------------------------------------------------------
#define WIFI_SSID "BioControl"
#define WIFI_PASS "Geodese2024"

#define latitude "-28.30"
#define longitude "-54.22"
#define api_key "4dc6bcd36941c512f7dc5588bb34ee1c" 
#define link "https://api.openweathermap.org/data/2.5/weather?lat=" latitude "&lon=" longitude "&units=metric&appid=" api_key

//------------------------------------------------------------------------------
// Configurações de Tempo (em milissegundos)
//------------------------------------------------------------------------------
#define T_DADOS_CLIMATICOS 15*60*1000// Tempo entre atualizações dos dados climáticos externos
#define T_BOMBA 5*60*1000            // Tempo de ciclo da bomba d'água (ligada/desligada)
#define CICLOS_BOMBA_DESLIGADA 3     // Quantos ciclos desligados para cada ciclo ligdado, ex: 3 = 5 ligado e 15 desligado
#define T_DHT 5000                   // Tempo entre leituras do sensor DHT11
#define T_LCD 500                    // Tempo entre atualizações do display LCD
#define T_DURACAO_IRRIGACAO 1*60*1000 // Tempo de duração de cada ciclo de irrigação
#define N_VEZES_CHAMADA_ENTRE_IRRIG 2 // Define a frequência da irrigação em relação ao tempo de duração (N * T_DURACAO_IRRIGACAO)
#define T_VERIFICAR_EXAUSTOR 1*60*1000 // Tempo entre verificações da temperatura para controle dos exaustores
#define T_VERIFICAR_LEDS 3*60*1000   // Tempo entre verificações do horário para controle dos LEDs

//------------------------------------------------------------------------------
// Configurações de Operação da Fazenda Vertical
//------------------------------------------------------------------------------
#define TEMP_MAX 30        // Temperatura máxima desejada na estufa
#define TEMP_IDEAL 25       // Temperatura ideal na estufa
#define TEMP_MIN 20         // Temperatura mínima desejada na estufa
#define TEMP_MARGEM 2      // Margem de temperatura para ativar/desativar o controle

#define UMID_MAX 95         // Umidade máxima permitida na estufa
#define UMID_IDEAL 70        // Umidade ideal na estufa
#define UMID_MAX_EXAUST 90  // Umidade que ativa os exaustores (se a externa for menor)

#define TIMEOUT_EXAUSTORES 60*60*1000 // Tempo máximo que os exaustores podem ficar ligados continuamente

#define HORA_DESLIGAR_LED 21 // Hora para desligar os LEDs
#define HORA_LIGAR_LED 8    // Hora para ligar os LEDs

//------------------------------------------------------------------------------
// Mapeamento de Pinos
//------------------------------------------------------------------------------
#define PIN_SENSOR_DHT11 32
#define PIN_SENSOR_FLUXO 2
#define PIN_BOIA_MAX  34
#define PIN_BOIA_MIN 35
#define PIN_LED_IR 0
#define PIN_DATA_RELES 4
#define PIN_CLOCK_RELES 19
#define PIN_LATCH_RELES 18
#define PIN_DATA_LEDS1  17
#define PIN_CLOCK_LEDS1 16
#define PIN_LATCH_LEDS1 4
#define PIN_DATA_LEDS2 13 
#define PIN_CLOCK_LEDS2 14
#define PIN_LATCH_LEDS2 27
#define PIN_DATA_LEDS3 33
#define PIN_CLOCK_LEDS3 25
#define PIN_LATCH_LEDS3 26

//------------------------------------------------------------------------------
// Definições de Cor dos LEDs
//------------------------------------------------------------------------------
#define RED 1
#define BLUE 0

//------------------------------------------------------------------------------
// Estruturas de Dados
//------------------------------------------------------------------------------

/**
 * @brief Estrutura para armazenar o estado dos atuadores.
 */
struct Outs{
  bool bomba1 = false;
  bool bomba2 = false;
  bool solenoide_caixa = false;
  bool solenoide_irrigacao = false;
  bool exaustor = false;
  bool contatora_leds = false;
  bool refletor = false;
  bool lampada = false;
  bool ac = false;
};

/**
 * @brief Estrutura para armazenar os dados dos sensores.
 */
struct Ins{
  byte temperatura = TEMP_IDEAL;
  byte umidade = UMID_IDEAL;
  bool chuva = false;
  byte umidade_externa = UMID_IDEAL;
};

//------------------------------------------------------------------------------
// Variáveis Globais
//------------------------------------------------------------------------------
Outs state;                // Variável global para armazenar o estado dos atuadores
Ins input;                 // Variável global para armazenar os dados dos sensores
unsigned long millis_last_bomba = 0; // Variável para controlar o tempo de atuação da bomba d'água

bool alexa_controll_exaust = false; // Flag para indicar se o controle dos exaustores está sendo feito pela Alexa
bool alexa_controll_bomba = false;  // Flag para indicar se o controle da bomba d'água está sendo feito pela Alexa
bool alexa_controll_leds = false;   // Flag para indicar se o controle dos LEDs está sendo feito pela Alexa

//------------------------------------------------------------------------------
// Instâncias de Objetos
//------------------------------------------------------------------------------
OffTime offtime;           // Objeto para lidar com o horário
WiFiUDP udp;              // Objeto para comunicação UDP (NTP)
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600); // Objeto para sincronizar o horário com o servidor NTP brasileiro
fauxmoESP fauxmo;         // Objeto para comunicação com a Amazon Alexa

Thread thread_bomba = Thread();          // Thread para controle da bomba d'água
Thread thread_irrigacao = Thread();       // Thread para controle da irrigação
Thread thread_dht = Thread();            // Thread para leitura do sensor DHT11
Thread thread_lcd = Thread();             // Thread para atualização do display LCD
Thread thread_exaustor = Thread();        // Thread para controle dos exaustores
Thread thread_clima = Thread();           // Thread para obter dados climáticos externos
Thread thread_leds = Thread();            // Thread para controle dos LEDs

DHT dht(PIN_SENSOR_DHT11, DHT11);        // Objeto para o sensor DHT11
CronOut exaustor_timeout((60*60*1000),nullptr); // Timeout para os exaustores (60 minutos)
// AC_CTRL ar_condicionado = AC_CTRL();    // Objeto para controle do ar condicionado (não implementado)
CtrlLCD lcd(0x27,16,2);                // Objeto para o display LCD

//------------------------------------------------------------------------------
// Símbolo Personalizado para a Barra de Carregamento do LCD
//------------------------------------------------------------------------------
byte bar_char_custom[8] = {
  0b00000,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b00000
};

//------------------------------------------------------------------------------
// Protótipos de Funções
//------------------------------------------------------------------------------
void wifi_config();
void modo_apresentacao();
void main_bomba_agua();
void set_outs();
void code_74hc595(bool data_arr[], byte pin_data, byte pin_clock, byte pin_latch);
void print_bin(byte aByte);
void main_get_dht();
void main_lcd();
void self_test(bool* state);
void main_irrigacao();
void controll_umid();
void set_state_leds(bool red_value, bool blue_value);
void controll_temp();
void set_led(bool color, byte num_led, bool state);
void ligar_leds();
void desligar_leds();
void main_dados_clima();
void main_exaustores();
void main_leds();

//------------------------------------------------------------------------------
// Funções para Controle da Alexa
//------------------------------------------------------------------------------
#define ID_bomba           "bomba de água"
#define ID_bomba_auto      "aquaponia automática"
#define ID_exaustor        "exaustores"
#define ID_exaustor_auto   "sensor temperatura"
#define ID_leds            "suplementação luminosa"
#define ID_refletor        "refletor central"
#define ID_lampadas        "iluminação"
#define ID_leds_auto       "fonte automática"

//==============================================================================
// Função de Configuração do WiFi
//==============================================================================
void wifi_config() {
    WiFi.mode(WIFI_STA);
    logger("conectando a rede "+String(WIFI_SSID), "WIFI");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Aguarda a conexão
    while (WiFi.status() != WL_CONNECTED) {
        log_point();
        delay(100);
    }
    new_line();

    // Conexão estabelecida
    logger("Ip adquirido: "+ String(WiFi.localIP()), "WIFI");
}

//==============================================================================
// Classe para Controle do Ar Condicionado (Não Implementado)
//==============================================================================
class AC_CTRL{
  private:
    /* data */
  public:
    AC_CTRL(/* args */){};
    void set_temp(byte temp){};
    void set_desumid(){};
    void set_normal(){};
    void on(){};
    void off(){};
};

//==============================================================================
// Função para Obter Dados Climáticos Externos
//==============================================================================
void main_dados_clima(){
    if(WiFi.status()== WL_CONNECTED){
      WiFiClientSecure client;
      HTTPClient http;

      // Ignorar a verificação SSL (não recomendado para produção)
      client.setInsecure();
    
      http.begin(client, link);
      
      int httpResponseCode = http.GET();
      
      String jsonBuffer = "{}"; 
      
      if (httpResponseCode>0) {
        jsonBuffer = http.getString();
      }

      logger("Resposta http: "+String(httpResponseCode),"CLIMA");

      // Free resources
      http.end();

      //Serial.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      if (JSON.typeof(myObject) == "undefined") {
        logger("Erro ao processar o Json", "CLIMA");
        return;
      }
    
      logger("Umidade Externa: " + myObject["main"]["humidity"], "CLIMA");

      input.umidade_externa = myObject["main"]["humidity"];
    }
}

//==============================================================================
// Função para Atualizar o Display LCD
//==============================================================================
void main_lcd(){

  // Primeira linha do LCD: Umidade, Temperatura e Tempo até a Próxima Ativação da Bomba
  char buffer[20];
  sprintf(buffer, "Umd:%d% Temp:%dC", String(input.umidade), String(input.temperatura));
  lcd.msg(0,0,String(buffer));

  // Calcula o tempo restante para ligar/desligar a bomba
  unsigned long tempo_passado = millis() - millis_last_bomba;
  unsigned short segundos_restantes;

  if (state.bomba1 == true || state.bomba2 == true) {
      // Bomba ligada: calcula o tempo restante até desligar
      if (tempo_passado < T_BOMBA) {
          segundos_restantes = (T_BOMBA - tempo_passado) / 1000;
      } else {
          segundos_restantes = 0; 
      }
  } else {
      // Bomba desligada: calcula o tempo restante até ligar
      if (tempo_passado < T_BOMBA * CICLOS_BOMBA_DESLIGADA) {  // Considera o tempo total do ciclo (ligada + desligada)
          segundos_restantes = (T_BOMBA * CICLOS_BOMBA_DESLIGADA - tempo_passado) / 1000;
      } else {
          segundos_restantes = 0; 
      }
  }

  // Formata a mensagem de tempo restante
  byte minutos = segundos_restantes / 60;
  byte segundos = segundos_restantes % 60;
  sprintf(buffer, "Bomba %d:%02d Min", minutos, segundos);
  String msg = String(buffer);

  logger(msg, "MAIN LCD");
  lcd.msg(1,0,msg);
}

//==============================================================================
// Função para Controlar a Bomba D'Água
//==============================================================================
void main_bomba_agua(){
  static char flag_vez_ligar = 0; 


  // Verifica se o controle pela Alexa está desativado
  if(alexa_controll_bomba == false){
    // Lógica de alternância entre as bombas (ainda não implementada a detecção de falhas)
    if(state.bomba1 == false && flag_vez_ligar < CICLOS_BOMBA_DESLIGADA){
      flag_vez_ligar++;
    }
    
    if (state.bomba1 == false && flag_vez_ligar >= CICLOS_BOMBA_DESLIGADA){
      millis_last_bomba = millis();
      state.bomba1 = true; 
      state.bomba2 = false;
      flag_vez_ligar = 0;
    } else if (state.bomba1 == true){
      millis_last_bomba = millis();
      state.bomba1 = false; 
      state.bomba2 = false;
      flag_vez_ligar++;
    }
    set_outs();
  }

  //Serial.println("main_bomba_agua "+String(alexa_controll_bomba) + " contagem: "+String((int)flag_vez_ligar));
}


//==============================================================================
// Função para Ler os Dados do Sensor DHT11
//==============================================================================
void main_get_dht(){
  if (!isnan(dht.readHumidity()) == false || isnan(dht.readTemperature()) == false) { 
    input.umidade = int(dht.readHumidity());
    input.temperatura = int(dht.readTemperature());
  }
}

//==============================================================================
// Função para Controlar a Irrigação
//==============================================================================
void main_irrigacao(){
  static byte vez_de_irrigar = 0;

  vez_de_irrigar++;

  if (vez_de_irrigar >= N_VEZES_CHAMADA_ENTRE_IRRIG){
    vez_de_irrigar = 0;
    state.solenoide_irrigacao = true;
  }else{
    state.solenoide_irrigacao = false;
  }
  set_outs();
}

//==============================================================================
// Função para Controlar os Exaustores
//==============================================================================
void main_exaustores(){
  if(alexa_controll_exaust == false){
    controll_umid(); // verifica o controle de umidade
    controll_temp(); // verifica acoes para controle de temperatura
  }
}

//==============================================================================
// Função para Controlar a Temperatura (Exaustores e Ar Condicionado)
//==============================================================================
void controll_temp(){
  static bool status_ac = false;

  if (input.temperatura > TEMP_MAX) {
    // Temperatura alta: liga os exaustores
    if (state.exaustor == false && status_ac == false) {
        state.exaustor = true;
        exaustor_timeout.start(); 
    } 
  } else if (input.temperatura < TEMP_MIN) {
    // Temperatura baixa: liga o ar condicionado (não implementado)
    state.exaustor = false;
    status_ac = true;
    //ar_condicionado.on();
    //ar_condicionado.set_temp(TEMP_MAX - TEMP_MARGEM); 
  }

  // Verifica se o tempo máximo dos exaustores foi atingido
  if (exaustor_timeout.hasTimedOut() && input.temperatura > TEMP_MAX) {
    state.exaustor  = false;
    // status_ac = true;
    // ar_condicionado.on();
    // ar_condicionado.set_temp(TEMP_MIN + TEMP_MARGEM);
  }

  // Desliga os exaustores e o ar condicionado se a temperatura estiver ideal
  if (input.temperatura <= (TEMP_MAX - TEMP_MARGEM) && input.temperatura >= (TEMP_MIN + TEMP_MARGEM)) {
    state.exaustor  = false;
    //ar_condicionado.off();
  }
}

//==============================================================================
// Função para Controlar a Umidade (Exaustores e Ar Condicionado)
//==============================================================================
void controll_umid(){
  // Liga os exaustores se a umidade estiver alta (mas não no extremo) e temperatura tiver nao tao baixa, 
  // para poder manter a temperatura interna em caso de dia frio e umido, melhor manter quente e umido do que seco e frio (peixes principalemtne)
  if ((input.umidade > UMID_MAX_EXAUST && input.umidade < UMID_MAX) && input.temperatura > TEMP_MIN){
    if(input.chuva == false && input.umidade_externa < UMID_MAX_EXAUST){
      state.exaustor = true;
      exaustor_timeout.start();
    }
  } else if(input.umidade >= UMID_MAX){  // Liga o ar condicionado se a umidade estiver muito alta (não implementado)
      //ar_condicionado.on();
      //delay(1000);
      //ar_condicionado.set_desumid();
  }

  // Desliga os exaustores após o timeout ou se a umidade baixar
  if (exaustor_timeout.hasTimedOut() || input.umidade <= (UMID_MAX_EXAUST - 5)) { 
    state.exaustor  = false;
    // ar_condicionado.on();
    // delay(1000);
    // ar_condicionado.set_normal();
    // delay(1000);
    // ar_condicionado.off();
  }
}

//==============================================================================
// Funções de Uso Geral
//==============================================================================

//==============================================================================
// Função para Auto Teste das Saídas
//==============================================================================
void self_test(bool* state) {
  (*state) = true;
  set_outs();
  delay(100);
  (*state) = false;
  set_outs();
  delay(100);
}

//==============================================================================
// Função para Atualizar o Estado das Saídas
//==============================================================================
void set_outs(){
  static bool array_reles[8] = {0,0,0,0,0,0,0,0};

  array_reles[0] = state.bomba1;
  array_reles[1] = state.bomba2;
  array_reles[2] = state.solenoide_caixa;
  array_reles[3] = state.solenoide_irrigacao;
  array_reles[4] = state.contatora_leds;
  array_reles[5] = state.exaustor;
  array_reles[6] = state.refletor;
  array_reles[7] = state.lampada;

  code_74hc595(array_reles,PIN_DATA_RELES,PIN_CLOCK_RELES,PIN_LATCH_RELES);
}

//==============================================================================
// Função para Enviar Dados para o Shift Register 74HC595
//==============================================================================
void code_74hc595(bool data_arr[], byte pin_data, byte pin_clock, byte pin_latch){
  byte binario = 0b00000000;

  for (int i = 0; i < 8; i++) {
      if (data_arr[i] == 1) {
          bitWrite(binario, 7 - i, 1);
      }
  }

  digitalWrite(pin_latch, LOW);
  shiftOut(pin_data, pin_clock, MSBFIRST, binario);
  digitalWrite(pin_latch, HIGH);
  digitalWrite(pin_latch, LOW);
}

//==============================================================================
// Função para Controlar os LEDs
//==============================================================================
void main_leds(){
  if(alexa_controll_leds == false){
    byte hora = offtime.get_hour();
    if((hora >= HORA_LIGAR_LED) && (hora <= HORA_DESLIGAR_LED)){
      ligar_leds();
    }else{
      desligar_leds();
    }
  }
}

//==============================================================================
// Função para Modo de Apresentação dos LEDs (Não Implementado)
//==============================================================================
void modo_apresentacao(){
  // Lógica para o modo de apresentação dos LEDs (a ser implementada)
}

//==============================================================================
// Função para Ligar os LEDs
//==============================================================================
void ligar_leds(){
  state.contatora_leds = true;
  state.refletor = true;
  set_outs();

  set_state_leds(1,1);
  //Serial.println("ligando leds");
}

//==============================================================================
// Função para Desligar os LEDs
//==============================================================================
void desligar_leds(){
  state.contatora_leds = false;
  state.refletor = false;
  set_outs();

  set_state_leds(0,0);
  //Serial.println("desligando leds");
}

//==============================================================================
// Função para Controlar um LED Individual
//==============================================================================
void set_led(bool color, byte num_led, bool state){
  static bool data_arr_2[8] = {0,0,0,0,0,0,0,0};
  static bool data_arr_3[8] = {0,0,0,0,0,0,0,0};
  static bool data_arr_4[8] = {0,0,0,0,0,0,0,0};

  if(color == RED){
    if(num_led < 8){
      data_arr_2[num_led] = state;
    }else if (num_led >= 8 && num_led < 12){
      data_arr_3[num_led-8] = state;
    }
  }else{
    if(num_led < 4){
      data_arr_3[num_led+4] = state;
    }else if (num_led >= 4 && num_led < 12){
      data_arr_4[num_led-4] = state;
    }    
  }

  code_74hc595(data_arr_2,PIN_DATA_LEDS1,PIN_CLOCK_LEDS1,PIN_LATCH_LEDS1);
  code_74hc595(data_arr_3,PIN_DATA_LEDS2,PIN_CLOCK_LEDS2,PIN_LATCH_LEDS2);
  code_74hc595(data_arr_4,PIN_DATA_LEDS3,PIN_CLOCK_LEDS3,PIN_LATCH_LEDS3);
}

//==============================================================================
// Função para Definir o Estado de Todos os LEDs
//==============================================================================
void set_state_leds(bool red_value, bool blue_value){  
  for (byte i = 0; i < 12; i++){
    set_led(RED, i, red_value);
    set_led(BLUE, i, blue_value);
  }
}

//==============================================================================
// Função de Configuração
//==============================================================================
void setup(){
  
  // Configuração dos pinos
  pinMode(PIN_SENSOR_FLUXO, INPUT);
  pinMode(PIN_LED_IR, OUTPUT);
  pinMode(PIN_DATA_RELES, OUTPUT);
  pinMode(PIN_CLOCK_RELES, OUTPUT);
  pinMode(PIN_LATCH_RELES, OUTPUT);

  pinMode(PIN_DATA_LEDS1, OUTPUT);
  pinMode(PIN_CLOCK_LEDS1, OUTPUT);
  pinMode(PIN_LATCH_LEDS1, OUTPUT);

  pinMode(PIN_DATA_LEDS2, OUTPUT);
  pinMode(PIN_CLOCK_LEDS2, OUTPUT);
  pinMode(PIN_LATCH_LEDS2, OUTPUT);

  pinMode(PIN_DATA_LEDS3, OUTPUT);
  pinMode(PIN_CLOCK_LEDS3, OUTPUT);
  pinMode(PIN_LATCH_LEDS3, OUTPUT);

  Serial.begin(BAUND_RATE);

  // Inicialização do LCD
  lcd.init();                     
  lcd.backlight();
  lcd.createChar(0, bar_char_custom);

  // Inicialização do sensor DHT11
  dht.begin();

  // Tela de carregamento no LCD
  lcd.set_scroll(0,"Inicializando Sistema... ");

  for (byte i = 0; i < 16; i++){
    lcd.setCursor(i, 1);
    lcd.write(byte(0));
    delay(150);
    lcd.update_scroll(0);
  }

  lcd.delete_scroll(1);
  lcd.delete_scroll(0);

  // Inicialização das threads
	thread_bomba.onRun(main_bomba_agua);
  thread_dht.onRun(main_get_dht);
  thread_lcd.onRun(main_lcd);
  thread_irrigacao.onRun(main_irrigacao);
  thread_exaustor.onRun(main_exaustores);
  thread_leds.onRun(main_leds);
  thread_clima.onRun(main_dados_clima);

  thread_exaustor.setInterval(T_VERIFICAR_EXAUSTOR);
  thread_leds.setInterval(T_VERIFICAR_LEDS);
  thread_irrigacao.setInterval(T_DURACAO_IRRIGACAO);
	thread_bomba.setInterval(T_BOMBA);
  thread_dht.setInterval(T_DHT);
  thread_lcd.setInterval(T_LCD);
  thread_clima.setInterval(T_DADOS_CLIMATICOS);

  // Rotinas de testes
  lcd.msg(0,0,"Iniciando testes");

  set_outs(); // Desliga tudo
  delay(1000);

  // Teste das saídas
  self_test(&state.bomba1);  
  self_test(&state.bomba2);  
  self_test(&state.solenoide_irrigacao);  
  self_test(&state.contatora_leds);  
  self_test(&state.exaustor);  
  self_test(&state.refletor);  
  self_test(&state.lampada);  
  
  
  lcd.msg(1,0,"Conectando wifi");
  wifi_config();

  lcd.clear();
  lcd.msg(1,0,"Obtendo horario");

  ntp.begin();               
  ntp.forceUpdate();    
  unsigned long unix_time_ntp = ntp.getEpochTime();
  offtime.set(unix_time_ntp);

  logger("Hora: "+offtime.get_hour(), "OFFTIME");
  
  lcd.msg(1,0,"Config. Alexa");

  // Configuração da Alexa
  fauxmo.createServer(true); 
  fauxmo.setPort(80); 
  fauxmo.enable(true);

  // Adiciona os dispositivos virtuais
  fauxmo.addDevice(ID_bomba);
  fauxmo.addDevice(ID_bomba_auto);
  fauxmo.addDevice(ID_exaustor);
  fauxmo.addDevice(ID_exaustor_auto);
  fauxmo.addDevice(ID_lampadas);
  fauxmo.addDevice(ID_leds);
  fauxmo.addDevice(ID_refletor);
  fauxmo.addDevice(ID_leds_auto); // Adiciona o dispositivo "leds_auto"

  // Define a função de callback para quando o estado de um dispositivo for alterado
  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state_in, unsigned char value) {
        
    logger("Device: "+String(device_name) + " state: "+(state_in ? "ON" : "OFF"),"ALEXA");

    if (strcmp(device_name, ID_bomba)==0) {
      state.bomba1 = state_in;
    } else if (strcmp(device_name, ID_bomba_auto)==0) {
      alexa_controll_bomba = !state_in;
    } else if (strcmp(device_name, ID_exaustor)==0) {
      state.exaustor = state_in;
    } else if (strcmp(device_name, ID_exaustor_auto)==0) {
      alexa_controll_exaust = !state_in;
    } else if (strcmp(device_name, ID_lampadas)==0) {
      state.lampada = state_in;
    } else if (strcmp(device_name, ID_leds)==0) {
      set_state_leds(state_in,state_in);
      state.contatora_leds = state_in;
    } else if (strcmp(device_name, ID_refletor)==0) {
      state.refletor = state_in;
    } else if (strcmp(device_name, ID_leds_auto)==0) {
      alexa_controll_leds = !state_in;
    }
  });
  
  lcd.msg(1,0,"Sistema OK");
  
  lcd.clear();
  
  set_state_leds(1,1);

  millis_last_bomba = millis();

  // Executa as threads ao iniciar
  thread_bomba.run();
  thread_clima.run();
  thread_leds.run();
  thread_irrigacao.run(); 
  thread_dht.run(); 
  thread_lcd.run(); 
  thread_exaustor.run(); 
}

//==============================================================================
// Função de Loop Principal
//==============================================================================
void loop(){

  // Executa as threads
	if(thread_bomba.shouldRun())
		thread_bomba.run();
  if(thread_clima.shouldRun())
		thread_clima.run();
  if(thread_leds.shouldRun())
		thread_leds.run();
	if(thread_irrigacao.shouldRun())
		thread_irrigacao.run(); 
  if(thread_dht.shouldRun())
		thread_dht.run(); 
  if(thread_lcd.shouldRun())
		thread_lcd.run(); 
  if(thread_exaustor.shouldRun())
		thread_exaustor.run(); 

  fauxmo.handle();
  
  set_outs();

  static unsigned long last = millis();
  if (millis() - last > 5000) {
      last = millis();
      logger("Memoria livre: " + String(ESP.getFreeHeap()) + " bytes", "LOOP");
  }
}