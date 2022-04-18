include <stdio.h>;

// ------------------- VARIAVEIS / CONSTANTES ----------------------------------
int img_x = 0;
int img_y = 0;
// +2 é um "padding" de 0 na borda para facilitar a apliação dos filtros
int original_img[img_x+2][img_y+2];
int filtered_img[img_x+2][img_y+2];
// origin e end são para as operações trabalharem apenas sobre a imagem e não terem que percorrer a borda
int img_start_x = 1;
int img_start_y = 1;
int img_end_x = img_x + 1;
int img_end_y = img_y + 1;
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

  18/04/22
  Obs.: a mascara pode ser simplificada e desconsiderar as diagonais, o que nos ajuda na hora de detectar e seguir a borda
  ignorando os cantos na aplicação da mascara será impossivel a formação de "L" isso quer dizer que após aplicação do filtro
  em um espaço 3x3 poderão existir apenas 3 pixeis pretos, e eles deverão tocar pelo menos dois lados opostos dese quadrado
  dessa forma basta saber sua posição atual, sua ultiama posição e a partir dai procurar a ultima posição.
*/
int edge_mask [3][3] = {{0 , -1 , 0}, {-1 , 4 , -1}, {0 , -1 , 0}};
// ------------------- FUNÇÔES -------------------------------------------------
// TO-DO escolher entre one-liner e matriz para o filtro. 18/04/22 => ONE-LINER
int apply_mask (cord_x, cord_y){
  return img[cord_x][cord_y] - (img[cord_x -1][cord_y] * img[cord_x+1][cord_y]
                                * img[cord_x][cord_y-1] * img[cord_x][cord_y+1])
};
// TO-DO função que percorre a imagem e encontra as "sementes" das formas
void find_seed () {
  /*
  função apenas para lustração, pois ela percorre a imagem que ela não recebe como parametro.
  essa função aparecerá dentro do main, ela percorre a matrix e encontra o priemrio ponto
  preto da imagem "semente", e passa a cordenada dessa semente para a função "get_shape"
  que por sua vez percorre a forma, procurando por pixeis conectados e os remove da imagem.
  a procura por buracos é feita posterirormente.
  */
  // i Linhas  | j Colunas
  int i;
  int j;
  //busca começa (1,1) e termina em (x+1, y+1) por causa do padding adicionado no inicio do código
  for (j = img_start_y ; j <= img_end_y ; j++ ){
    for (i = img_start_x ; i <= img_end_x ; i++){
      //preto = 1 = TRUE
      if (filtered_img[i][j]){
        get_shape(i,j);
      }
    }
  }
};
// TO-DO estrutura que armazena as fórmas.
