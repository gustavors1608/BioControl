#include <Arduino.h>
#include <LiquidCrystal_I2C.h>


//classe para extender a lib original, adicionando funcoes 
class CtrlLCD : public LiquidCrystal_I2C {
  private:

    String msg_scroll[2] = {" ", " "}; //1 mensagem para cada linha
    byte size_msg_scroll[2]; // Comprimento da mensagem
    bool flag_scroll[2]; // Flag para habilitar o scrolling
    byte position[2];

  public:

    CtrlLCD(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows) : LiquidCrystal_I2C(lcd_Addr,lcd_cols,lcd_rows){
      delete_scroll(0);
      delete_scroll(1);
    }

    // printa um binario por extenso (se jogar direto acaba convertendo pra decimal)
    void print_bin(byte binario) {
      for (int8_t posicao = 7; posicao >= 0; posicao--)
        Serial.write(bitRead(binario, posicao) ? '1' : '0');
    }

    //simplifica a etapa de mostrar informaçoes em somente uma função
    void msg(const byte linha, const byte coluna, const String & txt){
      setCursor(coluna,linha);
      print(txt);
    }

    void set_scroll(byte line, String message) {
      msg_scroll[line] = message;
      size_msg_scroll[line] = message.length();
    }
    void delete_scroll(byte line){
      msg_scroll[line].clear();
      size_msg_scroll[line] = 0;
      //position[line] = 0;
    }

    void update_scroll(byte line) {
      setCursor(0, line);
      if (size_msg_scroll[line] > 16) {
        //scroling de maneira continua
      /*
      ex: tamanho msg = 20 caracteres
      imprime do 
        0 ate o 16 
        1 ate o 17 
        2 ate o 18 
        3 ate o 19 
        4 ate o 20
        5 ate o 20(tamanho tela + indice = 21) + espaco em branco

        acabou os caracteres da string... e agora? comeca do zero 
        6 ate o 20(22) + espaco + 0 
        7 ate o 20(23) + espaco + 0 ao 1
        8 ate o 20(24) + espaco + 0 ao 2.... --> soma total = tamanho da tela
        ...
        15 ate o 20(31) + espaco + 0 ao 10
        ...
        19 ate o 20(35) + espaco + 0 ao 14
        0 ao 16
        1 ao 17
        */
        #define tamanho_display 16
        byte posicao_inicial = position[line];
        byte posicao_final = position[line] + tamanho_display;
        String msg_atual = "";

        if(posicao_final > size_msg_scroll[line]+1){
          posicao_final = size_msg_scroll[line];// se for maior, entao bota o ultimo digito no ultimo caracter 
          msg_atual = msg_scroll[line].substring(posicao_inicial, posicao_final);

          //segunda parte da string ( a parte que está entrando novamente na tela)
          posicao_inicial = 0;
          posicao_final = (( position[line] + tamanho_display) - size_msg_scroll[line] - 1); //1 do espaço que foi adicionado
          //                14 + 16(30) - 28 = 2, 2 - 1 = 1 
          msg_atual = msg_atual + " " + msg_scroll[line].substring(posicao_inicial, posicao_final);
        } else{
          msg_atual = msg_scroll[line].substring(posicao_inicial, posicao_final);
        }
        
        //Serial.println(msg_atual);
        print(msg_atual);


        position[line]++;
        if (position[line] > size_msg_scroll[line]) {
            // Quando chegamos ao final da string e além, reiniciamos a posição para 0
            position[line] = 0;
        }
      }else{
        byte completar_espacos = tamanho_display - msg_scroll[line].length();
        String resultado = "";
        for (int i = 0; i < completar_espacos; i++) {
            resultado += " ";
        }
        print(msg_scroll[line]+resultado);
      }
    }
};
