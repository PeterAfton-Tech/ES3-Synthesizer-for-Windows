# ES3 — Matriz de referência (v0.3)

Fontes consultadas: Apple Logic Pro User Guide / Instruments (help.apple.com, support.apple.com) e Sound On Sound (jun/2007), além do screenshot enviado.
Nada foi MEDIDO no ES2 real. "Documentado" = descrito nas fontes acima.

| Recurso | Evidência | ES3 hoje | Confiança |
|---|---|---|---|
| Osc 1: FM por Osc 2, sobre onda senoide | Documentado | Modulação de fase (profundidade provisória, usa Osc 2 da amostra anterior) | Parcial |
| Osc 2/3: sync rígido (quadrada/serra) a Osc 1 | Documentado | Reinício com precisão de sub-amostra; Osc 2 e 3 | Parcial |
| Anel (ring): Osc 1 x quadrada de Osc 2 | Documentado (só Osc 2; Osc 3 não tem) | Implementado só em Osc 2 | Parcial |
| Largura de pulso: de 50% até o pulso mais fino | Documentado | 50% a 2% (valor mínimo provisório) | Parcial |
| Osc 3: ruído branco por padrão, cor modulável | Documentado + gravação (gaussiano, curtose 3,01) | Gaussiano; cor = passa-baixa de 1 polo (provisório) | Parcial |
| Ataque/release mínimos do ADSR | Gravações (poucos eventos; pode haver cortes de edição) | ~50 µs / ~60 µs, sem fade | Parcial |
| Osc Start: free / soft / hard | Documentado (hard = nível mais alto; free = fase aleatória) | Fases iniciais provisórias | Parcial |
| 100 Digiwaves | Documentado | NÃO disponíveis; há uma onda provisória | Desconhecida |
| Coarse ±3 oitavas; fine em cents | Documentado | ±36 st / ±100 c | Verificada (faixa) |
| Triângulo de mix: crossfade; lado = 2 osciladores | Documentado | Pesos baricêntricos lineares | Parcial |
| Orientação dos vértices do triângulo | CONFLITO: SOS descreve topo=Osc1, esquerda=Osc2, base=Osc3; o screenshot mostra triângulo apontando para a direita | Osc1 (0,0), Osc2 (1,0.5), Osc3 (0,1) | Desconhecida |
| Aliasing de época | Gravações: entre PolyBLEP parcial e ingênuo (limitado pelo codec) | 0,5 | Parcial |
| Filtro 1: multimodo 12 dB/oit (Lo, Hi, Peak, BP, BR) | Documentado (Apple, SOS) | SVF de 2 polos; Peak até +12 dB (provisório) | Parcial |
| Filtro 2: passa-baixa 12/18/24 dB e Fat (24 dB com compensação de graves) | Documentado | SVF (12) e escada de 3/4 polos; Fat = ganho de entrada 1+k | Parcial |
| Roteamento série/paralelo, Blend -1..+1 (-1 só F1, +1 só F2, 50/50 em paralelo) | Documentado | Implementado; posição do 2º Drive em série é suposição | Parcial |
| Drive (um circuito em paralelo; independentes em série) | Documentado | tanh com compensação (curva provisória) | Parcial |
| Auto-oscilação de filtro (Flt Reset) | Documentado | Filtro 2 de 18/24 dB oscila em res=1 (testado, 1000 Hz) | Parcial |
| FM de filtro, Cut 1+2 encadeados | Visto no screenshot/doc | NÃO implementado | — |
| ENV2 = envelope do filtro (ligação pré-definida no Router) | Documentado | ENV2->Cut fixo provisório (±8 oitavas) até o Router | Parcial |
| LFO 1 polifônico, LFO 2 monofônico, 7 formas, fade-in | Documentado (parcial) | Classe pronta e testada; sem destino até o Router | Parcial |
| Curvas de cutoff/ressonância (mapeamento dos knobs) | Sem medição | Hz logarítmico e res linear (provisório) | Desconhecida |
| Router, ENV1, vector, efeitos, presets | Partes 5 a 8 | Não implementado | — |
