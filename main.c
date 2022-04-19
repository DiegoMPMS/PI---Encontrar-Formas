include <stdio.h>;

// ------------------- VARIAVEIS / CONSTANTES ----------------------------------
int img_x = 0;
int img_y = 0;
int line_size = 0;
unsigned int* original_img;
unsigned int* filtered_img;
// origin e end são para as operações trabalharem apenas sobre a imagem e não terem que percorrer a borda
int img_start_x = 1;
int img_start_y = 1;
int img_end_x = 0 // img_x + 1;
int img_end_y = 0 //img_y + 1;
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

  int edge_mask [3][3] = {{0 , -1 , 0}, {-1 , 4 , -1}, {0 , -1 , 0}};

*/
// ------------------- FUNÇÔES -------------------------------------------------
// TO-DO escolher entre one-liner e matriz para o filtro. 18/04/22 => ONE-LINER
int apply_mask (cord_x, cord_y){
  return (original_img[cord_x][cord_y] - (original_img[cord_x -1][cord_y] * original_img[cord_x+1][cord_y] * original_img[cord_x][cord_y-1] * original_img[cord_x][cord_y+1]));
};

// função que percorre a imagem e encontra as "sementes" das formas
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
}

// ABRIR ARQUIVO E PASSAR PARA VETOR DE INT()
int open_img (string file_name){
  FILE *fp;
  int c;
  char formato[3];
  // PASSOS 1-abrir arquivo, 2-verificar formato, 3-verificar se é BW (P1), 4-descobrir tamanho, 5-passar para matriz
  // PASSO 1
  fp = fopen(file_name, "r");
  if (fp == NULL){
    perror("Erro ao abrir arquivo, certifique-se de que o nome do arquivo foi inserido corretamente.");
    exit(1);
  }
  //PASSO 2
  if (!fgets(formato, sizeof(formato), fp)){
    perror("ERROR: Arquivo %s formatado incorretamente, por favor fornecer um arquivo PBM.");
    exit(1);
  }
  //PASSO 3
  if (formato[0] != P || formato[1] != 1){
    fprintf(stderr, "ERRO: imagem não se encontra no formato P1 (Preto e Branco)\n");
    exit(1);
  }
  //PASSO 3.1 - ignorar comentários
  c = getc(fp);
  /*
  1 - lê a stream (1ª linha já foi removida no passo anterior, ela contem o formato)
  2 - c == "#" verifica se a linha é um comentário.
  3 - getc(fp) !=  '\n' é usado para ignorar todos os comentŕios e percorrer a stream.
  4 - c = getc(fp), lê a proxima linha e o ciclo continua.
  5 - usar um ungetc(c, fp) para devolver o ultimo valor pois esse loop
      precisa achar o primeiro valor não comtário.
  */
  while (c == "#"){
    while (getc(fp) != '\n');
    c = getc(fp);
  }
  ungetc (c, fp);

  //PASSO 4
  if(scanf(fp, "%d %d", img_x, img_y) != 2){
    fprintf(stderr, "ERRO: problema ao ler tamamho da imagem\n");
    exit(1);
  }

  //PASSO 5
  img_end_x = img_x + 1;
  img_end_y = img_y + 1;
  /*
  C não suporta um array de bits, podemos desperdiçar 31bits de espaço e armazenar cada pixel como uma int.
  ou podemos agrupar 32 pixels em um int, o que reduz MUITO o espaço necessário,
  mas consume muito mais da minha paciencia.
  desperdiçãndo espaço uma img de 10.000x10.000 pixels ocuparia aprox 381 megabytes,
  agrupando 32 bits em um int ela ocuparia apenas 11.9 megabytes ("pequena diferença")
  a imagem maxima suportada nesse código tem n pixels onde n é o maior tamanho possivel de um int,
  tal imagem ocuparia 8gigas de ram (desperdico) ou 255megas (agrupado).

  utilizando o agrupamento de bits em inteiros, nós teremeos o vetor:
    original_img[(img_x/32)+(1*(img_x%32 != 0))][img_y]
    ou seja o número de colunas da imgagem n inteiros onde n = res_x/32 e caso a divisão não seja exata
    ele irá adicionar mais um inteiro que será parcialmente preenchido.
    sendo assim é de vital importancia guardar o tamanho da imagem original.
  */
  //+2 é o padding de pixels brancos em volta da imagem para facilitar aplicação do filtro
  line_size = ((img_x+2/32)+(1*(img_x+2%32 != 0)));
  int size = ( line_size * img_y+2);
  original_img = (int*)malloc(sizeof(int)*size);
  /*
   C é sofrimento, como a matriz que descreve a imgem tem seu tamanho declarado durante execução
   ela é na verdade um array 1D, ou seja devemos prestar atenção na gora de acessar.
   e não podemos acessar como img[x][y] temos que acessar img[y*line_size + x]
   !!!! IMPORTENTE COMO PADRÃO PARA ESSE PROJETO O VETOR 1D QUE ARMAZENA A IMAGEM
   É DO TIPO [Y][X], ONDE Y É A LINHA E X A COLUNA, DESSA FORMA AO ACESSAR O VETOR 1D
   TEMOS QUE MULTIPLICAR A POSIÇÃO DA LINHA PELO NÚMERO DE INTEIROS POR LINHA, PARA FACILIAR ESSE
   VALOR SERÁ UMA VÁRIAVEL DECLARADA NO COMEÇO DO CÓDIO.
   img[y*line_size + x] = img[y][x]
   onde y é o número de linhas e x é o número de colunas
  */
  if (!original_img){
    fprintf(stderr, "ERROR: incapaz de alocar memória\n", );
    exit(1)
  }
  //após alocação toda a matriz é preenchida com 0.
  memset(original_img, 0, size);

  for (i = 1; i <= img_y; i++){
    c = getc(fp);
    int x_pos = 1; // inicia em um para que a 1ª coluna seja completamente 0
    //esse while percorre uma linha por vez do arquivo.
    while ((c != '\n') && (c =! ' '){
      if (c == '1'){
        //a linha abaixo usa operações de bitshift para manipular os bits dentro de um inteiro.
         original_img[i*line_size + k/32] |= 1 << (k%32);
         x_pos++;
       }else{
         x_pos++;
       }
    c = getc;
    }
  }
}
