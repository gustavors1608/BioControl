/*-----------------------------------------------------------------------------------------------------------------------------------
                                  FUNCIONALIDADES PARA O SISTEMA DE SUPORTE A VIDA VEGETAL
-------------------------------------------------------------------------------------------------------------------------------------
---> CONTROLAR TEMPERATURA DO AR COM EXAUSTORES E AR CONDICIONADO, UMIDADE DO AR, BOMBEAMENTO DE AGUA DE HIDROPONIA/AQUAPONIA, 
 ILUMINAÇÃO ARTIFICIAL, NIVEL DA CAIXA DE AGUA (ENCHER SE ESTIVER MEIO VAZIA), IRRIGAÇÃO DE PLANTAS EM PLANTIO CONVENCIONAL, ETC
-------------------------------------------------------------------------------------------------------------------------------------
-LCD 16X2 I2C
    - exibe dados de temperatura, umidade, e na segunda linha do lcd fica para mostrar sobre atualizacoes sobre o sistema ex: "ligando exaustores, temperatura alta", ou "ligando leds no modo apresentação"
- Controle de Temperatura do Ar
    - Se temperatura passar de X (#DEFINE T_MAX) Ligar exaustores (RELE) até a temperatura chegar na ideal (#DEFINE T_IDEAL) 
    ou passam 30 tempo limite (ex: 30 minutos (#DEFINE timeout_exaustores))
    - Se der o Timeout e a temperatura não baixar, Liga o AR na temperatura minima (#DEFINE T.MIN) e desliga quando atingir A
    temperatura ideal
    - Se a temperatura baixar mais que "T_MIN", Liga o AR na "T_MAX" até atingir A "T_IDEAL" 
- Controle de umidade do AR 
    - se umidade acima de X (#DEFINE U-MAX) liga exaustores (RELE) até chegar na umidade ideal ou menos (#DEFINE U_IDEAL)
    - se passar do Timeout e não baixar até a ideal, Liga o AR no Modo desumidificação
- Aquaponia
    - Liga Bomba 1 (#DEFINE PIN-BOMBA...) por 15 minutos e desliga por 15 minuto, 24h por dia
    - Se a bomba 1 estiver ligada e o sensor de fluxo estiver DESLIGADO, A bomba 1 deve ter queimado ou trancado, então usa a bomba 2
    - Se o nivel da caixa baixar mais que o sensor indicador de 75% Liga um rele (RELE) até ligar o 2º sensor indicador de 100% (2 boias on/off)
-IRRIGAÇÃO
    - ligar solenoide a cada x tempo
-LEDs
    - Liga os LEDs (todos os módulos MOSFET) de X horas (#DEFINE HR_INIF_LED) Até Y hora (#DEFINE HR_FIM_LED) -> USA NTP NO ESP32
    - Liga 4 RELES nestes mesmos horarios (Refletores)
- ALEXA
    - alexa envia sinal para o ESP32 modo apresentacao: Liga os MOSFET dos LEDs azuis em sequencia depois volta ligando os vermelhos, 
      e por fim vai ligando os 2 AO MESMO TEMPO: AZUL -> VERMELHO -> AZUL+VERMELHO
    - Comando Individual on/off para 4 RELES (Refletone central)
-----------------------------------------------------------------------------------------------------------------------------------
DESENVOLVIDO POR: 
DATA INICIO: 24/05/2024
DATA ENTREGA V1:
DATA ULTIMA ATUALIZAÇÃO:

*/


#include <Arduino.h>
#include <Thread.h>
#include "lcd_extend.cpp"
#include <DHT.h>

//tempo entre chamadas das threads
#define T_BOMBA 15*60*1000 // 15 MINUTOS X 60 SEGUNDOS X 1000 MS
#define T_SOLENOIDE 3000 // 3S
#define T_DHT 5000
#define T_LCD 1000

//configuração de operação
#define TEMP_IDEAL 25
#define UMID_IDEAL 70


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

//variavel global que vai servir de base para controlar os shift register
struct Outs{
  bool bomba1 = false;
  bool bomba2 = false;
  bool solenoide_caixa = false;
};
Outs state;

struct Ins{
  byte temperatura = TEMP_IDEAL;
  byte umidade = UMID_IDEAL;
};
Ins input;

//mensagem de alerta ou aviso geral que vai aparecer na segunda linha do lcd durante a execução geral
String msg_lcd_alert = "";

//momento que foi chamado ultima vez a funcao de controle da bomba, usado para calcular quanto tempo falta para acionar ou desligar a bomba de agua
unsigned long millis_last_bomba = 0;

//definição dos objetos do tipo thread
Thread thread_bomba = Thread();
Thread thread_solenoide = Thread();
Thread thread_dht = Thread();
Thread thread_lcd = Thread();

//inicializacao do sensor de temperatura do ar e umidade
DHT dht(PIN_SENSOR_DHT11, DHT11);

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


/**
 * Definicação das funções das threads / rotinas
*/

void main_lcd(){


  //primeira linha do codigo mostra umidade do ar, temperatura do ar, e tempo até ligar a bomba de agua da aquaponia
  lcd.msg(0,0,"Umd:");
  lcd.msg(0,4,String(input.umidade));
  lcd.msg(0,6,"% Temp:");
  lcd.msg(0,13,String(input.temperatura));
  lcd.msg(0,15,"C");


  lcd.msg(1,0,"Bomba           ");
  lcd.msg(1,8,":");
  lcd.msg(1,13,"Min");

  // calcula quanto tempo falta até a proxima chamada da funcao da bomba de agua
  unsigned int segundos_ate_bomba_novamente = (T_BOMBA-(millis()-millis_last_bomba))/1000;

  if(msg_lcd_alert == ""){//se nao tiver alertas, mostra informacao sobre a bomba
    lcd.delete_scroll(1);

    byte minutos = segundos_ate_bomba_novamente / 60;
    byte segundos = segundos_ate_bomba_novamente % 60;

    if(minutos < 10){
      lcd.msg(1,6," "+String(minutos));
    }else{
      lcd.msg(1,6,String(minutos));
    }

    if(segundos < 10){
      lcd.msg(1,9,"0"+String(segundos));
    }else{
      lcd.msg(1,9,String(segundos));
    }



  }else{//se tiver um alerta, entao mostra ele no lugar dos dados sobre a bomba
    
    //segunda linha do lcd fica para mostrar sobre atualizacoes sobre o sistema (ex: "ligando exaustores, temperatura alta")
    lcd.set_scroll(1, msg_lcd_alert);
    lcd.update_scroll(1);
  }

  
}

//verifica o estado da bomba e altera o mesmo, verifica se nao teve problemas na bomba tbm, se teve, liga a outra bomba
void main_bomba_agua(){
  state.bomba1 = !state.bomba1;
  state.bomba2 = false;
  delay(500);

  if (digitalRead(PIN_SENSOR_FLUXO) == false && state.bomba1 == true){
    state.bomba2 = true;
  }
  
  millis_last_bomba = millis();

  set_outs();
}


//verifica se deve encher a caixa 
void main_controll_nivel_caixa(){
  if (digitalRead(PIN_BOIA_MIN) == false){
    state.solenoide_caixa = true;
  }else if(digitalRead(PIN_BOIA_MAX) == true){
    state.solenoide_caixa = false;
  }

  set_outs();
}

//busca dados do sensor dht 11
void main_get_dht(){
  if (!isnan(dht.readHumidity()) == false || isnan(dht.readTemperature()) == false) { // se nao tiver erro na leitura, atribui valor
    input.umidade = int(dht.readHumidity());
    input.temperatura = int(dht.readTemperature());
  }

}

/**
 * funções de uso geral
*/

//atribui ao array os valores que estao na struct de estados das saidas
void set_outs(){
  static bool array_reles[8] = {0,0,0,0,0,0,0,0};

  array_reles[0] = state.bomba1;
  array_reles[1] = state.bomba2;
  array_reles[2] = state.solenoide_caixa;
  //... outras saidas


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

  digitalWrite(pin_latch, HIGH);
  digitalWrite(pin_latch, LOW);
}

//liga os leds em sequencia (nao terminado ainda)
void modo_apresentacao(){
  static bool data_arr_2[8] = {0,0,0,0,0,0,0,0};
  static bool data_arr_3[8] = {0,0,0,0,0,0,0,0};
  static bool data_arr_4[8] = {0,0,0,0,0,0,0,0};

//aciona os azuis em sequencia por ex
  for (byte i = 0; i < 8; i++){
    data_arr_2[i] = 1;
    data_arr_2[i-1] = 0;
    delay(500);
    code_74hc595(data_arr_2,PIN_DATA_LEDS1,PIN_CLOCK_LEDS1,PIN_LATCH_LEDS1);
  }
  for (byte i = 0; i < 4; i++){
    data_arr_3[i] = 1;
    data_arr_3[i-1] = 0;
    delay(500); 
    code_74hc595(data_arr_3,PIN_DATA_LEDS2,PIN_CLOCK_LEDS2,PIN_LATCH_LEDS2);
  }
//apaga e volta ligando os vermelhos
  for (int i = 0; i < sizeof(data_arr_2); i++) {
    data_arr_2[i] = 0;
    data_arr_3[i] = 0;
  }
  code_74hc595(data_arr_2,PIN_DATA_LEDS1,PIN_CLOCK_LEDS1,PIN_LATCH_LEDS1);
  code_74hc595(data_arr_3,PIN_DATA_LEDS2,PIN_CLOCK_LEDS2,PIN_LATCH_LEDS2);
  
  for (byte i = 4; i >0; i--){
    data_arr_3[i] = 1;
    data_arr_3[i+1] = 0;
    delay(500); 
    code_74hc595(data_arr_3,PIN_DATA_LEDS2,PIN_CLOCK_LEDS2,PIN_LATCH_LEDS2);
  }
  for (byte i = 8; i > 0; i--){
    data_arr_4[i] = 1;
    data_arr_4[i+1] = 0;
    delay(500);
    code_74hc595(data_arr_4,PIN_DATA_LEDS3,PIN_CLOCK_LEDS3,PIN_LATCH_LEDS3);
  }
  
  //liga todos
    for (int i = 0; i < sizeof(data_arr_2); i++) {
    data_arr_2[i] = 0;
    data_arr_3[i] = 0;
  }
  code_74hc595(data_arr_2,PIN_DATA_LEDS1,PIN_CLOCK_LEDS1,PIN_LATCH_LEDS1);
  code_74hc595(data_arr_3,PIN_DATA_LEDS2,PIN_CLOCK_LEDS2,PIN_LATCH_LEDS2);
  
  for (byte i = 0; i < 8; i++){
    data_arr_2[i] = 1;
    data_arr_4[i] = 1;
    delay(500);
    code_74hc595(data_arr_2,PIN_DATA_LEDS1,PIN_CLOCK_LEDS1,PIN_LATCH_LEDS1);
    code_74hc595(data_arr_4,PIN_DATA_LEDS3,PIN_CLOCK_LEDS3,PIN_LATCH_LEDS3);
  }
  for (byte i = 0; i < 4; i++){
    data_arr_3[i] = 1;
    data_arr_3[i+4] = 1;
    delay(500); 
    code_74hc595(data_arr_3,PIN_DATA_LEDS2,PIN_CLOCK_LEDS2,PIN_LATCH_LEDS2);
  }

  code_74hc595(data_arr_2,PIN_DATA_RELES,PIN_CLOCK_RELES,PIN_LATCH_RELES);
}


  

void setup(){
  
  // Configuração de sentido das GPIOS
  pinMode(PIN_SENSOR_FLUXO, INPUT);
  pinMode(PIN_BOIA_MAX, INPUT);
  pinMode(PIN_BOIA_MIN, INPUT);
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

  //iniciar cartao sd

  //iniciar tela de carregamento no lcd
  lcd.set_scroll(0,"Inicializando Sistema... ");

  for (byte i = 0; i < 16; i++){
    lcd.setCursor(i, 1);
    lcd.write(byte(0));
    delay(400);
    lcd.update_scroll(0);
  }

  lcd.delete_scroll(1);
  lcd.delete_scroll(0);

  //inicialização das threads

	thread_bomba.onRun(main_bomba_agua);
  thread_solenoide.onRun(main_controll_nivel_caixa);
  thread_dht.onRun(main_get_dht);
  thread_lcd.onRun(main_lcd);

	thread_bomba.setInterval(T_BOMBA);
  thread_solenoide.setInterval(T_SOLENOIDE);
  thread_dht.setInterval(T_DHT);
  thread_lcd.setInterval(T_LCD);

  //rotinas de testes
  lcd.msg(0,0,"Iniciando testes");
    //--- liga todas as saidas por 1 segundo
  state.bomba2 = true;
  state.bomba1 = true;
  state.solenoide_caixa = true;
  set_outs();

  delay(1000);

    //--- desliga todas as saidas
  state.bomba2 = false;
  state.bomba1 = false;
  state.solenoide_caixa = false;
  set_outs();
  
  lcd.msg(1,0,"Sistema OK");

  lcd.clear();

}

void loop(){

  //verifica se deve rodar alguma thread
	if(thread_bomba.shouldRun())
		thread_bomba.run();
	if(thread_solenoide.shouldRun())
		thread_solenoide.run(); 
  if(thread_dht.shouldRun())
		thread_dht.run(); 
  if(thread_lcd.shouldRun())
		thread_lcd.run(); 

  if (Serial.available() > 0) {
    msg_lcd_alert = Serial.readString();
    msg_lcd_alert.trim();
    Serial.println(msg_lcd_alert);
  }
}