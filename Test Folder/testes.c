#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SetBit(A,k) ( A[(k/32)] |= (1 << (k%32)) )
#define ClearBit(A,k) ( A[(k/32)] &= ~(1 << (k%32)) )
#define TestBit(A,k) ( A[(k/32)] & (1 << (k%32)) )
// ------------------- VARIAVEIS / CONSTANTES ----------------------------------
int img_x = 0;
int img_y = 0;
int line_size = 0; //número de inteiros que compoem a linha de array de bits
unsigned int* original_img;
unsigned int* filtered_img;
// origin e end são para as operações trabalharem apenas sobre a imagem e não terem que percorrer a borda
int img_start_x = 1;
int img_start_y = 1;
int img_end_x = 0; // img_x + 1
int img_end_y = 0; //img_y + 1
// ------------------- FUNÇÔES -------------------------------------------------
// ABRIR ARQUIVO E PASSAR PARA VETOR DE INT()
int open_img (char *file_name){
  FILE *fp;
  int c;
  char formato[4];
  // PASSOS 1-abrir arquivo, 2-verificar formato, 3-verificar se é BW (P1), 4-descobrir tamanho, 5-passar para matriz
  // PASSO 1
  fp = fopen(file_name, "r");
  if (fp == NULL){
    perror("Erro ao abrir arquivo, certifique-se de que o nome do arquivo foi inserido corretamente.\n");
    exit(1);
  }
  //PASSO 2
  if (!fgets(formato, sizeof(formato), fp)){
    perror("ERROR: Arquivo %s formatado incorretamente, por favor fornecer um arquivo PBM.\n");
    exit(1);
  }
  //PASSO 3
  if (formato[0] != 'P' || formato[1] != '1'){
    fprintf(stderr, "ERRO: imagem não se encontra no formato P1 (Preto e Branco).\n");
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
      precisa achar o primeiro valor não comentário.
  */
  while (c == '#'){
    while (getc(fp) != '\n');
    c = getc(fp);
  }
  ungetc (c, fp);

  //PASSO 4
  if(fscanf(fp, "%d %d", &img_x, &img_y) != 2){
    fprintf(stderr, "ERRO: problema ao ler tamanho da imagem\n");
    exit(1);
  }
  printf("X: %d -- Y: %d\n",img_x, img_y);

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
  original_img = (unsigned int*)malloc(sizeof(unsigned int)*size);
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
    fprintf(stderr, "ERROR: incapaz de alocar memória\n");
    exit(1);
  }
  //após alocação toda a matriz é preenchida com 0.
  memset(original_img, 0, size);

  int y_pos;
  for (y_pos = 1; y_pos <= img_y; y_pos++){
    c = getc(fp);
    int x_pos = 1; // inicia em um para que a 1ª coluna seja completamente 0
    //esse while percorre uma linha por vez do arquivo.
    while ((c != '\n') && (c =! ' ')){
      if (c == '1'){
        //a linha abaixo usa operações de bitshift para manipular os bits dentro de um inteiro.
         SetBit(original_img, (y_pos*line_size + x_pos));
         x_pos++;
       }else{
         x_pos++;
       }
    c = getc(fp);
    }
  }
  return 0;
}

int save_img (char *file_name){

}
// ------------------- MAIN ----------------------------------------------------
int main(int argc, char const *argv[]) {
  printf("%d\n", open_img(argv[1]) );
  return 0;
}
