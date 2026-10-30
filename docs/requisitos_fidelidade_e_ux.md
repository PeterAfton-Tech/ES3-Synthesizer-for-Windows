# Requisitos definidos pelo autor do projeto (obrigatórios nas próximas partes)

## Fidelidade de época
- ES3 é uma réplica do ES2 para Windows, NÃO um sintetizador moderno. Manter as limitações e escolhas características do ES2.
- Aliasing: manter um pouco, como o ES2 (constante `kDefaultAliasing` em Source/Engine/EngineParams.h, hoje 0.5).
  Este valor é PROVISÓRIO: o aliasing real do ES2 não foi medido. Para calibrar de verdade são necessárias gravações do ES2
  (nota aguda de serra/quadrada, sem filtro) feitas por quem tem acesso legítimo ao Logic Pro.
- Interface clássica, sem recursos modernos, exceto os que ajudam no uso (abaixo).

## Conforto de uso (Parte 7, interface)
- Funcionar em janela solta ("Detach") do FL Studio.
- Interface redimensionável (com proporção travada).
- Duplo clique em um parâmetro: volta ao valor padrão.
- Botão direito: menu do parâmetro (resetar, digitar valor).
- Roda do mouse: gira botões e move faders.
