# Impressão 3D do Braço Robótico

Este projeto iniciou-se com a utilização de um **braço robótico parcialmente montado**, que desde as primeiras etapas apresentou **limitações estruturais e funcionais relevantes**. O modelo inicial possuía **5 graus de liberdade (DOF)** e era acionado por **7 servomotores**.

Com o avanço do desenvolvimento do código e a realização de testes experimentais, constatou-se que essas limitações físicas **comprometiam a confiabilidade do sistema**, além de dificultarem a evolução segura e consistente do projeto.

Diante desse cenário — e considerando a **experiência limitada da equipe no desenvolvimento e modelagem de componentes 3D** — optou-se pela **substituição do braço original** por um modelo já consolidado e amplamente utilizado em projetos públicos.

O novo braço robótico selecionado também possui **5 DOF**, porém utiliza **6 servomotores**, apresentando uma **arquitetura mecânica mais simples, robusta e confiável**, alinhada aos requisitos técnicos e aos objetivos do projeto.

A partir dos arquivos `.stl` disponibilizados pelo projeto escolhido, todas as peças foram impressas utilizando as **quatro impressoras 3D disponíveis no laboratório LPAE**, resultando em uma estrutura física **mais estável, precisa e adequada** para a continuidade do projeto.

---

## Visualização dos Modelos

Os modelos 3D utilizados na construção do novo braço robótico estão disponíveis no formato `.stl` e podem ser acessados diretamente no diretório abaixo:

**Diretório dos modelos 3D**  
https://github.com/CaioMeira/Projeto-Integrador-3/tree/main/Imagens/3D

---

##  Componentes do Braço Robótico

O novo braço robótico é composto por **10 modelos distintos**, totalizando a impressão de **14 peças**, conforme descrito na tabela a seguir:

| Arquivo `.stl`     | Quantidade | Função |
|--------------------|:------------:|--------|
| `Arm01.stl`        | 1 | Segmento principal do braço |
| `Arm 02 v3.stl`    | 1 | Segundo segmento articulado |
| `Arm 03.stl`       | 1 | Terceiro segmento do braço |
| `Base.stl`         | 1 | Base estrutural do braço robótico |
| `Waist.stl`        | 1 | Estrutura de rotação do eixo principal |
| `gear1.stl`        | 1 | Engrenagem garra |
| `gear2.stl`        | 1 | Engrenagem garra |
| `Gripper base.stl` | 1 | Base do mecanismo da garra |
| `Gripper 1.stl`    | 2 | Garras de preensão |
| `grip link 1.stl`  | 4 | Elos do mecanismo da garra |

**Total de peças impressas:** **14 unidades**

 O conjunto dessas peças resulta em um **braço robótico completo, funcional e mecanicamente estável**.

---

##  Tecnologias Utilizadas

- Impressão 3D (arquivos `.stl`)
- Servomotores
- Modelos mecânicos open-source
- Infraestrutura do laboratório **LPAE**
