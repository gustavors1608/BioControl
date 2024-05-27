# BioControl 🌿

O BioControl é um sistema avançado de automação para fazendas verticais, projetado para criar um ambiente otimizado para o cultivo de plantas. Utilizando tecnologia de ponta como o ESP32 e sensores variados, o sistema gerencia temperatura, umidade, aquaponia, iluminação LED e irrigação de maneira automatizada e precisa. Com integração futura planejada para APIs externas, o BioControl Nexus é ideal para ambientes educacionais e de pesquisa, demonstrando como a tecnologia pode facilitar práticas agrícolas inovadoras.

## Descrição 📝

Este projeto visa criar um ambiente controlado para plantas, gerenciando fatores como temperatura, umidade, aquaponia, iluminação e irrigação. O sistema é especialmente útil para ambientes educacionais, permitindo que alunos e pesquisadores vejam em ação as possibilidades de cultivo em um ambiente isolado do meio externo.

## Funcionalidades ⚙️

- **🌡️ Monitoramento e Controle de Temperatura:** Ajusta automaticamente a temperatura do ar usando exaustores e ar condicionado.
- **💧 Controle de Umidade:** Mantém a umidade do ar dentro dos níveis ideais.
- **🐟 Sistema de Aquaponia:** Gerencia bombas de água e sensores de fluxo para garantir um ambiente saudável para os peixes da aquaponia.
- **🚿 Irrigação Automatizada:** Controla solenóides para irrigar as plantas em intervalos regulares.
- **💡 Iluminação LED:** Controla os LEDs de cultivo e refletores com base em horários pré-definidos.
- **🗣️ Integração com Alexa:** Permite comandos de voz para modos de apresentação e controle individual de refletores.

## Tecnologias Utilizadas 🛠️

- **Hardware:**
  - ESP32
  - Sensores de temperatura e umidade DHT11
  - Bomba de água e sensores de fluxo
  - Solenoides para manter o nível da caixa de água e para controle da irrigação
  - LEDs de cultivo e refletores acionados por relés
  - CI Expansor de portas 74HC595
  - LCD 16x2 I2C

- **Software:**
  - Arduino (C, C++)
  - PlatformIO no VSCode

## Instalação 📦

1. Clone o repositório:
   ```bash
   git clone https://github.com/gustavors1608/BioControl.git
   ´´´
2. Abra o projeto no VSCode com o PlatformIO instalado.

3. Conecte os componentes de hardware conforme descrito no diagrama.

4. Compile e carregue o código no ESP32.

## Uso 🚀
- Configure os parâmetros no código conforme suas necessidades (temperaturas ideais, horários de iluminação, etc.).
- Utilize o display LCD para monitorar o status do sistema.
- Comandos de voz via Alexa podem ser configurados no app da mesma para modos de apresentação e controle de refletores de modo individual.

## Contribuições 🤝
Contribuições são bem-vindas! Por favor, faça um fork do repositório e envie um pull request com suas melhorias.

## Licença 📄
Este projeto está licenciado sob a Licença MIT. Veja o arquivo LICENSE para mais detalhes.

## Diagramas e Esquemas 🖼️
Em breve ...

## Contato 📧
Para mais informações, entre em contato com [seu email].
