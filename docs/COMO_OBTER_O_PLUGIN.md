# Como gerar o ES3 pronto para instalar (sem programar)

O computador do GitHub compila o plugin para você, de graça. Você só precisa de uma conta.

## Uma vez só
1. Acesse https://github.com e crie uma conta (botão "Sign up").
2. Clique no "+" (canto superior direito) > "New repository".
3. Em "Repository name" escreva `ES3`. Marque **Public**. Clique em "Create repository".
   (Público é recomendado: o JUCE usa licença AGPLv3, que exige código aberto se você distribuir o plugin.)
4. Extraia o ZIP do projeto no seu PC (botão direito > "Extrair tudo").
5. Na página do repositório, clique em "uploading an existing file".
6. Abra a pasta extraída e arraste **todo o conteúdo** dela (incluindo a pasta oculta `.github`) para a página. Use o Chrome ou o Edge.
   Se a pasta `.github` não aparecer no Explorador, clique em "Exibir" > "Mostrar" > "Itens ocultos".
7. Role até o fim e clique em "Commit changes".

## Pegar o instalador
8. Clique na aba **Actions**. Se pedir, clique em "I understand my workflows, enable them".
9. Clique em "Compilar ES3 (Windows)" e aguarde (cerca de 10 a 20 minutos; bolinha amarela = rodando, verde = ok, vermelha = erro).
10. Se ficar verde: clique na execução e, em "Artifacts" (no fim da página), clique em **ES3-Windows-x64** para baixar.
11. Extraia o ZIP baixado. Clique com o botão direito em `Instalar_ES3.bat` > "Executar".
12. No FL Studio: Options > Manage plugins > Find installed plugins. O ES3 aparece em Instruments.

## Se a bolinha ficar vermelha
Clique na execução com erro, copie as últimas linhas do passo que falhou e cole aqui no chat. Eu corrijo e entrego um ZIP novo.

## Atualizações
Para cada ZIP novo: extraia, apague os arquivos antigos do repositório (ou arraste por cima, substituindo) e repita os passos 5 a 12.
