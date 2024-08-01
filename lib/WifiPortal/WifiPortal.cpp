#include "WifiPortal.h"




#define log(x) Serial.println("WPortal: " + String(x))
#define FORMATAR_SPIFFS_SE_FALHAR_MONTAGEM false

// Função para inicializar o SPIFFS
void init_spiffs() {
    if (!SPIFFS.begin(FORMATAR_SPIFFS_SE_FALHAR_MONTAGEM)) { // Tenta montar o SPIFFS e formata se necessário
        log(F("Falha ao montar o SPIFFS. Tentando formatar..."));
        if (!SPIFFS.begin(true)) {
            log(F("Falha ao formatar o SPIFFS."));
            return;
        }
        log(F("SPIFFS formatado com sucesso."));
    } else {
        log(F("SPIFFS montado com sucesso."));
    }
}

// Função para encerrar o SPIFFS
void end_spiffs() {
    SPIFFS.end();
}

// Função para ler um arquivo do SPIFFS
String read_spiffs(const char *path_file) {
    File file = SPIFFS.open(path_file, "r");
    if (!file || file.isDirectory()) {
        //NAO ENCONTRADO O ARQUIVO
        return "";
    }

    String fileContent;
    while (file.available()) {
        fileContent = file.readStringUntil('\n');
        break;     
    }
    file.close();
    return fileContent;
}

// Função para escrever em um arquivo do SPIFFS
void write_spiffs(const char *path, const char *message) {
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) {
        return;
    }
    if (file.print(message)) {
        //SUCESSO
    } else {
        //ERRO AO ESCREVER
    }
    file.close();
}

// Função para remover um arquivo do SPIFFS
void remove_spiffs(const char *path) {
    if (!SPIFFS.exists(path)) {
        //ARQUIVO NAO EXISTE
        return;
    }
    SPIFFS.remove(path);
}

// Classe Timeout para gerenciamento de tempo limite
class Timeout {
  private:
    uint32_t previousMillis;
    uint32_t limite_timeout;

  public:
    Timeout(const unsigned long limite_timeout_arg) {
        this->previousMillis = millis();
        this->limite_timeout = limite_timeout_arg;
    }

    ~Timeout() {
        previousMillis = 0;
        limite_timeout = 0;
    }

    // Função para zerar a contagem de tempo
    void zerar_contagem() {
        this->previousMillis = millis();
    }

    // Função para verificar se o tempo limite foi atingido
    bool Verify() {
        return (millis() - this->previousMillis < limite_timeout);
    }
};

// Declaração dos objetos
AsyncWebServer server(80);

// Construtor da classe WifiPortal
WifiPortal::WifiPortal(/* args */) {
    
}

// Destrutor da classe WifiPortal
WifiPortal::~WifiPortal() {
    server.end();
    end_spiffs();
}

// Função para resetar dados
void WifiPortal::reset_data() {
    remove_spiffs(ssidPath);
    remove_spiffs(passPath);
    remove_spiffs(reconectPath);
}

// Função para criar uma div com o nome e potência do sinal Wi-Fi
String WifiPortal::criar_div_wifi(const char* nome_wifi, unsigned char potencia_porcentagem, const char* potencia_simbolo, unsigned char id, short rssi) {
    String div_wifi = FPSTR(html_div_wifi);
    div_wifi.replace(FPSTR(T_wifi_name), nome_wifi);
    div_wifi.replace(FPSTR(T_wifi_number), String(potencia_porcentagem));
    div_wifi.replace(T_wifi_class, potencia_simbolo);
    div_wifi.replace(T_wifi_rssi,String(rssi));
    div_wifi.replace(T_wifi_id, String(id));
    return div_wifi;
}

// Função para criar a página de configuração do Wi-Fi
String WifiPortal::criar_pagina_wifi(String divs_wifi) { 
    String page = FPSTR(html_inicio);
    page += divs_wifi;
    divs_wifi.clear();
    page += FPSTR(html_form_wifi);

    #if get_email_user == true
        page += FPSTR(html_form_user);
    #endif

    page += FPSTR(html_final);
    return page;
}
#if get_email_user ==true
    String WifiPortal::get_email(){
        return read_spiffs(userPath);
    }
#endif

// Função para verificar a conexão com a internet
bool WifiPortal::vrf_conexao() {
    return WiFi.isConnected();
}

// Função para conectar ao Wi-Fi em modo estação
bool WifiPortal::connect_wifi_sta() {     
        String ssid = read_spiffs(ssidPath); 
        log("conectando ao wifi: " + ssid);

        if (ssid == "") {
            log(F("SSID não encontrado"));
            return false;
        }

        // Desliga o Wi-Fi para evitar conflitos
        WiFi.disconnect();
        //WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);

        // Configura o tipo de rede
        IPAddress localIP(192, 168, 1, 50);
        IPAddress localGateway(192, 168, 1, 254); 
        IPAddress subnet(255, 255, 255, 0);

        // Tenta se conectar
        String pass = read_spiffs(passPath);
        if (pass != "") {
            WiFi.begin(ssid.c_str(), pass.c_str());
        } else {
            WiFi.begin(ssid.c_str(), NULL, 0, NULL, true);
        }

        if (!WiFi.config(localIP, localGateway, subnet, IPAddress(1, 1, 1, 1), IPAddress(8, 8, 8, 8))) {
            return false;
        }

        // Configura um timeout para a conexão Wi-Fi
        Timeout timeout_connect_wifi = Timeout(timeout_conection_limit); 

        while (WiFi.status() != WL_CONNECTED) {
            yield();
            if (!timeout_connect_wifi.Verify()) {
                log(F("Tempo limite atingido STA"));
                return false;
            }
            delay(200);
        }
        log(F("Conectado com sucesso"));
        return true;
}

// Função para configurar o servidor em modo ponto de acesso (AP)
void WifiPortal::config_server_ap() {
    // Configura o servidor do captive portal
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->redirect(F("/"));
    });

    // Tela inicial
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        static String html_exibir_user = "";
        if (html_exibir_user == "") {
            html_exibir_user = read_spiffs(html_home);
        }
        request->send_P(200, "text/html", html_exibir_user.c_str());
    });

    // Salvar configuração de Wi-Fi
    server.on("/wifisave", HTTP_GET, [] (AsyncWebServerRequest *request) {
        if (request->hasParam(PARAM_INPUT_1)) {
            const char *ssid = request->getParam(PARAM_INPUT_1)->value().c_str();
            write_spiffs(ssidPath, ssid);
        }
        if (request->hasParam(PARAM_INPUT_2)) {
            const char *pass = request->getParam(PARAM_INPUT_2)->value().c_str();
            write_spiffs(passPath, pass);
        }

        #if get_email_user == true
            if (request->hasParam(PARAM_INPUT_3)) {
                const char *email = request->getParam(PARAM_INPUT_3)->value().c_str();
                write_spiffs(userPath, email);
            }
        #endif
           
        String response = FPSTR(html_sucesso_wifi);

        // Adiciona a string de sucesso do usuário se necessário
        #if(get_email_user == true)
            response += FPSTR(html_sucesso_user);
        #endif

        request->send(200, "text/html", response);

        response.clear();
        
        delay(5000);
        ESP.restart();
    });


}

// Função para converter o RSSI para porcentagem
unsigned char WifiPortal::converter_rssi_porcentagem(short rssi) {
    unsigned char porcentagem = map(rssi, -100, -50, 0, 100);
    return (porcentagem > 100) ? 100 : porcentagem;
}

// Função para escanear redes Wi-Fi disponíveis
byte WifiPortal::scan_wifis() {    
    byte numero_redes = WiFi.scanNetworks();
    if (numero_redes == -1) {
        log(F("redes não encontradas"));
    }
    return numero_redes;
}

// Função para montar o captive portal com as redes Wi-Fi disponíveis
void WifiPortal::montar_captive_wifi() {
    String divs_wifis = "";

    for (int id_rede = 0; id_rede < this->scan_wifis(); id_rede++) {
        int rssi = WiFi.RSSI(id_rede);
        unsigned char porcentagem_sinal = this->converter_rssi_porcentagem(rssi);

        if (porcentagem_sinal > porcentagem_minima_sinal) {
            divs_wifis.concat(
                this->criar_div_wifi(WiFi.SSID(id_rede).c_str(), porcentagem_sinal, String(map(porcentagem_sinal, 0, 100, 1, 4)).c_str(),id_rede,rssi)
            );  
        }
        yield();
    }
    write_spiffs(html_home, (this->criar_pagina_wifi(divs_wifis).c_str()));

    divs_wifis.clear();
}

byte WifiPortal::vrf_counter_reconect(){
    return (read_spiffs(reconectPath).toInt() > 255)?255:read_spiffs(reconectPath).toInt();
}

void WifiPortal::increment_counter_reconect(){
    byte counter = this->vrf_counter_reconect();
    counter++;

    // Converte o valor de counter para uma string
    char buffer[4]; // Buffer grande o suficiente para armazenar um valor de 0 a 255
    itoa(counter, buffer, 10);

    // Escreve o valor convertido para o arquivo
    write_spiffs(reconectPath, buffer);

}

// Função para tentar conectar e, se falhar, abrir um captive portal para configuração
void WifiPortal::connect() {
    init_spiffs();

    String ssid = read_spiffs(ssidPath);
    if (ssid == "") {
        // Abre o AP para configuração
        this->abrir_portal();  
        return;
    }

    // Verifica se já se conectou a essa rede anteriormente
    if (vrf_counter_reconect() > 0) {
        while (true) {
            if (this->connect_wifi_sta() == true) {
                // Conectou: Incrementa o valor do contador de reconexões
                this->increment_counter_reconect();
                break;
            } else {
                // Não conectou: Reinicia e tenta novamente(tenta infinitamente até ir pois ja se conectou nela outras vezes)
                delay(10000);
                ESP.restart();
            }
        }
    } else {
        // Tenta conectar uma vez
        if (this->connect_wifi_sta()) {
            // Conectou: Prossegue normalmente
            this->increment_counter_reconect();
        } else {
            // Não conectou: Abre o AP novamente
            this->abrir_portal();  
        }
    }
}


/*
Caso 1: Senha incorreta ou ssid incorreto
Verifica se já se conectou a essa rede anteriormente:
Sim: Tenta reconectar repetidamente.
    Conectou: Incrementa o valor do contador de reconexões e salva no arquivo.
    nao conectou? reinicia e tenta denovo até conseguir
Não: Tenta conectar uma vez.
    Conectou: Prossegue normalmente.
    Não conectou: Abre o AP novamente.


Caso 2: Nenhum SSID salvo
    Abre o AP para configuração.
        Usuário insere SSID e senha.
        Tenta conectar com as credenciais fornecidas:
        reinicia e tenta se conectar com essas infos

Caso 3: Conexão bem-sucedida
    Verifica se as credenciais estão salvas.
        Sim: Tenta conectar com as credenciais salvas.
            Conectou: Incrementa o contador de reconexões.
            Não conectou: entra no caso 1.
        Não: Abre o AP para configuração.

Caso 4: Rede fora do alcance
    Verifica se as credenciais estão salvas.
        Sim: Tenta conectar repetidamente por um período.
            Conectou: Prossegue normalmente.
            Não conectou: caso 1
        Não: Abre o AP para configuração.

Caso 5: Mudança de rede
    Usuário decide mudar para uma nova rede.
    Apaga os dados da memoria e reinicia


*/

// Função para definir o callback de configuração
void WifiPortal::set_config_callback(void (*name_callback)(void), unsigned short ms_interval_call) {
    if (name_callback != nullptr) {
        this->_callback = name_callback;
        this->time_ms_callback_config = ms_interval_call;
    }
}

// Função para abrir o portal de configuração
void WifiPortal::abrir_portal() {
    this->reset_data();
    //WiFi.disconnect();
    //WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(nome_rede_ap, NULL);
    this->montar_captive_wifi();

    DNSServer dnsServer;
    dnsServer.start(53, "*", WiFi.softAPIP());

    this->config_server_ap();
    server.begin();
    log(F("abrindo portal"));

    uint32_t lastmillis = 0;

    while (true) {
        dnsServer.processNextRequest();         
        yield();

        if (millis() > (lastmillis + this->time_ms_callback_config)) {
            if (_callback != nullptr) {
                _callback();
            }
            lastmillis = millis();
        }
    }
}
