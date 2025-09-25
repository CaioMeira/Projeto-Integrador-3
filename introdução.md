# Projeto Integrador III - Braço Robótico

## Problema  
O Brasil ainda enfrenta grandes desafios relacionados ao desenvolvimento tecnológico e industrial.  
Apesar de possuir abundância de matéria-prima, grande parte dela é exportada para países que, por sua vez, a transformam em produtos de alto valor agregado e posteriormente os revendem ao mercado brasileiro.  

Esse ciclo resulta em uma dependência tecnológica significativa, altos custos para aquisição de equipamentos e máquinas industriais, além de reduzir a competitividade das indústrias nacionais.  

Nesse contexto, a automação e a robótica se apresentam como ferramentas fundamentais para fomentar o crescimento da indústria local. No entanto, a aquisição de sistemas robóticos modernos, como braços manipuladores industriais, ainda é extremamente onerosa para micro e pequenos empreendedores. Esse fator cria uma barreira de entrada para empresas emergentes que poderiam utilizar tais tecnologias para expandir suas capacidades produtivas e explorar novos mercados.  

O desenvolvimento de um protótipo de braço robótico de baixo custo, utilizando plataformas acessíveis de hardware e software livre, como ESP32 e ROS (Robot Operating System), pode contribuir de forma significativa para reduzir esse obstáculo.  
Tal solução poderia permitir que pequenos negócios implementem processos automatizados de forma mais barata, escalável e eficiente, fortalecendo a indústria nacional e, ao mesmo tempo, diminuindo a dependência externa de equipamentos de alto custo.  

---

## Motivação  
O presente projeto tem sua origem no Projeto Integrador II, no qual já foram trabalhados conceitos relacionados à automação de processos utilizando microcontroladores, sensores e atuadores. Durante essa etapa, foi possível identificar o potencial da automação na solução de problemas práticos e no aumento da eficiência de sistemas.  

Com base nessa experiência, no Projeto Integrador III, buscou-se unir os conceitos de automação com a área de robótica, resultando no desenvolvimento de um protótipo de braço robótico articulado com 5 DoF.  
Essa evolução foi motivada pela necessidade de aprofundar os conhecimentos adquiridos e explorar uma área estratégica para o futuro da indústria 4.0, permitindo também uma aplicação mais direta em problemas reais de manufatura e logística.  

Além disso, a escolha pela integração com o ROS e o simulador Gazebo foi motivada pela possibilidade de aproximar o projeto dos padrões utilizados em centros de pesquisa e indústrias avançadas, garantindo que o aprendizado e os resultados obtidos sejam compatíveis com o estado da arte em robótica.  

Dessa forma, o projeto não apenas consolida os conhecimentos adquiridos ao longo do curso, mas também busca gerar impacto social e econômico, tornando a tecnologia mais acessível e fortalecendo a competitividade das pequenas indústrias brasileiras.  

---

## Objetivo Geral  
Desenvolver um braço robótico de baixo custo, controlado por ESP32, integrado ao sistema ROS e ao simulador Gazebo, de forma a oferecer uma solução acessível para fins educacionais, experimentais e de apoio a micro e pequenos empreendedores.  

---

## Objetivos Específicos  
1. Projetar a arquitetura de hardware e software do braço robótico utilizando ESP32, driver PCA9685 e servomotores.  
2. Implementar a comunicação entre o ESP32 e o ROS via rede sem fio (Wi-Fi/Bluetooth).  
3. Desenvolver algoritmos de controle para movimentação dos graus de liberdade do braço robótico.  
4. Simular o funcionamento do protótipo no ambiente Gazebo, garantindo validação prévia antes da implementação física.  
5. Realizar a integração prática do sistema físico com o ROS, validando a correspondência entre o protótipo e o modelo simulado.  
6. Oferecer uma alternativa de baixo custo que possibilite a aplicação do braço robótico em contextos educacionais e em pequenas linhas de produção.

## Diagrama de blocos

![Diagrama_de_blocos_v1](https://github.com/CaioMeira/Projeto-Integrador-3/blob/main/Imagens/Diagrama%20de%20blocos/Diagrama%20de%20blocos%20v2.png)

