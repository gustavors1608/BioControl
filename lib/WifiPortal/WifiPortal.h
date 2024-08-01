#ifndef WIFIPORTAL_H
#define WIFIPORTAL_H

#include "Arduino.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include "FS.h"
#include "SPIFFS.h"

// Definições de parâmetros e caminhos
#define porcentagem_minima_sinal 15 //Sinal mínimo em porcentagem para aparecer na pesquisa de WiFis
#define PARAM_INPUT_1 "ssid"
#define PARAM_INPUT_2 "pass"
#define PARAM_INPUT_3 "e"
#define ssidPath "/ssid.txt"
#define passPath "/pass.txt"
#define userPath "/user.txt"
#define reconectPath "/recon.txt"
#define html_home "/index.html"
#define timeout_conection_limit 5000 // Tempo máximo para conectar ao WiFi como STATION
#define get_email_user false

class WifiPortal {
    private:
        String criar_div_wifi(const char* nome_wifi, unsigned char potencia_porcentagem, const char* potencia_simbolo, unsigned char id, short rssi);
        String criar_pagina_wifi(String divs_wifi);
        String criar_pagina_user();
        void config_server_ap();
        void abrir_portal();
        unsigned char converter_rssi_porcentagem(short rssi);
        void montar_captive_wifi();
        bool connect_wifi_sta();
        byte scan_wifis();
        byte vrf_counter_reconect();
        void increment_counter_reconect();

        unsigned short time_ms_callback_config = 300;
        void (*_callback)(void) = nullptr;

    public:
        WifiPortal(); 
        ~WifiPortal();
        void connect();
        void reset_data();
        void set_config_callback(void (*name_callback)(void), unsigned short ms_interval_call = 100);
        bool vrf_conexao();
        #if get_email_user ==true
            String get_email();
        #endif
};






//site----------------------------------------------------------------------------------------------------------------------

#define nome_projeto "BioControl"
#define cor_fundo_projeto  " #595858 "
#define cor_letras_projeto " #d8d8d8 "
#define cor_destaque_projeto " #b521ff "
#define link_site " www.example.com " //indicado ao user para acessar para controlar o device após inserir a senha
#define nome_rede_ap "WifiPortal ESP32"

const char html_inicio[] PROGMEM = "<!DOCTYPE html><html lang=pt-br><meta charset=UTF-8><meta content=\"width=device-width,initial-scale=1,user-scalable=no\"name=viewport><title>Wifi Portal</title><style>:root{--letras:" cor_letras_projeto ";--destaques:" cor_destaque_projeto ";--background:" cor_fundo_projeto ";--logo:2.3em;--titulo:1.6em;--texto:1em}body,header{text-align:center;font-size:var(--texto);font-family:ubuntu,sans-serif,Roboto;margin:0;padding:0;background-color:var(--background);height:100%;width:100%}div{padding:5px;margin:6px 0;box-sizing:border-box}.submit,a,body,header{color:var(--letras);text-decoration:none}.wifi_name{border-bottom:1px solid rgba(0,0,0,.1)}.q{height:16px;margin:0;padding:0 5px;text-align:right;min-width:38px;float:right}.q.q-1:after{background-position-x:-16px}.q.q-2:after{background-position-x:-32px}.q.q-3:after{background-position-x:-48px}.q.q-4:after{background-position-x:-64px}.q:after,.q:before{content:'';width:16px;height:16px;display:inline-block;background-repeat:no-repeat;background-position:16px 0;background-image:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGAAAAAQCAMAAADeZIrLAAAAJFBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADHJj5lAAAAC3RSTlMAIjN3iJmqu8zd7vF8pzcAAABsSURBVHja7Y1BCsAwCASNSVo3/v+/BUEiXnIoXkoX5jAQMxTHzK9cVSnvDxwD8bFx8PhZ9q8FmghXBhqA1faxk92PsxvRc2CCCFdhQCbRkLoAQ3q/wWUBqG35ZxtVzW4Ed6LngPyBU2CobdIDQ5oPWI5nCUwAAAAASUVORK5CYII=)}svg{margin:.5em .5em -.35em .5em;vertical-align:text-bottom}.back_logo{width:42px;height:42px;border-radius:50%}.back_logo,.input_div input{background-color:var(--letras)}.logo_txt{font-size:var(--logo)}.submit,header,hr,input{border:none;box-shadow:0 2px 4px rgba(0,0,0,.5)}.input_div input{padding:1em;border-radius:.3rem;width:100%;margin:2px 0;box-sizing:border-box}hr{height:1px;opacity:30%;background-color:var(--letras)}.submit,h2{font-size:var(--titulo)}.submit{position:fixed;bottom:1%;right:0;padding:.8em;width:95%;margin:0 2.5% 0 2.5%;background-color:var(--destaques)}.wrap{text-align:left;width:min(400px);margin:auto}.popup{display:none;position:fixed;top:50%;left:50%;transform:translate(-50%,-50%);padding:20px;background-color:var(--letras);color:var(--background);box-shadow:0 0 10px rgba(0,0,0,.5);z-index:1000;text-align:left}#overlay{display:none;position:fixed;top:0;left:0;width:100%;height:100%;background-color:rgba(0,0,0,.5);z-index:500}</style><header><span class=logo_txt>"nome_projeto"</span><hr></header><div class=wrap><div id=overlay onclick=hp()></div><h2>Configurações</h2><hr>";

const char html_div_wifi[] PROGMEM = "<div class=\"wifi_name\"><a onclick=\"c(this)\" href='#p'><span>{w_n}</span><div role='img' class='q q-{w_c}' onclick=\"sp('{w_id}')\"> <div id=\"{w_id}\" class=\"popup\"><p>Intensidade: {w_p}%</p><p>Rssi: {w_rssi}Db</p><p>Ssid: '{w_n}'</p></div> </div></a></div>";


const char html_form_wifi[] PROGMEM = "<br><form action='wifisave'><div class='input_div'><label for='s'>Nome da rede Wi-Fi</label><br> <input id='s' length='32' name='ssid' placeholder='Digite ou clique no seu Wi-Fi acima' required></div><div class='input_div'><label for='p'>Senha do Wi-Fi</label><input id='p' length='64' name='pass' placeholder='Digite a senha do seu Wi-Fi' required></div>";
#if get_email_user == true
const char html_form_user[] PROGMEM = "<br><hr><br><h2>Sua conta</h2>Nos informe o email que poderá controlar esse dispositivo.<div class='input_div'><label for='e'>Email da sua conta</label><br><input id='e' length='255' name='e' placeholder='exemplo@email.com' required type='email'></div>";
const char html_sucesso_user[] PROGMEM = "<h2>E seu email foi salvo, em até 10 minutos deve ser possivel controlar em nosso site: " link_site " </h2><h2>Se não for possivel, configure novamente e verifique se digitou corretamente o seu email, se o problema persistir entre em contato com nosso suporte.</h2>";
#endif

const char html_final[] PROGMEM = "<br><br><br><br><br><br><br><br><br><button class=submit type=submit>Salvar Alterações</button></div><script>function c(e) {var paragraph = e.querySelector('span');document.getElementById('s').value = paragraph ? paragraph.textContent : '';document.getElementById('p').focus();}let cpop = null;function sp(e) {if (cpop) {document.getElementById(cpop).style.display = 'none';}cpop = e;document.getElementById(e).style.display = 'block';document.getElementById('overlay').style.display = 'block';}function hp() {if (cpop) {document.getElementById(cpop).style.display = 'none';cpop = null;}document.getElementById('overlay').style.display = 'none';}</script></body></html>";

const char html_sucesso_wifi[] PROGMEM = "<h1>Dados salvos com sucesso</h1><hr><h2>O dispositivo está tentando se conectar ao Wi-Fi. <br>Se ocorrer algum erro, como: senha ou nome da rede incorretos, este Wi-Fi de configuração ficará disponivel para você tentar novamente.</h2>";

#define T_wifi_name "{w_n}"  // @token wifi_n nome
#define T_wifi_class "{w_c}" // @token wifi_c classe
#define T_wifi_number "{w_p}" // @token wifi_p potencia
#define T_wifi_rssi "{w_rssi}" // @token wifi_rssi rssi
#define T_wifi_id "{w_id}" // @token wifi_id id











#endif // WIFIPORTAL_H
