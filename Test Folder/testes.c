#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define SetBit(A,k) ( A[(k/32)] |= (1 << (k%32)) ) // lembrete k = ((y_pos*line_size*32) + x_pos)
#define ClearBit(A,k) ( A[(k/32)] &= ~(1 << (k%32)) ) // lembrete k = ((y_pos*line_size*32) + x_pos)
#define TestBit(A,k) ( A[(k/32)] & (1 << (k%32)) ) // lembrete k = ((y_pos*line_size*32) + x_pos)
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
  fp = fopen(file_name, "rb");
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
  line_size = (((img_x+2)/32)+(1*(((img_x+2)%32) != 0)));
  printf("line size = %d\n", line_size );
  int size = ( line_size * (img_y+2));
  printf("total size = %d integers\n", size);
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
  int x_pos;
  for (y_pos = 1; y_pos <= img_y; y_pos++){
    x_pos = 1; // reset da variavel.
    while (x_pos <= img_end_x) {
      c = getc(fp);
      if (c == 10 || c == 20){

      }else{
        if (c == 49){
          SetBit(original_img, ((y_pos*line_size*32) + x_pos));
        }
      x_pos++;
      }
    }
  }

  fclose(fp);
  return 0;
}

int save_img (char *file_name, unsigned int *img, char *c1, char *c2){
  // "wb" tenta abrir/criar o arquivo e sobrescreve o conteudo.
  FILE *fp;
  fp = fopen(file_name, "wb");
  if (!fp){
    fprintf(stderr, "Erro ao salvar arquivo: %s\n", file_name);
    exit(1);
  }
  //Formato
  fprintf(fp, "P1\n");
  //Comentário
  fprintf(fp, "# Projeto PI 2021.2 - Grupo: Diego, Feliep, Luan.\n");
  fprintf(fp, "# %s\n",c1 ); //c1 = X Formas Encontradas.
  fprintf(fp, "# %s\n",c2 ); //c2 = X Buracos Encontrados.
  //tamanho
  int t_x = img_end_x-img_start_x;
  int t_y = img_end_y-img_start_y;
  fprintf(fp, "%d %d\n",t_x, t_y); //

  //LoooooooooooooooooooooP
  int y_pos;
  int x_pos;
  for (y_pos = 1; y_pos <= img_end_y; y_pos++){
    for (x_pos = 1; x_pos <= img_end_x; x_pos++){
      if (TestBit(original_img, ((y_pos*line_size*32) + x_pos))){
        putc(49, fp);
      }else {
        putc(48, fp);
      }
    }
    putc('\n', fp);
  }
  fclose(fp);

  return 0;
}
// ------------------- MAIN ----------------------------------------------------
int main(int argc, char const *argv[]) {
  clock_t t ;
  double elapsed_clocks;

  t = clock(); //"Start clock"
  printf("%d\n", open_img(argv[1]) );//Load
  t = clock() - t;//"Stop Clock"
  elapsed_clocks = ((double)t)/CLOCKS_PER_SEC; //"Math"
  printf("Load demorou: %f segundos\n", elapsed_clocks);

  t = clock(); //"Start clock"
  printf("%d\n", save_img("copia.pbm", original_img, "nada", "algo"));
  t = clock() - t;//"Stop Clock"
  elapsed_clocks = ((double)t)/CLOCKS_PER_SEC; //"Math"
  printf("Save demorou: %f segundos\n", elapsed_clocks);
  free(original_img);
  return 0;
}
