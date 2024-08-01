#ifndef OFF_TIME
#define OFF_TIME

#include "Arduino.h"

class OffTime{
private:
  uint32_t _unixtime = 0;

public:
  OffTime(){
    Serial.print("ola mundo");
  }

  //retorna o unix atual (precisa ter ocorrido pelo menos 1 atualização do tipo set seja vindo de servidor ntp, rtc externo etc)
  unsigned long now() {
    static unsigned long last_millis;
    while (millis() - last_millis >= 1000) {
      this->_unixtime++;
      last_millis += 1000;
    }
    return this->_unixtime;
  }

  //define o unix atual, pode ser baseado em um servidor de ntp
  void set(unsigned long unix_new) {
    this->_unixtime = unix_new;
  }

  //retorna hora (0-23)
  int get_hour() {
    return ((_unixtime % (60 * 60 * 24)) / (60 * 60));
  }

  //retorna minuto (0-59)
  int get_minute() {
    return ((_unixtime % (60 * 60)) / 60);
  }

  //retorna segundo (0-59)
  int get_second() {
    return (_unixtime % 60);
  }

  //retorna dia (1-31)
  //Calcula o dia do mês (1-31) dividindo o timestamp Unix por 86400 (segundos em um dia) e obtendo o resto da divisão por 31.
  //Adiciona 1 ao resultado para que o dia seja contado de 1 a 31.
  int get_day() {
    return ((_unixtime / (60 * 60 * 24)) % 31) + 1;
  }

  //retorna mês (1-12)
  // Calcula o mês (1-12) usando um array com o número de dias em cada mês.
  // Itera sobre os meses, somando os dias de cada mês até que o número total de dias acumulados seja maior ou igual ao número de dias decorridos desde o início do ano.
  // O índice do mês no array representa o mês atual.
  int get_month() {
    int dias_no_mes[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int dias_acumulados = 0;
    int mes = 1;
    for (; mes <= 12; mes++) {
      dias_acumulados += dias_no_mes[mes - 1];
      if ((_unixtime / (60 * 60 * 24)) < dias_acumulados) {
        break;
      }
    }
    return mes;
  }

  //retorna ano (desde 1970)
  //Calcula o ano (desde 1970) dividindo o timestamp Unix por 31536000 (segundos em um ano) e adicionando 1970 (o ano de início do timestamp Unix).
  //O cálculo assume que um ano tem 365 dias, ignorando anos bissextos.
  int get_year() {
    return 1970 + (_unixtime / (60 * 60 * 24 * 365));
  }
};

#endif


