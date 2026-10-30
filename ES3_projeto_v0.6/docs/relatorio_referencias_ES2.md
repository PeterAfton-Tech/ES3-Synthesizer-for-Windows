# Relatório de análise das gravações de referência do ES2

Material: 7 arquivos FLAC (48 kHz, estéreo), extraídos e cortados pelo autor de vídeos do YouTube: Sine, Triangle, Saw, Square, Pulse, Noise, Melody.
**Nenhum áudio dessas gravações foi usado para gerar som no ES3.** Elas serviram só como referência de análise. Nada aqui é medição oficial do ES2.

## 1. Estado do material (por que as medições são limitadas)
- Observado: os arquivos têm trechos de silêncio digital exato (zeros) inseridos na edição, e a maioria dos eventos de início/fim de nota fica colada a esses cortes.
- Observado: antes dos inícios de nota aparece uma subida lenta de -60 para -50 dB durante 5 a 10 ms. Isso é típico de pré-eco de codec, e foi tratado como artefato.
- Observado: acima de ~16 kHz o espectro cai para -70 dB ou menos (corte do codec). Não dá para ver aliasing nem harmônicas acima disso.
- Observado: o nível muda entre trechos (de -8 a -20 dBFS), e filtros, efeitos e ajustes do patch são desconhecidos. As gravações são praticamente mono (correlação L/R ≥ 0,9997), exceto o ruído (0,97).
- Consequência: só foram analisados trechos sustentados e estáveis (0,5 a 2,3 s), longe de ataques e cortes.

## 2. Identificação por arquivo
| Arquivo | Resultado | Confiança |
|---|---|---|
| Sine | Seno puro: 2ª harmônica ≤ -65 dB. 3ª/5ª harmônicas: -67/-75 dB em -18 dBFS, mas -38/-46 dB no trecho a -7 dBFS. Isso cresce com o nível, então vem de saturação posterior, não do oscilador | Alta (forma), média (origem da distorção) |
| Triangle | Série ímpar 1/n²: 3ª = -18,9 dB (ideal -19,1), 5ª = -28,3 (-28,0). Pares ≤ -63 dB | Alta |
| Saw | Todas as harmônicas ~1/n (2ª = -6,6 dB vs -6,0 ideal; até 24ª dentro de ±2 dB). O ciclo médio é uma rampa descendente | Alta (espectro); polaridade não confirmada |
| Square | Série ímpar 1/n: 3ª = -9,2 dB (ideal -9,5), 5ª = -14,3 (-14,0). Pares ≤ -57 dB, ou seja, 50% de duty | Alta |
| Pulse | **Não identificada com segurança.** Há nulos nas harmônicas 6, 12, 18, 24 e quedas fortes em 9, 15, 21, mas a 3ª harmônica é fraca e a fundamental não segue nenhum duty simples (melhor ajuste: erro de 30 dB²). Provavelmente há PWM, filtro ou mistura de osciladores no patch | Baixa |
| Noise | Ruído branco **gaussiano** (curtose 3,01; uniforme seria 1,8). Espectro plano ±3 dB de 200 Hz a 6 kHz (inclinação +0,85 dB/oitava) | Alta (distribuição), média (cor) |
| Melody | É um patch com filtro e efeitos, não uma forma de onda isolada. Usado só para ver o comportamento de início de nota | — |

## 3. Observado (medido nas gravações)
- Afinação: C3 medido em 130,8123 a 130,8134 Hz, ou seja, A4 = 440 Hz com 0,0 cents de desvio (sem drift aparente).
- Harmônicas baixas de Triangle e Square batem com as formas ideais em ±0,3 dB.
- Nos trechos de 130 Hz de Triangle e Square, as harmônicas altas ficam abaixo do ideal: -2 dB em 916 Hz, -8 dB em 1,7 kHz, ~-16 dB em 2,7 kHz (parecido com um passa-baixa de 2ª ordem em ~1,07 kHz). O Square a 261 Hz não mostra isso (cerca de -1 dB em tudo).
- Início de nota: nos 3 inícios que ficam dentro de áudio contínuo (Sine em 1,442 s, 1,740 s e 7,585 s), a amplitude chega ao nível pleno em ≤ 0,5 ms. No de 7,585 s, as amostras sobem de 0,014 → 0,089 → 0,127 (~2-3 amostras).
- Fim de nota: no trecho de Sine em 7,564 s o sinal cai de 0,107 para 0,006 em 3 amostras (~60 µs), no meio da onda, sem fade.
- Melody: todos os inícios de nota saltam mais de 25 dB em menos de 0,5 ms.
- Aliasing: o contraste entre vales e picos espectrais de Saw e Square a 261,6 Hz fica entre o modelo PolyBLEP parcial (aliasing=0,5) e o modelo ingênuo. Para o Square, 8-12 kHz bate com o modelo 0,5 (-31,2 vs -31,7 dB). Para o Saw, o resultado fica mais sujo que o ingênuo, então há ruído ou modulação misturados.

## 4. Inferido (plausível, não confirmado)
- O ES2 tem ataque mínimo e release mínimo praticamente instantâneos, e o release mínimo é um corte seco (com clique), como você descreveu. As evidências são poucas (4 eventos) e parte dos cortes pode ser da edição.
- As formas Sine, Triangle, Saw e Square são ideais, com Osc em 50% de duty no Square.
- O ruído do Osc 3 é gaussiano.
- O aliasing do ES2 é parcial: não é PolyBLEP completo e provavelmente não é totalmente ingênuo. O valor 0,5 é compatível com o que foi visto, mas não é medido.
- A queda nas harmônicas altas dos trechos de 130 Hz vem de filtro ou cadeia do patch, não do oscilador. Por isso não foi incorporada às formas de onda.

## 5. Sem confirmação
Forma exata da onda Pulse e da largura de pulso; polaridade da Saw; origem da saturação do Sine em alto nível; curva exata dos envelopes (só os mínimos foram observados); lei de mix do triângulo; cor do ruído em outros ajustes; as 100 Digiwaves; aliasing em frequências agudas (o codec corta acima de 16 kHz); qualquer efeito de Analog/CBD.

## 6. O que mudou no ES3 por causa das gravações
1. Ruído do Osc 3 passou de uniforme para gaussiano (curtose medida no teste: 2,96).
2. Envelope: Attack = 0 sobe em ~50 µs e Release = 0 corta em ~60 µs (2-3 amostras), sem fade de proteção. Ambos os parâmetros agora aceitam 0. Testado a 44,1, 48 e 96 kHz.
3. Mantido: aliasing de época = 0,5, agora com a justificativa acima. Ainda é provisório.
4. Não alterado, por falta de evidência: forma Pulse, Triangle, Saw, Square, Sine (já estavam compatíveis).
