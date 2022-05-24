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

Imagem *img0;
unsigned int formas_count = 0;
unsigned int buracos_count = 0;
unsigned int formas_defeito = 0;
unsigned int tolerancia = 0;
// tolerancia é a quantidade máxima de buracos em uma forma para que ela seja considerada ok.
//Shape e Hole map movidos para global para ser possivel buscar por formas com buracos_count
unsigned int *shape_map;
unsigned int *hole_map;
// ------------------- FUNÇÔES -------------------------------------------------
unsigned int menor (unsigned int a, unsigned int b){
  if (a <= b){
    return a;
  } else {
    return b;
  }
}
unsigned int maior (unsigned int a, unsigned int b){
  if (a >= b){
    return a;
  } else {
    return b;
  }
}
int cmpfunc (const void * a, const void * b) {
  //para uso na função qsort.
  return ( *(int*)a - *(int*)b );
}

// ABRIR ARQUIVO E PASSAR PARA VETOR DE INT()
int open_img (char *file_name){
  FILE *fp;
  int c;
  char formato[4];
  printf("LOAD\n");
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

int save_img (char *file_name, Imagem *img, unsigned int f_c, unsigned int h_c){
  // "wb" tenta abrir/criar o arquivo e sobrescreve o conteudo.
  printf("SAVE\n");
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
  fprintf(fp, "# Total de Formas Encontradas: %d.\n",f_c ); //c1 = X Formas Encontradas.
  fprintf(fp, "# Total de Buracos Encontrados: %d.\n",h_c ); //c2 = X Buracos Encontrados.
  fprintf(fp, "# Para mais informações sobre as formas favor observar o arquivo: 'FormasGraymap.pgm'.\n");
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

void push (stackPtr *top, int x , int y){
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

int graymap (unsigned int *map, int tons, unsigned int colunas, char *file_name){

  if (tons > 255){
    printf("Mais de 255 formas encontradas, não foi possivel gerar graymap\n");
    return 1;
  }

  int intervalo = (254/tons);
  unsigned int *tone_map;
  tone_map = (unsigned int*)malloc(tons*2*sizeof(unsigned int));

  for (int i = 0; i < tons; i++){
    tone_map[2*i] = i;
    tone_map[(2*i)+1] = 1+(i*intervalo);
    //printf("(%d->%d)\n",tone_map[2*i], tone_map[(2*i)+1] );
  }

  //basicamente save_img
  FILE *fp;
  fp = fopen(file_name, "wb");
  if (!fp){
    fprintf(stderr, "Erro ao salvar arquivo: %s\n", file_name);
    exit(1);
  }
  //Formato
  fprintf(fp, "P2\n");
  //Comentário
  fprintf(fp, "# Projeto PI 2021.2 - Grupo: Diego, Felipe, Luan.\n");
  fprintf(fp, "# Número de formas: %d.\n", tons); //c1 = X Formas Encontradas.
  fprintf(fp, "# Intervalo entre os tons: %d.\n",intervalo); //c2 = X Buracos Encontrados.
  //tamanho
  unsigned int t_x = img0->size_x;
  unsigned int t_y = img0->size_y;
  fprintf(fp, "%d %d\n",t_x, t_y); //
  fprintf(fp, "%d\n", 255); //

  //LoooooooooooooooooooooP
  unsigned int y_pos;
  unsigned int x_pos;
  for (y_pos = img0->start_y; y_pos < img0->end_y; y_pos++){
    for (x_pos = img0->start_x; x_pos < img0->end_x; x_pos++){
      unsigned int g = map[(y_pos*colunas)+x_pos];
      if (g){
        int cinza = 0;
        for (int i = 0; i < tons; i++){
          if (g == tone_map[2*i]){
            cinza = tone_map[(2*i)+1];
          }
        }
        fprintf(fp, "%d ", cinza);
      }else {
        fprintf(fp, "%d ", 255);
      }
    }
    fprintf(fp, "\n");
  }
  fclose(fp);
  return 0;
}

int find_shape (Imagem *img) {
  unsigned int x_pos;
  unsigned int y_pos;
  unsigned int index_count = 1;
  unsigned int colunas = img->size_x + 2;
  unsigned int linhas = img->size_y + 2;
  //unsigned int *shape_map;
  stackPtr links = NULL;
  unsigned int *grupos;
  unsigned int *grupos_validos;
  printf("Formas\n");

  shape_map = (unsigned int*)malloc(linhas * colunas * sizeof(unsigned int));
  if (!shape_map) {
       fprintf(stderr, "Erro ao alocar Memória para busca de formas.\n");
       exit(1);
}
  memset(shape_map, 0, (linhas*colunas*sizeof(unsigned int)));

  //varrendo a imagem e agrupando os pixels
  for (y_pos = img->start_y; y_pos < img->end_y; y_pos++){
    for (x_pos = img->start_x; x_pos < img->end_x; x_pos++){
      unsigned int i = TestBit(img->data, ((y_pos*img->line_size*32) + x_pos));
      /* g_ é o valor do grupo ao qual o ponto pertence:
      i é o pixel alvo (x, y),
      r está acima de i (x, y-1),
      t está a esqueda de i (x-1, y)
      devido ao possivel caso do pixel estar e contato apenas pela diagonal,
      temos que chacar s que é (x-1, y-1)
      */
      unsigned int g_r = shape_map[((y_pos-1)*colunas)+x_pos];
      unsigned int g_t = shape_map[((y_pos)*colunas)+(x_pos-1)];
      if (i){
        if((g_r == g_t) && (g_r != 0)){
          shape_map[((y_pos)*colunas)+x_pos] = g_r;
        }else if (g_r != g_t){
          if ((g_r != 0) && (g_t != 0)){
            shape_map[((y_pos)*colunas)+x_pos] = g_r;
            push(&links, g_r, g_t);
          }else if(g_r != 0){
            shape_map[((y_pos)*colunas)+x_pos] = g_r;
          }else if (g_t != 0){
            shape_map[((y_pos)*colunas)+x_pos] = g_t;
          }
        }else {
          shape_map[((y_pos)*colunas)+x_pos] = index_count;
          index_count++;
        }
      }
    }
  }

  //Check se existem formas
  if (index_count == 1){
    return 1;
  }

  //agrupar os grupos;
  grupos = (unsigned int*)malloc(index_count*sizeof(unsigned int));
  memset(grupos, 0, (index_count*sizeof(unsigned int)));
  while(links != NULL){
    Tupla temp = pop(&links);
    unsigned int a = temp.a;
    unsigned int b = temp.b;
    if ((grupos[a]==0) && (grupos[b]==0)){
      formas_count++;
      grupos[a] = formas_count;
      grupos[b] = formas_count;
    } else if((grupos[a]==0) && (grupos[b]!=0)) {
      grupos[a] = grupos[b];
    }else if ((grupos[b]==0) && (grupos[a]!=0)){
      grupos[b] = grupos[a];
    }else if (grupos[a] != grupos[b]){
      unsigned int holder = menor(grupos[a], grupos[b]);
      unsigned int to_change = maior(grupos[a], grupos[b]);
      for (int i = index_count-1; i > 0; i--){
        if (grupos[i] == to_change){
          grupos[i] = holder;
        }
      }
    }
  }

  //agrupar formas simples
  for (int i = index_count-1; i > 0; i--){
    if (grupos[i] == 0){
      formas_count++;
      grupos[i] = formas_count;
    }
  }

  //garantia que existe pelo menos uma forma devido ao check no começo da função
  formas_count = 1;

  //--contar o total de formas
  unsigned int *grupos_cpy = (unsigned int*)malloc(index_count*sizeof(unsigned int));
  memset(grupos_cpy, 0, (index_count*sizeof(unsigned int)));
  //copiar array de formas.
  for (int i = index_count-1; i > 0; i--){
    grupos_cpy[i] = grupos[i];
  }

  //qsort() na cópia do array de grupos nos permite contar com facilidade a quantidade
  //de elementos unicos no array o que equivale a real quantidade de formas.
  qsort(grupos_cpy, index_count, sizeof(unsigned int), cmpfunc);
  for (int i = 2; i < index_count; i++){
    if (grupos_cpy[i]>grupos_cpy[i-1]){
      formas_count++;
    }
  }

  //eu poderia juntar esses dois for loops em um só? sim poderia, mas não estou
  //com um pingo de vontade usar um array que cresce durante a execução.
  grupos_validos = (unsigned int*)malloc(formas_count*sizeof(unsigned int));
  memset(grupos_validos, 0 , formas_count*sizeof(unsigned int));
  int k = 0;
  grupos_validos[k] = grupos_cpy[1];
  for (int i = 2; i < index_count; i++){
    if (grupos_cpy[i]>grupos_cpy[i-1]){
      k++;
      grupos_validos[k] = grupos_cpy[i];
    }
  }

  //ultima organização para que os grupos tenham os valores minimos
  for (int i = 1; i < index_count-1; i++){
    for (int j = 0; j < formas_count; j++){
      if (grupos[i] == grupos_validos[j]){
        grupos[i] = j+1;
      }
    }
  }

  //print para verificação
  /*for (int i = index_count-1; i > 0; i--){
      printf("(%d->%d) ", i, grupos[i]);
    }
  printf("\n");*/

  //PERCORRER imagem e separar as formas
  for (y_pos = img->start_y; y_pos < img->end_y; y_pos++){
    for (x_pos = img->start_x; x_pos < img->end_x; x_pos++){
      unsigned int g_value = shape_map[((y_pos)*colunas)+x_pos];
      if(g_value){
        shape_map[((y_pos)*colunas)+x_pos]=grupos[g_value];
      }
    }
  }

  graymap(shape_map, formas_count, colunas, "FormasGraymap.pgm");

  free(grupos);
  free(grupos_cpy);
  free(grupos_validos);
  //free(shape_map);
  free(links);
  return 0;
}

int find_hole (Imagem *img){
  unsigned int x_pos;
  unsigned int y_pos;
  unsigned int index_count = 1;
  unsigned int colunas = img->size_x + 2;
  unsigned int linhas = img->size_y + 2;
  //unsigned int *hole_map;
  stackPtr links = NULL;
  unsigned int *grupos;
  unsigned int *buracos_validos;
  printf("Buracos\n");

  hole_map = (unsigned int*)malloc(linhas * colunas * sizeof(unsigned int));
  if (!hole_map) {
       fprintf(stderr, "Erro ao alocar Memória para busca de buracos.\n");
       exit(1);
}
  memset(hole_map, 0, (linhas*colunas*sizeof(unsigned int)));

  //varrendo a imagem e agrupando os pixels
  for (y_pos = img->start_y; y_pos < img->end_y; y_pos++){
    for (x_pos = img->start_x; x_pos < img->end_x; x_pos++){
      unsigned int i = TestBit(img->data, ((y_pos*img->line_size*32) + x_pos));
      /* g_ é o valor do grupo ao qual o ponto pertence:
      i é o pixel alvo (x, y),
      r está acima de i (x, y-1),
      t está a esqueda de i (x-1, y)
      devido ao possivel caso do pixel estar e contato apenas pela diagonal,
      temos que chacar s que é (x-1, y-1)
      */
      unsigned int g_r = hole_map[((y_pos-1)*colunas)+x_pos];
      unsigned int g_t = hole_map[((y_pos)*colunas)+(x_pos-1)];
      if (i == 0){
        if((g_r == g_t) && (g_r != 0)){
          hole_map[((y_pos)*colunas)+x_pos] = g_r;
        }else if (g_r != g_t){
          if ((g_r != 0) && (g_t != 0)){
            hole_map[((y_pos)*colunas)+x_pos] = g_r;
            push(&links, g_r, g_t);
          }else if(g_r != 0){
            hole_map[((y_pos)*colunas)+x_pos] = g_r;
          }else if (g_t != 0){
            hole_map[((y_pos)*colunas)+x_pos] = g_t;
          }
        }else {
          hole_map[((y_pos)*colunas)+x_pos] = index_count;
          index_count++;
        }
      }
    }
  }

  //agrupar os grupos;
  grupos = (unsigned int*)malloc(index_count*sizeof(unsigned int));
  memset(grupos, 0, (index_count*sizeof(unsigned int)));
  while(links != NULL){
    Tupla temp = pop(&links);
    unsigned int a = temp.a;
    unsigned int b = temp.b;
    if ((grupos[a]==0) && (grupos[b]==0)){
      buracos_count++;
      grupos[a] = buracos_count;
      grupos[b] = buracos_count;
    } else if((grupos[a]==0) && (grupos[b]!=0)) {
      grupos[a] = grupos[b];
    }else if ((grupos[b]==0) && (grupos[a]!=0)){
      grupos[b] = grupos[a];
    }else if (grupos[a] != grupos[b]){
      unsigned int holder = menor(grupos[a], grupos[b]);
      unsigned int to_change = maior(grupos[a], grupos[b]);
      for (int i = index_count-1; i > 0; i--){
        if (grupos[i] == to_change){
          grupos[i] = holder;
        }
      }
    }
  }

  //agrupar formas simples
  for (int i = index_count-1; i > 0; i--){
    if (grupos[i] == 0){
      buracos_count++;
      grupos[i] = buracos_count;
    }
  }

  //garantia que existe pelo menos uma forma devido ao check no começo da função
  buracos_count = 1;

  //--contar o total de formas
  unsigned int *grupos_cpy = (unsigned int*)malloc(index_count*sizeof(unsigned int));
  memset(grupos_cpy, 0, (index_count*sizeof(unsigned int)));
  //copiar array de formas.
  for (int i = index_count-1; i > 0; i--){
    grupos_cpy[i] = grupos[i];
  }

  //qsort() na cópia do array de grupos nos permite contar com facilidade a quantidade
  //de elementos unicos no array o que equivale a real quantidade de buracos.
  qsort(grupos_cpy, index_count, sizeof(unsigned int), cmpfunc);
  for (int i = 2; i < index_count; i++){
    if (grupos_cpy[i]>grupos_cpy[i-1]){
      buracos_count++;
    }
  }

  buracos_validos = (unsigned int*)malloc(buracos_count*sizeof(unsigned int));
  memset(buracos_validos, 0,buracos_count*sizeof(unsigned int));
  int k = 0;
  buracos_validos[k] = grupos_cpy[1];
  for (int i = 2; i < index_count; i++){
    if (grupos_cpy[i]>grupos_cpy[i-1]){
      k++;
      buracos_validos[k] = grupos_cpy[i];
    }
  }

  //ultima organização para que os grupos tenham os valores minimos
  for (int i = index_count-1; i > 0; i--){
    for (int j = buracos_count; j >= 0; j--){
      if (grupos[i] == buracos_validos[j]){
        grupos[i] = j+1;
      }
    }
  }

  //print para verificação
  /*for (int i = index_count-1; i > 0; i--){
      printf("(%d->%d) ", i, grupos[i]);
    }
  printf("\n");*/

  //PERCORRER imagem e separar as formas
  for (y_pos = img->start_y; y_pos < img->end_y; y_pos++){
    for (x_pos = img->start_x; x_pos < img->end_x; x_pos++){
      unsigned int g_value = hole_map[((y_pos)*colunas)+x_pos];
      if(g_value){
        hole_map[((y_pos)*colunas)+x_pos]=grupos[g_value];
      }
    }
  }

  graymap(hole_map, buracos_count, colunas, "BuracosGraymap.pgm");

  free(grupos);
  free(grupos_cpy);
  free(buracos_validos);
  //free(hole_map);
  free(links);
  return 0;
}

int find_defects (unsigned int *holes, unsigned int *shapes){
  unsigned int x_pos;
  unsigned int y_pos;
  unsigned int colunas = img0->size_x + 2;
  printf("Formas com Defeitos \n");
  /* O método de busca utilizado será:
  1 - carregar as listas e mapas e de buacos e formas validas;
  2 - gerar uma matrix x*y onde X é o número de formas e y o número de buracos,
  3 - sempre que houver contato entre um grupo e um buraco, o ponto x,y da matriz será marcado
  4 - após toda a imagem ser percorrida a matriz final indicará quais grupos e quis buracos se tocam,
  5 - o buraco que tocar todos os fundos é o background e cada buraco deve tocar apenas uma forma
      mas uma forma pode ter multiplos buracos.
  */

  //+1 que é adicionado permite alinhas cada grupo de formas e buracos com o seu index na matriz
  //desperdiçamos um pouco de memoria (uma linha e uma coluna), mas isso nos permite remover
  //dois for loops para percorrer as formas validadas e os buracos válidos.
  unsigned int *defect_map = (unsigned int*)malloc((formas_count+1)*(buracos_count+1)*sizeof(unsigned int));
  memset(defect_map, 0, ((formas_count+1)*(buracos_count+1)*sizeof(unsigned int)));

  for (y_pos = img0->start_y; y_pos < img0->end_y; y_pos++){
    for (x_pos = img0->start_x; x_pos < img0->end_x; x_pos++){
     unsigned int g_val = shapes[((y_pos)*colunas)+x_pos];
     unsigned int h1_val = holes[((y_pos)*colunas)+x_pos-1];
     unsigned int h2_val = holes[((y_pos-1)*colunas)+x_pos];

     if (g_val != 0){
       if (h2_val != 0){defect_map[(h2_val*(formas_count+1))+g_val] = 1;}
       else if (h1_val != 0){defect_map[(h1_val*(formas_count+1))+g_val] = 1;}
     }
    }
  }
  // i e j inicaim em 1 para pular o padding adicionado anterirormente
  //para gerar a tabela de peças com buracos por favor remover o // das linhas printf abaixo
  for (int i = 1; i < (formas_count+1); i++){
    int k = 0;
    for (int j = 1; j < (buracos_count+1); j++){
      printf("%d | ", defect_map[(j*(formas_count+1))+i]);
      if (defect_map[(j*(formas_count+1))+i]){
        k++;
      }
      //para aumentar a tolerancia modifique a variavel no inicio do código
      if (k > tolerancia+1){
        formas_defeito++;
        break;
      }
    }
    printf("\n" );
  }
  free(defect_map);
  return 0;
}
// ------------------- MAIN ----------------------------------------------------
int main(int argc, char *argv[]) {
  clock_t t ;
  double elapsed_clocks;

  t = clock(); //"Start clock"
  open_img(argv[1]);//Load
  t = clock() - t;//"Stop Clock"
  elapsed_clocks = ((double)t)/CLOCKS_PER_SEC; //"Math"
  printf("Load demorou: %f segundos\n", elapsed_clocks);
  printf("-------------------------------------------\n");

  t = clock(); //"Start clock"
  find_shape(img0);
  t = clock() - t;//"Stop Clock"
  elapsed_clocks = ((double)t)/CLOCKS_PER_SEC; //"Math"
  printf("Total de formas Encontradas: %d\n", formas_count);
  printf("Busca por formas/gerar graymap demorou: %f segundos\n", elapsed_clocks);
  printf("-------------------------------------------\n");

  t = clock(); //"Start clock"
  find_hole(img0);
  t = clock() - t;//"Stop Clock"
  elapsed_clocks = ((double)t)/CLOCKS_PER_SEC; //"Math"
  printf("Total de buracos Encontrados: %d\n", buracos_count-1);
  printf("Busca por buracos/gerar graymap demorou: %f segundos\n", elapsed_clocks);
  printf("-------------------------------------------\n");

  t = clock(); //"Start clock"
  find_defects(hole_map, shape_map);
  t = clock() - t;//"Stop Clock"
  elapsed_clocks = ((double)t)/CLOCKS_PER_SEC; //"Math"
  printf("Total de peças com defeito: %d\n", formas_defeito);
  printf("Busca por peças com defeito demororu: %f segundos\n", elapsed_clocks);

  printf("-------------------------------------------\n");

  t = clock(); //"Start clock"
  save_img("copia.pbm", img0, formas_count, buracos_count-1);//Save
  t = clock() - t;//"Stop Clock"
  elapsed_clocks = ((double)t)/CLOCKS_PER_SEC; //"Math"
  printf("Save demorou: %f segundos\n", elapsed_clocks);

  free(shape_map);
  free(hole_map);
  free(img0);
  return 0;
}
