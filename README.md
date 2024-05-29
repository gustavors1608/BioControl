# BioControl 🌿

O BioControl é um sistema avançado de automação para fazendas verticais, projetado para criar um ambiente otimizado para o cultivo de plantas. Utilizando tecnologia de ponta como o ESP32 e sensores diversos, o sistema gerencia temperatura, umidade, aquaponia, iluminação LED e irrigação de maneira automatizada e precisa. Com integração futura planejada para APIs externas, o BioControl Nexus é ideal para ambientes educacionais e de pesquisa (utilizado e projetado para a fazenda vertical da Fasa - Faculdade Santo Ângelo), demonstrando como a tecnologia pode facilitar práticas agrícolas inovadoras.

## Descrição 📝

Este projeto visa criar um ambiente controlado para plantas, gerenciando fatores como temperatura, umidade, aquaponia, iluminação e irrigação. O sistema é especialmente útil para ambientes educacionais, permitindo que alunos e pesquisadores vejam em ação as possibilidades de cultivo em um ambiente isolado do meio externo.

## Funcionalidades ⚙️


- **🌡️ Monitoramento e Controle de Temperatura:**

  - Ajusta a temperatura do ar usando exaustores e ar condicionado, mantendo-a dentro de um intervalo ideal.
  - Se a temperatura exceder o limite máximo, ativa os exaustores.
  - Se o tempo limite para atingir a temperatura ideal for excedido, ativa o ar condicionado.
  - Também ativa o ar condicionado para manter a temperatura dentro da faixa mínima.
- **💧 Controle de Umidade:**

  - Monitora e controla a umidade do ar, mantendo-a dentro dos níveis ideais.
  - Se a umidade estiver acima do limite, o sistema ativa o ar condicionado no modo desumidificação.
- **🐟 Sistema de Aquaponia:**

  - Gerencia as bombas de água, alternando entre duas bombas caso uma falhe, garantindo o funcionamento do sistema de aquaponia.
  - Utiliza sensores de nível para manter o nível da caixa de água, acionando um solenoide para reabastecer o nível caso necessário.
- **🌱 Irrigação Automatizada:**

  - Controla solenoides para irrigar as plantas em intervalos regulares, com a frequência definida pelo usuário.
- **💡 Iluminação LED:**

  - Controla LEDs de cultivo e refletores com base em horários pré-definidos, permitindo a simulação de ciclos de luz.
- **🖥️ Display LCD 16x2 I2C:**

  - Exibe dados de temperatura, umidade e informações sobre o status do sistema, como "ligando exaustores" ou "ligando LEDs no modo apresentação".
- ## 🎤 Integração com Alexa:

  - Permite comandos de voz para modos de apresentação e controle individual de refletores.

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
   
2. Abra o projeto no VSCode com o PlatformIO instalado.

3. Conecte os componentes de hardware conforme descrito no diagrama.

4. Compile e carregue o código no ESP32.

## Uso 🚀
- Configure os parâmetros no código conforme suas necessidades (temperaturas ideais, horários de iluminação, etc.).
- Utilize o display LCD para monitorar o status do sistema.
- Comandos de voz via Alexa podem ser configurados no app da mesma para modos de apresentação e controle de refletores de modo individual.

## Próximos Passos ⏭️

- Integração com API externa para monitoramento e controle remoto.
- Adicionar comandos de voz via Alexa para controle de modos e funcionalidades.
- Aprimorar a interface do LCD com barra de progresso para a bomba de água.
- Criar diagrama de hardware detalhado.

## Contribuições 🤝
Contribuições são bem-vindas! Por favor, faça um fork do repositório e envie um pull request com suas melhorias.

## Licença 📄
Este projeto está licenciado sob a Licença MIT. Veja o arquivo LICENSE para mais detalhes.

## Diagramas e Esquemas 🖼️
Em breve ...

## Contato 📧
Para mais informações, entre em contato com EcoNext@sejafasa.com.br
