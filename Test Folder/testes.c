#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define SetBit(A,k) ( A[(k/32)] |= (1 << (k%32)) ) // lembrete k = ((y_pos*img0->line_size*32) + x_pos)
#define ClearBit(A,k) ( A[(k/32)] &= ~(1 << (k%32)) ) // lembrete k = ((y_pos*img0->line_size*32) + x_pos)
#define TestBit(A,k) ( A[(k/32)] & (1 << (k%32)) ) // lembrete k = ((y_pos*img0->line_size*32) + x_pos)
// ------------------- VARIAVEIS / CONSTANTES / STRUCTS ------------------------
typedef struct{
  unsigned int start_x;
  unsigned int start_y;
  unsigned int end_x;
  unsigned int end_y;
  unsigned int size_x;
  unsigned int size_y;
  unsigned int line_size;
  unsigned int* data;
} Imagem;

struct stack{
   unsigned int index_a;
   unsigned int index_b;
   struct stack *ptr; //pointer type of stack
};
typedef struct{
  unsigned int a;
  unsigned int b;
}Tupla;

typedef struct stack Stack;
typedef Stack *stackPtr;
// start e end são para as operações trabalharem apenas sobre a Imagem e não terem que percorrer a borda
Imagem *img0;
unsigned int *shape_map;
unsigned int *grupos;
// start e end são para as operações trabalharem apenas sobre a Imagem e não terem que percorrer a borda
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
    fprintf(stderr, "ERRO: Imagem não se encontra no formato P1 (Preto e Branco).\n");
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

  //PASSO 3.2 Pre alocar struct.
  img0 = (Imagem *)malloc(sizeof(Imagem));
    if (!img0) {
         fprintf(stderr, "Erro ao alocar Memória para a estrutura da Imagem.\n");
         exit(1);
  }
  //definir que a Imagem começa em (1,1), garante que existe padding em volta da Imagem.
  img0->start_x = 1;
  img0->start_y = 1;

  //PASSO 4
  if(fscanf(fp, "%d %d", &img0->size_x, &img0->size_y) != 2){
    fprintf(stderr, "ERRO: problema ao ler tamanho da Imagem\n");
    exit(1);
  }
  printf("X: %d -- Y: %d\n",img0->size_x, img0->size_y);

  //PASSO 5
  img0->end_x = img0->size_x + 1;
  img0->end_y = img0->size_y + 1;
  /*
  C não suporta um array de bits, podemos desperdiçar 31bits de espaço e armazenar cada pixel como uma int.
  ou podemos agrupar 32 pixels em um int, o que reduz MUITO o espaço necessário,
  mas consume muito mais da minha paciencia.
  desperdiçãndo espaço uma img de 10.000x10.000 pixels ocuparia aprox 381 megabytes,
  agrupando 32 bits em um int ela ocuparia apenas 11.9 megabytes ("pequena diferença")
  a Imagem maxima suportada nesse código tem n pixels onde n é o maior tamanho possivel de um int,
  tal Imagem ocuparia 8gigas de ram (desperdico) ou 255megas (agrupado).

  utilizando o agrupamento de bits em inteiros, nós teremeos o vetor:
    img0[(img_x/32)+(1*(img_x%32 != 0))][img_y]
    ou seja o número de colunas da imgagem n inteiros onde n = res_x/32 e caso a divisão não seja exata
    ele irá adicionar mais um inteiro que será parcialmente preenchido.
    sendo assim é de vital importancia guardar o tamanho da Imagem original.
  */
  //+2 é o padding de pixels brancos em volta da Imagem para facilitar aplicação do filtro
  img0->line_size = (((img0->size_x+2)/32)+(1*(((img0->size_x+2)%32) != 0)));
  printf("line size = %d\n", img0->line_size );
  int size = ( img0->line_size * (img0->size_y+2));
  printf("total img size = %d integers\n", size);
  img0->data = (unsigned int*)malloc(sizeof(unsigned int)*size);
  /*
   C é sofrimento, como a matriz que descreve a imgem tem seu tamanho declarado durante execução
   ela é na verdade um array 1D, ou seja devemos prestar atenção na gora de acessar.
   e não podemos acessar como img[x][y] temos que acessar img[y*line_size + x]
   !!!! IMPORTENTE COMO PADRÃO PARA ESSE PROJETO O VETOR 1D QUE ARMAZENA A Imagem
   É DO TIPO [Y][X], ONDE Y É A LINHA E X A COLUNA, DESSA FORMA AO ACESSAR O VETOR 1D
   TEMOS QUE MULTIPLICAR A POSIÇÃO DA LINHA PELO NÚMERO DE INTEIROS POR LINHA, PARA FACILIAR ESSE
   VALOR SERÁ UMA VÁRIAVEL DECLARADA NO COMEÇO DO CÓDIO.
   img[y*line_size + x] = img[y][x]
   onde y é o número de linhas e x é o número de colunas
  */
  if (!img0){
    fprintf(stderr, "ERROR: incapaz de alocar memória para os dados da Imagem\n");
    exit(1);
  }
  //após alocação toda a matriz é preenchida com 0.
  memset(img0->data, 0, size*sizeof(unsigned int));

  int y_pos;
  int x_pos;
  for (y_pos = 1; y_pos < img0->end_y; y_pos++){
    x_pos = 1; // reset da variavel.
    while (x_pos < img0->end_x) {
      c = getc(fp);
      if (c == 10 || c == 20){
        //nada acontece feijoada.
        //10 = line break, 20 = espaço, 48 = 0, 49 = 1.
      }else{
        if (c == 49){
          SetBit(img0->data, ((y_pos*img0->line_size*32) + x_pos));
        }
      x_pos++;
      }
    }
  }

  fclose(fp);
  return 0;
}

int save_img (char *file_name, Imagem *img, char *c1, char *c2){
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
  fprintf(fp, "# Projeto PI 2021.2 - Grupo: Diego, Felipe, Luan.\n");
  fprintf(fp, "# %s\n",c1 ); //c1 = X Formas Encontradas.
  fprintf(fp, "# %s\n",c2 ); //c2 = X Buracos Encontrados.
  //tamanho
  unsigned int t_x = img->size_x;
  unsigned int t_y = img->size_y;
  fprintf(fp, "%d %d\n",t_x, t_y); //

  //LoooooooooooooooooooooP
  unsigned int y_pos;
  unsigned int x_pos;
  for (y_pos = 1; y_pos < img->end_y; y_pos++){
    for (x_pos = 1; x_pos < img->end_x; x_pos++){
      if (TestBit(img->data, ((y_pos*img->line_size*32) + x_pos))){
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

void push( stackPtr *top, int x , int y){
    stackPtr nodePtr;

    nodePtr = malloc(sizeof(Stack));

    if(nodePtr != NULL)
    {
       nodePtr->index_a = x;
       nodePtr->index_b = y;
       nodePtr->ptr = *top;
       *top = nodePtr;
    }

    else
       printf("Erro ao alocar espaço para ponteiros/n");
   }

Tupla pop(stackPtr *top){
  Tupla tp;
  stackPtr tempPtr; //temporary pointer

  tempPtr = *top;
  tp.a = (*top)-> index_a;
  tp.b = (*top)-> index_b;
  *top = (*top) -> ptr;
  free(tempPtr); //free temporary pointer value

  return tp;
}

int find_shape (Imagem *img) {
  unsigned int x_pos;
  unsigned int y_pos;
  unsigned int index_count = 1;
  unsigned int colunas = img->size_x + 2;
  unsigned int linhas = img->size_y + 2;
  stackPtr links = NULL;

  shape_map = (unsigned int*)malloc(linhas * colunas * sizeof(unsigned int));
  if (!shape_map) {
       fprintf(stderr, "Erro ao alocar Memória para busca de formas.\n");
       exit(1);
}
  memset (shape_map, 0, (linhas*colunas*sizeof(unsigned int)));

  for (y_pos = img->start_y; y_pos < img->end_y; y_pos++){
    for (x_pos = img->start_x; x_pos < img->end_x; x_pos++){
      unsigned int i = TestBit(img->data, ((y_pos*img->line_size*32) + x_pos));
      unsigned int r = TestBit(img->data, (((y_pos-1)*img->line_size*32) + x_pos));
      unsigned int t = TestBit(img->data, ((y_pos*img->line_size*32) + (x_pos-1)));
      unsigned int g_r = shape_map[((y_pos-1)*colunas)+x_pos];
      unsigned int g_t = shape_map[((y_pos)*colunas)+(x_pos-1)];

      if (i){
        if (r && t){
          if (g_r == g_t){
            shape_map[(y_pos*colunas)+x_pos] = g_r;
          } else {
              shape_map[(y_pos*colunas)+x_pos] = g_r;
              push(&links, g_r, g_t);
          }
        }else if (r){
          shape_map[(y_pos*colunas)+x_pos] = g_r;
        }else if (t){
          shape_map[(y_pos*colunas)+x_pos] = g_t;
        }else {
          shape_map[(y_pos*colunas)+x_pos] = index_count;
          index_count++;
        }
      }
    }
  }

  stackPtr temp = links;
  grupos = (unsigned int*)malloc(index_count* sizeof(unsigned int));
  memset (grupos, 0, index_count*sizeof(unsigned int));
  while (temp != NULL) {
    Tupla t_temp;
    t_temp = pop(&temp);
    grupos [t_temp.b] = t_temp.a;
  }
  for (int i = 0; i < index_count; i++){
    printf("(%d->%d) ", i, grupos[i]);
  }
  return 0;
}
// ------------------- MAIN ----------------------------------------------------
int main(int argc, char const *argv[]) {
  clock_t t ;
  double elapsed_clocks;

  t = clock(); //"Start clock"
  printf("Img Open result: %d\n", open_img(argv[1]) );//Load
  t = clock() - t;//"Stop Clock"
  elapsed_clocks = ((double)t)/CLOCKS_PER_SEC; //"Math"
  printf("Load demorou: %f segundos\n", elapsed_clocks);

  t = clock(); //"Start clock"
  printf("Img Save result: %d\n", save_img("copia.pbm", img0, "nada", "algo"));
  t = clock() - t;//"Stop Clock"
  elapsed_clocks = ((double)t)/CLOCKS_PER_SEC; //"Math"
  printf("Save demorou: %f segundos\n", elapsed_clocks);

  t = clock(); //"Start clock"
  printf("Img find_shape result: %d\n", find_shape(img0));//Load
  t = clock() - t;//"Stop Clock"
  elapsed_clocks = ((double)t)/CLOCKS_PER_SEC; //"Math"
  printf("Busca por formas demorou: %f segundos\n", elapsed_clocks);
  free(img0);
  return 0;
}
