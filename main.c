include <stdio.h>;

// ------------------- VARIAVEIS / CONSTANTES ----------------------------------
int img_x = 0;
int img_y = 0;
// +2 é um "padding" de 0 na borda para facilitar a detecção de objetos nas bordas
int img[img_x + 2] [img_y + 2]
// origin e end são para as operações trabalharem apenas sobre a imagem e não terem que percorrer a borda
int img_origin [2] = {1,1};
int img_end [2] = [img_x + 1, img_y + 1];
/*
  Essa mascara de detecção de bordas simples funciona apenas para imagens preto e branco,
  seu funcionamento é bem simples:
  se "você(pixel)" é 1 e pelo menos um de seus vizinhos é 0 = você é uma borda;
    se todos seus vizinhos são 1 = você não é bordas;
  e por fim se você é 1 = você não é borda;
  esse processo permite converter as formas em bordas continuas de um pixel.

  obs.: talvez seja possivel reduzir o número de movimentações na matriz usando
  apenas uma linha e multiplicações de boleanos (devido a imagem preto e branco).

  pix(x,y) = pix(x,y) - [pix(x-1,y-1)*pix(x-1,y)*pix(x-1,y+1)*...*pix(x+1,y+1)]

  o termo entre cheves permite facilmente detectar se algum dos pixels vizinhos é 0 (short-circuit)
  se o ponto central for 0 e todos vizinhos 1 o resultado é -1 ou seja => 0;
  se o ponto central for 0 e todos vizinhos 0 o resultado é 0 ou seja => 0;
  se o ponto central for 0 e e pelo menos um vizinho é != 1 o resultado é 0 ou seja => 0;
  se o ponto central for 1 e todos vizinhos 1 o resultado é 0 ou seja => 0;
  se o ponto central for 1 e todos vizinhos 0 o resultado é 1 ou seja => 1; (caso especial de forma com tamanho de 1 pixel)
  se o ponto central for 1 e e pelo menos um vizinho é 0 o resultado é 1 ou seja => 1;
*/
int edge_mask [3][3] = {{-1 , -1 , -1}, {-1 , 9 , -1}, {-1 , -1 , -1}};
