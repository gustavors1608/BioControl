## BioControl 🌿

O BioControl é um sistema avançado de automação para fazendas verticais, projetado para criar um ambiente otimizado para o cultivo de plantas. Utilizando tecnologia como o ESP32 e sensores diversos, o sistema gerencia temperatura, umidade, aquaponia, iluminação LED e irrigação de maneira automatizada e precisa. Desenvolvido e utilizado na fazenda vertical da FASA - Faculdade Santo Ângelo, o BioControl é ideal para ambientes educacionais e de pesquisa, demonstrando como a tecnologia pode facilitar práticas agrícolas inovadoras.

## Descrição 📝

Este projeto visa criar um ambiente controlado para plantas, gerenciando fatores como:

- Temperatura 🌡️
- Umidade 💧
- Aquaponia 🐟
- Iluminação 💡
- Irrigação 🌱

O sistema é especialmente útil para ambientes educacionais, permitindo que alunos e pesquisadores vejam em ação as possibilidades de cultivo em um ambiente isolado do meio externo, simulção de clima etc.

## Funcionalidades ⚙️

- **Controle de Temperatura e Umidade:**
    - Ajusta a temperatura e umidade do ar usando exaustores e ar condicionado (futuramente), mantendo-os dentro de intervalos ideais, levando em consideração também os dados climáticos externos. 
    - Ex: Em dias frios e úmidos, o sistema prioriza o aquecimento em detrimento da redução da umidade para proteger os peixes do sistema de aquaponia.
- **Sistema de Aquaponia:**
    - Gerencia automaticamente as bombas de água, alternando entre duas bombas para garantir o funcionamento contínuo do sistema de aquaponia, mesmo em caso de falha de uma das bombas.
- **Irrigação Automatizada:**
    - Irriga as plantas secundarias em intervalos regulares, com a frequência definida pelo usuário.
- **Iluminação LED Inteligente:**
    - Simula ciclos de luz natural, ligando e desligando os LEDs de cultivo e refletores em horários pré-definidos.
    - Permite ajustes finos na intensidade e espectro de luz para otimizar o crescimento das plantas (implementação futura).
- **Interface Intuitiva:**
    - Exibe dados de temperatura, umidade e informações sobre o status do sistema em um display LCD 16x2.
- **Integração com Alexa:**
    - Permite o controle por voz de dispositivos específicos, como:
        - Bomba d'água (ligar/desligar e modo automático)
        - Exaustores (ligar/desligar e modo automático)
        - Iluminação (ligar/desligar e modo automático)
        - Refletor central
        - Lâmpadas auxiliares

## Tecnologias Utilizadas 🛠️

**Hardware:**

- ESP32
- Sensor de temperatura e umidade DHT11
- Bombas de água
- Sensor de fluxo de água
- Solenoides
- LEDs de cultivo
- Refletores
- CI Expansor de portas 74HC595
- LCD 16x2 I2C

**Software:**

- Arduino (C, C++)
- PlatformIO
- Visual Studio Code
- Bibliotecas:
    - DHT
    - CronOut
    - fauxmoESP
    - WiFi
    - NTPClient
    - WiFiUdp
    - HTTPClient
    - Arduino_JSON
    - Thread
    - LCD (customizada)

## Instalação 📦

1. Clone o repositório:
   ```bash
   git clone https://github.com/gustavors1608/BioControl.git
   ```
2. Abra o projeto no Visual Studio Code com a extensão PlatformIO instalada.
3. Conecte os componentes de hardware conforme o esquema de ligação e configure os pinos correspondentes no código.
4. Compile e carregue o código para o ESP32.

## Uso 🚀

- Configure os parâmetros no código, como temperaturas ideais, horários de iluminação e frequência de irrigação.
- Utilize o display LCD para monitorar os dados e o estado do sistema.
- Controle as funcionalidades através de comandos de voz com a Alexa.

## Próximos Passos ⏭️

- retirar codigo inutilizado
- sistemas de segurança contra travamentos ou erros em cascata
- funções de simulacao de clima (alvos de temperatura e umidade, talvez PID)
- melhorar controle de exaustores, deixando funcao de controle de temp com funcionamento em harmonia com umidade local e externa

## Contribuições 🤝

Contribuições são bem-vindas! Faça um fork do repositório, implemente suas melhorias e envie um pull request.

## Licença 📄

Este projeto está licenciado sob a Licença MIT. Veja o arquivo LICENSE para mais detalhes.

## Diagramas e Esquemas 🖼️

Em breve...
