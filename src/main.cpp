/*
  DESENVOLVIDO POR: Gustavo R. Stroschon
  DATA INICIO: 24/05/2024
  DATA ULTIMA ATUALIZAÇÃO: 31/07/2024


  proximas atividades:
  dados de clima externos
*/


#include <Arduino.h>
#include <Thread.h>
#include <DHT.h>
#include <CronOut.h>
//#include "WifiPortal.h"
#include "fauxmoESP.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

#include <OffTime.cpp>
#include "lcd_extend.cpp"

//tempo entre chamadas das threads
#define latitude "-28.30"
#define longitude "-54.22"
#define api_key "SUA_CHAVE_DE_API" 
#define link "https://api.openweathermap.org/data/2.5/weather?lat=" latitude "&lon=" longitude "&units=metric&appid=" api_key

//https://api.openweathermap.org/data/2.5/weather?lat=-28.30&lon=-54.22&appid=ddd9de4839420466345e2e508a5ba36a&units=metric

#define T_DADOS_CLIMATICOS 1*60*1000
#define T_BOMBA 1*60*1000 // 5 MINUTOS X 60 SEGUNDOS X 1000 MS
#define T_DHT 2000
#define T_LCD 500
#define T_DURACAO_IRRIGACAO 1*60*1000 // a cada 30 minutos e a duracao será de 30 minutos ligado, alterar o valor de baixo tbm
#define N_VEZES_CHAMADA_ENTRE_IRRIG 2 //max 255, a cada N_VEZES_CHAMADA_ENTRE_IRRIG chamadas (cada chamada a cada T_DURACAO_IRRIGACAO) vai irrigar, para descobrir o tempo: N_VEZES_CHAMADA_ENTRE_IRRIG x T_DURACAO_IRRIGACAO / 60 = a cada quantas horas vai irrigar
#define T_VERIFICAR_EXAUSTOR 60*1000 //1 vez a cada 60 segundos
#define T_VERIFICAR_LEDS 3*60*1000//a cada 3 minuto verifica se ja está na hora de ligar ou desligar

//configuração de operação
#define TEMP_MAX 30
#define TEMP_IDEAL 25
#define TEMP_MIN 20
#define TEMP_MARGEM 2

#define UMID_MAX 95
#define UMID_IDEAL 70
#define UMID_MAX_EXAUST 90

#define TIMEOUT_EXAUSTORES 60*60*1000 //em Ms

#define HORA_DESLIGAR_LED 21
#define HORA_LIGAR_LED 8

//configuração de pinos

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
 
#define PIN_DATA_LEDS2 13//12
#define PIN_CLOCK_LEDS2 14
#define PIN_LATCH_LEDS2 27

#define PIN_DATA_LEDS3 33
#define PIN_CLOCK_LEDS3 25
#define PIN_LATCH_LEDS3 26

#define RED 1
#define BLUE 0




// Inicializa o WifiPortal
//WifiPortal wifiPortal;

#define WIFI_SSID "BioControl"
#define WIFI_PASS "Geodese2024"

void wifiSetup() {

    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);

    // Connect
    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Wait
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println();

    // Connected!
    Serial.printf("[WIFI] SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    

}
//classe de controle do ar condicionado
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

//variavel global que vai servir de base para controlar os shift register
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
Outs state;

struct Ins{
  byte temperatura = TEMP_IDEAL;
  byte umidade = UMID_IDEAL;

  //estacao meteorologica
  bool chuva = false;
  bool umidade_externa = UMID_IDEAL;
};
Ins input;

//momento que foi chamado ultima vez a funcao de controle da bomba, usado para calcular quanto tempo falta para acionar ou desligar a bomba de agua
unsigned long millis_last_bomba = 0;

//usada para desativar o controle automatico caso a alexa que tenha mandado comando
bool alexa_controll_exaust = false;
bool alexa_controll_bomba = false;
bool alexa_controll_leds = false;


OffTime offtime;
WiFiUDP udp;
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600);/* Cria um objeto "NTP" com as configurações utilizadas no Brasil */

fauxmoESP fauxmo;

//definição dos objetos do tipo thread
Thread thread_bomba = Thread();
Thread thread_irrigacao = Thread();
Thread thread_dht = Thread();
Thread thread_lcd = Thread();
Thread thread_exaustor = Thread();
Thread thread_clima = Thread();
Thread thread_leds = Thread();

//inicializacao do sensor de temperatura do ar e umidade
DHT dht(PIN_SENSOR_DHT11, DHT11);

//configuraçoes dos timeouts
CronOut exaustor_timeout((60*60*1000),nullptr); // Timeout de 60 minutos sem callback

//controle do ar condicionado
AC_CTRL ar_condicionado = AC_CTRL();

//inicializacao do lcd
CtrlLCD lcd(0x27,16,2);

//declaracao das funcoes
void modo_apresentacao();
void main_bomba_agua();
void main_controll_nivel_caixa();
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

//simbolo personalizado usado para barra de carregamento
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




// Função de callback para mostrar enquanto o portal está aberto, poderia ser um led que fica piscando enquanto está configurando
void config_callback() {
  lcd.msg(1,0,"Configure o WiFi");
}


/**
 * Definicação das funções para a alexa
 * exaustores
 * leds
 * refletor
 * luz
 * bomba
 * irrigacao
 * 
*/


#define ID_bomba           "bomba de água"
#define ID_bomba_auto      "aquaponia automática"
#define ID_exaustor        "exaustores"
#define ID_exaustor_auto   "sensor temperatura"
#define ID_leds            "suplementação luminosa"
#define ID_refletor        "refletor central"
#define ID_lampadas        "iluminação"
#define ID_leds_auto       "fonte automática"


/**
 * Definicação das funções das threads / rotinas
*/


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
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        jsonBuffer = http.getString();
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();

      //Serial.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
    
      
      Serial.print("Temperatura: ");
      Serial.println(myObject["main"]["temp"]);
      
      Serial.print("Umidade: ");
      Serial.println(myObject["main"]["humidity"]);
      input.umidade_externa = myObject["main"]["humidity"];
    }

}

void main_lcd(){

  //primeira linha do codigo mostra umidade do ar, temperatura do ar, e tempo até ligar a bomba de agua da aquaponia
  lcd.msg(0,0,"Umd:");
  lcd.msg(0,4,String(input.umidade));
  lcd.msg(0,6,"% Temp:");
  lcd.msg(0,13,String(input.temperatura));
  lcd.msg(0,15,"C");

// Define os tempos de operação em milissegundos
#define TEMPO_LIGADA  T_BOMBA
#define TEMPO_DESLIGADA  T_BOMBA*2

// Calcula quanto tempo passou desde a última mudança de estado
unsigned long tempo_passado = millis() - millis_last_bomba;

// Variáveis para armazenar o tempo restante em segundos
unsigned short segundos_restantes;

// Verifica o estado da bomba para calcular o tempo restante
if (state.bomba1 == true or state.bomba2 == true) {
    // Se a bomba está ligada, calcula o tempo restante até desligar
    if (tempo_passado < TEMPO_LIGADA) {
        segundos_restantes = (TEMPO_LIGADA - tempo_passado) / 1000;
    } else {
        segundos_restantes = 0; // Se já passou o tempo, ela deve ser desligada
    }
} else {
    // Se a bomba está desligada, calcula o tempo restante até ligar
    if (tempo_passado < TEMPO_DESLIGADA) {
        segundos_restantes = (TEMPO_DESLIGADA - tempo_passado) / 1000;
    } else {
        segundos_restantes = 0; // Se já passou o tempo, ela deve ser ligada
    }
}

// Calcula os minutos e segundos restantes
byte minutos = segundos_restantes / 60;
byte segundos = segundos_restantes % 60;

// Formata a mensagem com o tempo restante
char buffer[20];
sprintf(buffer, "Bomba %d:%02d Min", minutos, segundos);
String msg = String(buffer);


  Serial.println(msg);
  lcd.msg(1,0,msg);
}

//verifica o estado da bomba e altera o mesmo, verifica se nao teve problemas na bomba tbm, se teve, liga a outra bomba
void main_bomba_agua(){
  //bomba de agua, se nao tiver fluxo e ja trocou a bomba, entao espera pq pode ter secado a caixa

  static char flag_vez_ligar = 0; //para cada vez ligada, deve aguardar 2 desligada
  Serial.println("main_bomba_agua "+String(alexa_controll_bomba) + " contagem: "+String((int)flag_vez_ligar));

  if(alexa_controll_bomba == false){

    if (state.bomba1 == false and flag_vez_ligar >= 2){//se tiver desligada e for ligar, verifica ja se a flag é igual 2 
      millis_last_bomba = millis();
      state.bomba1 = true; 
      state.bomba2 = false;
      flag_vez_ligar = 0;

    }else if (state.bomba1 == true){//se tiver ligada, vai desligar e somar 1 vez na flag
      millis_last_bomba = millis();
      state.bomba1 = false; 
      state.bomba2 = false;
      flag_vez_ligar++;

    }else {//se a bomba tiver desligada e a flag nao foi atingida
      flag_vez_ligar++;
    }
    
    set_outs();
    
    /*
    delay(1000);
    if (digitalRead(PIN_SENSOR_FLUXO) == false && state.bomba1 == true){
      state.bomba1 = false;
      state.bomba2 = true;
      set_outs();
      delay(1000);

      if (digitalRead(PIN_SENSOR_FLUXO) == false && state.bomba2 == true){
        //ligou as 2 e ainda nao teve flxo? entao pode ser que nao tenha agua.... desliga e espera a proxima chamada
        state.bomba1 = false;
        state.bomba2 = false;
        set_outs();
      }
    }*/
    
  }
}


//busca dados do sensor dht 11
void main_get_dht(){
  if (!isnan(dht.readHumidity()) == false || isnan(dht.readTemperature()) == false) { // se nao tiver erro na leitura, atribui valor
    input.umidade = int(dht.readHumidity());
    input.temperatura = int(dht.readTemperature());
  }

}

//a cada 20x tempo liga a irrigacao durante x tempo 
void main_irrigacao(){
  static byte vez_de_irrigar = 0;
  //a cada tantas chamadas em 1 delas mantem ligada até a proxima chamada
  vez_de_irrigar++;

  if (vez_de_irrigar >= N_VEZES_CHAMADA_ENTRE_IRRIG){
    vez_de_irrigar = 0;
    state.solenoide_irrigacao = true;
  }else{
    state.solenoide_irrigacao = false;
  }
  set_outs();
}

//verifica se temperatura está ideal
void main_exaustores(){
  if(alexa_controll_exaust == false){
    controll_temp();
    controll_umid();
  }
}

void controll_temp(){
  static bool status_ac = false;

  if (input.temperatura > TEMP_MAX) {
    //se tiver quente e tudo desligado, liga exaust por 1 hora pra ver se baixa
    if (state.exaustor == false && status_ac == false) {
        state.exaustor = true;
        exaustor_timeout.start();  // reinicia o contador
    } 
  } else if (input.temperatura < TEMP_MIN) {
    // tá muito frio, entao liga o ac pra esquentar, e desliga o exaust se tiver ligado
    state.exaustor = false;
    status_ac = true;
    ar_condicionado.on();
    ar_condicionado.set_temp(TEMP_MAX - TEMP_MARGEM);  // 28
  }

  //verifica se o exautor resolveu, se nao liga o ac
  if (exaustor_timeout.hasTimedOut() && input.temperatura > TEMP_MAX) {//se ja ja ta ligado a tempo suficiente, e a temperatura ainda ta acima do maximo, entao liga o ar
    state.exaustor  = false;

    status_ac = true;
    ar_condicionado.on();
    ar_condicionado.set_temp(TEMP_MIN + TEMP_MARGEM);  // 22
  }

  // se a temperatura estiver ideal, desliga exaust e ac
  if (input.temperatura <= (TEMP_MAX - TEMP_MARGEM) && input.temperatura >= (TEMP_MIN + TEMP_MARGEM)) {
    state.exaustor  = false;
    ar_condicionado.off();
  }
}

void controll_umid(){
  //se tiver abaixo do extremo mas aind aalta, liga os exaustores (se nao estiver chovendo)
  if (input.umidade > UMID_MAX_EXAUST && input.umidade < UMID_MAX){
    //liga exaustores até baixar essa umidade, se nao tiver chovendo e a umidade externa tiver mais baixa que a maxima interna
    if(input.chuva == false && input.umidade_externa < UMID_MAX_EXAUST){
      state.exaustor = true;
      exaustor_timeout.start();
    }
  }else if(input.umidade >= UMID_MAX){  // se tiver no extremo superior de umidade liga o ac
      ar_condicionado.on();
      delay(1000);
      ar_condicionado.set_desumid();
  }

  if (exaustor_timeout.hasTimedOut() || input.umidade <= (UMID_MAX_EXAUST - 5)) { //desliga após um tempo ou se atingir a umidade
    state.exaustor  = false;
    ar_condicionado.on();
    delay(1000);
    ar_condicionado.set_normal(); //tira do modo desumidificador
    delay(1000);
    ar_condicionado.off();
  }
}



/**
 * funções de uso geral
*/

//obs, recebe o endereco da variavel de saida, altera o state dela direto na memoria com ponteiro e define as saidas, por que isso? para alterar as saidas sem precisar digitar essas funcoes todas pra cada saida difetrente por cada saida ter um nome diferente
void self_test(bool* state) {//auto teste para saidas
  (*state) = true;
  set_outs();
  delay(100);//tempo ligado
  (*state) = false;
  set_outs();
  delay(100);//tempo desligado
}

//atribui ao array os valores que estao na struct de estados das saidas
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

//envia array de dados para o 74hc595 
void code_74hc595(bool data_arr[], byte pin_data, byte pin_clock, byte pin_latch){
  // Variável para armazenar o número binário
  byte binario = 0b00000000;

  for (int i = 0; i < 8; i++) {
      // Se o valor no índice 'i' for 1, define o bit correspondente para 1
      if (data_arr[i] == 1) {
          bitWrite(binario, 7 - i, 1);
      }
  }
  //print_bin(binario);

  digitalWrite(pin_latch, LOW);

  shiftOut(pin_data, pin_clock, MSBFIRST, binario);

  delay(1);
  digitalWrite(pin_latch, HIGH);
  delay(1);
  digitalWrite(pin_latch, LOW);
}

void main_leds(){
  //verifica o horario atual, se ja estiver na hora de ligar, liga, se estiver na hora de desligar, desliga
  //verifica o horario de acordo com um relogio interno que é constantemente atualizado pelo ntp
  if(alexa_controll_leds == false){
    byte hora = offtime.get_hour();
    if(hora >= HORA_LIGAR_LED and hora <= HORA_DESLIGAR_LED){
      ligar_leds();
    }else{
      desligar_leds();
    }
  }
}


//liga os leds em sequencia (nao terminado ainda)
void modo_apresentacao(){
  //liga 1 por um azul, apaga, depois 1 por um vermelho do outro sentido, apaga, depois tudo azul, depois tudo vermelho e dai tudo roxo
  for (byte i = 0; i < 12; i++){
    set_led(BLUE, i, true);
    delay(500);
    set_state_leds(0,0);
  }
  for (byte i = 12; i > 0; i--){
    set_led(RED, i, true);
    delay(500);
    set_state_leds(0,0);
  }
  delay(500);//apagado
  set_state_leds(0,1);//azul
  delay(1000);
  set_state_leds(1,0);//vermelho
  delay(1000);
  set_state_leds(0,0);//apagado
  delay(1000);
  //set_state_leds(1,1);//roxo

  bool usados[12];
  for (int i = 0; i < 12; i++) {
    byte num_aleatorio;
    do {
      num_aleatorio = random(12);
    } while (usados[num_aleatorio]);
    usados[num_aleatorio] = true;
    set_led(RED, num_aleatorio,true);
    set_led(BLUE, num_aleatorio,true);
    delay(200);
  }

}

void ligar_leds(){
  state.contatora_leds = true;
  state.refletor = true;
  set_outs();

  set_state_leds(1,1);
  Serial.println("ligando leds");
}

void desligar_leds(){
  state.contatora_leds = false;
  state.refletor = false;
  set_outs();

  set_state_leds(0,0);
  Serial.println("desligando leds");
}

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

void set_state_leds(bool red_value, bool blue_value){  
  //12 saidas vermelhas e 12 azuis *OBS: podem estar invertidas, mas ao testar se descobre

  for (byte i = 0; i < 12; i++){
    set_led(RED, i, red_value);
    set_led(BLUE, i, blue_value);
  }
  

}

  

void setup(){
  
  // Configuração de sentido das GPIOS
  pinMode(PIN_SENSOR_FLUXO, INPUT);
  pinMode(PIN_LED_IR, OUTPUT);

  // Configuração dos pinos dos registradores de deslocamento como saída
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

  Serial.begin(9600);

  //inicialização do lcd
  lcd.init();                     
  lcd.backlight();
  lcd.createChar(0, bar_char_custom);

  //iniciar o sensor dht11
  dht.begin();

  //iniciar tela de carregamento no lcd
  lcd.set_scroll(0,"Inicializando Sistema... ");

  for (byte i = 0; i < 16; i++){
    lcd.setCursor(i, 1);
    lcd.write(byte(0));
    delay(150);
    lcd.update_scroll(0);
  }

  lcd.delete_scroll(1);
  lcd.delete_scroll(0);

  //inicialização das threads

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

  //rotinas de testes
  lcd.msg(0,0,"Iniciando testes");

  set_outs();//desliga tudo
  delay(1000);
  //--- pulsa cada saida
  self_test(&state.bomba1);  
  self_test(&state.bomba2);  
  self_test(&state.solenoide_irrigacao);  
  self_test(&state.contatora_leds);  
  self_test(&state.exaustor);  
  self_test(&state.refletor);  
  self_test(&state.lampada);  
  
  
  lcd.msg(1,0,"Conectando wifi");
  wifiSetup();

  //wifiPortal.set_config_callback(config_callback, 500);
  //wifiPortal.connect();

  lcd.clear();

  lcd.msg(1,0,"Obtendo horario");

  ntp.begin();               /* Inicia o protocolo */
  ntp.forceUpdate();    /* Atualização */
  unsigned long unix_time_ntp = ntp.getEpochTime();
  offtime.set(unix_time_ntp);

  Serial.println(offtime.get_hour());
  
  lcd.msg(1,0,"Config. Alexa");


// By default, fauxmoESP creates it's own webserver on the defined port
    // The TCP port must be 80 for gen3 devices (default is 1901)
    // This has to be done before the call to enable()
    fauxmo.createServer(true); // not needed, this is the default value
    fauxmo.setPort(80); // This is required for gen3 devices

    // You have to call enable(true) once you have a WiFi connection
    // You can enable or disable the library at any moment
    // Disabling it will prevent the devices from being discovered and switched
    fauxmo.enable(true);

    // You can use different ways to invoke alexa to modify the devices state:
    // "Alexa, turn yellow lamp on"
    // "Alexa, turn on yellow lamp
    // "Alexa, set yellow lamp to fifty" (50 means 50% of brightness, note, this example does not use this functionality)

    // Add virtual devices
    fauxmo.addDevice(ID_bomba);
    fauxmo.addDevice(ID_bomba_auto);
    fauxmo.addDevice(ID_exaustor);
    fauxmo.addDevice(ID_exaustor_auto);
    fauxmo.addDevice(ID_lampadas);
    fauxmo.addDevice(ID_leds);
    fauxmo.addDevice(ID_refletor);

    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state_in, unsigned char value) {
        

        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state_in ? "ON" : "OFF", value);

        if (strcmp(device_name, ID_bomba)==0) {
          state.bomba1 = state_in;
        } else if (strcmp(device_name, ID_bomba_auto)==0) {
          //invertido pois quando tiver no modo automatico (true), a alexa não irá controlar
          alexa_controll_bomba = !state_in;
        } else if (strcmp(device_name, ID_exaustor)==0) {
          state.exaustor = state_in;
        } else if (strcmp(device_name, ID_exaustor_auto)==0) {
          //invertido pois quando tiver no modo automatico (true), a alexa não irá controlar
          alexa_controll_exaust = !state_in;
        } else if (strcmp(device_name, ID_lampadas)==0) {
          state.lampada = state_in;
        } else if (strcmp(device_name, ID_leds)==0) {
          set_state_leds(state_in,state_in);
          state.contatora_leds = state_in;
        } else if (strcmp(device_name, ID_refletor)==0) {
          state.refletor = state_in;
        }else if (strcmp(device_name, ID_leds_auto)==0) {
          //invertido pois quando tiver no modo automatico (true), a alexa não irá controlar
          alexa_controll_leds = !state_in;
        }

    });
  
  lcd.msg(1,0,"Sistema OK");
  
  lcd.clear();
  
  set_state_leds(1,1);

  millis_last_bomba = millis();

  
}

void loop(){

  //verifica se deve rodar alguma thread
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

  set_outs();

  fauxmo.handle();
}