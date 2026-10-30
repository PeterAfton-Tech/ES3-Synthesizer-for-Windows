# ES3 (v0.6)
Sintetizador VST3 e Standalone para Windows, recriação fiel do clássico ES2 (Logic Pro).
Guia passo a passo para compilar e instalar: [`docs/COMO_OBTER_O_PLUGIN.md`](docs/COMO_OBTER_O_PLUGIN.md)

## Estado do Projeto (v0.6)
- **Interface Gráfica Completa (Parte 7)**:
  - Visual autêntico em alumínio escovado e titânio chumbo.
  - Knobs rotativos de alumínio com arcos luminosos laranja vintage (`#FF8C00`).
  - Sliders verticais metálicos para os Envelopes e LFOs.
  - Triângulo de mixagem baricêntrico 2D com retículo interativo apontando para a direita.
  - Seção central circular com display duplo dos filtros.
  - Matriz LCD azul-ciano com 10 slots de modulação no Router.
  - Pad XY bidimensional e seletor de presets no topo.
  - Redimensionável com proporção travada em 1024x680, duplo-clique para reset e roda do mouse habilitada.
- **Motor Sonoro e Efeitos (Partes 1 a 6)**:
  - 3 osciladores analógicos virtuais com formas clássicas, PW contínuo, Hard Sync, Ring Mod, FM e ruído calibrado.
  - Aliasing de época ajustável e drift analógico orgânico por voz (`Analog`).
  - Filtro 1 (Lo/Hi/Peak/BP/BR) e Filtro 2 (12/18/24 dB e modo Fat preservando subgraves), roteamento série/paralelo com Blend e Drive.
  - Efeitos embutidos: Distorção (Soft e Hard com Tone) e Modulações estéreo (Chorus, Flanger e Phaser).
  - Sub-oscilador senoidal dedicado (`Sine Level`).
- **Modulação e Expressividade (Parte 5)**:
  - Router de 10 slots com Target, Via, Source e Amount bipolar.
  - 3 envelopes dedicados: ENV1 (filtro), ENV2 (modulação livre), ENV3 (amplitude).
  - LFO 1 polifônico (por voz) e LFO 2 monofônico (global) com fade-in e 7 formas de onda.
  - Síntese Vetorial (Vector Envelope) no triângulo de mix e Glide/Portamento.
- **Presets de Fábrica (Parte 8)**:
  - Presets incluídos: *User Default*, *Fat ES2 Bass*, *Logic Classic Saw Lead*, *Vintage 80s Poly Brass*, *Morphing Vector Pad*, *Resonant FM Pluck*.
