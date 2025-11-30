/*+-------------------------------------------------------------+
 | UNIFAL – Universidade Federal de Alfenas. |
 | BACHARELADO EM CIENCIA DA COMPUTACAO. |
 | Trabalho..: SIMULACAO DE SISTEMA DE ARQUIVOS FAT |
 | Disciplina: Sistemas Operacionais |
 | Professor.: Romário Borges |
 | Aluno(s)..: Gustavo Andrade Moreira de Assis |
 | ..........: Wagner Donizete Gonçalves. |
 | Data......: 30/11/2025 |
 +-------------------------------------------------------------+ */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define TAM_GRANULO 3
#define TAM_MEMORIA 30
#define TRUE 1
#define FALSE 0

// listas encadeadas para representar setores livres e arquivos
typedef struct noSet *ptnoSet;
typedef struct noSet
{
    int inicio, fim;
    ptnoSet prox;
} noSet;

// lista de arquivos: guarda nome, tamanho em caracteres e lista de setores alocados
typedef struct noArq *ptnoArq;
typedef struct noArq
{
    char nome[13];
    int caracteres;
    ptnoSet setores;
    ptnoArq prox;
} noArq;

// Memória simulada do "disco": 30 setores × 3 caracteres cada
typedef char memoria[TAM_MEMORIA][TAM_GRANULO];

/*---------------------------------------------
 * Funcoes auxiliares
 *---------------------------------------------*/

 // Cria um nó representando um intervalo de setores livres
ptnoSet criaNoSet(int inicio, int fim)
{
    ptnoSet novo = (ptnoSet)malloc(sizeof(noSet));
    if (!novo)
    {
        printf("Erro de alocacao de memoria\n");
        exit(EXIT_FAILURE);
    }
    novo->inicio = inicio;
    novo->fim = fim;
    novo->prox = NULL;
    return novo;
}

// Libera lista de setores
void liberaListaSet(ptnoSet S)
{
    ptnoSet aux;
    while (S)
    {
        aux = S;
        S = S->prox;
        free(aux);
    }
}

// Mostra lista de setores
void mostraSetores(ptnoSet S, char *n)
{
    printf("%s = [", n);
    while (S)
    {
        printf("(%d,%d)", S->inicio, S->fim);
        S = S->prox;
        if (S)
            printf(",");
    }
    printf("]\n");
}

// Mostra lista de arquivos
void mostraArquivos(ptnoArq A)
{
    printf("Arquivos:\n");
    while (A)
    {
        printf(" %12s, %2d caracter(es). ", A->nome, A->caracteres);
        mostraSetores(A->setores, "Setores");
        A = A->prox;
    }
    printf("\n");
}

// Mostra conteúdo da memória
void mostraMemoria(memoria Memo)
{
    int i, j;
    for (i = 0; i < TAM_MEMORIA; i++)
    {
        printf("%3d:[", i);
        for (j = 0; j < TAM_GRANULO - 1; j++)
            printf("%c,", Memo[i][j]);
        printf("%c]", Memo[i][TAM_GRANULO - 1]);
        if ((i + 1) % 10 == 0)
            printf("\n");
    }
    printf("\n");
}

// Inicializa área livre e memória vazia
void inicia(ptnoSet *Area, ptnoArq *Arq, memoria Memo)
{
    int i, j;
    *Area = (ptnoSet)malloc(sizeof(noSet));
    (*Area)->inicio = 0;
    (*Area)->fim = TAM_MEMORIA - 1;
    (*Area)->prox = NULL;
    *Arq = NULL;
    for (i = 0; i < TAM_MEMORIA; i++)
        for (j = 0; j < TAM_GRANULO; j++)
            Memo[i][j] = ' ';
}

// Retira setores livres da área (alocação simples)
ptnoSet retirarSetor(ptnoSet *Area, int quantidade)
{
    ptnoSet aux = *Area, anterior = NULL;

    while (aux)
    {
        int tamanho = aux->fim - aux->inicio + 1;

        // Verifica se há espaço suficiente neste bloco
        if (tamanho >= quantidade)
        {
            ptnoSet setorRetirado = criaNoSet(aux->inicio, aux->inicio + quantidade - 1);
            aux->inicio += quantidade;

            // Se o bloco ficou vazio, remove-o da lista
            if (aux->inicio > aux->fim)
            {
                if (anterior)
                {
                    anterior->prox = aux->prox;
                }
                else
                {
                    *Area = aux->prox;
                }
                free(aux);
            }
            return setorRetirado;
        }
        anterior = aux;
        aux = aux->prox;
    }
    return NULL; // Não há espaço suficiente
}

// Devolve setores livres para a área (liberação simples)
void devolverSetor(ptnoSet *Area, ptnoSet setor)
{
    if (!setor)
        return;

    ptnoSet aux = *Area, anterior = NULL;

    // Encontra a posição correta para inserir o setor devolvido
    while (aux && aux->inicio < setor->inicio)
    {
        anterior = aux;
        aux = aux->prox;
    }

    // Tenta fundir com o bloco anterior
    if (anterior && anterior->fim + 1 == setor->inicio)
    {
        anterior->fim = setor->fim;
        free(setor);
        setor = anterior;
    }
    else
    {
        setor->prox = aux;
        if (anterior)
        {
            anterior->prox = setor;
        }
        else
        {
            *Area = setor;
        }
    }

    // Tenta fundir com o bloco seguinte
    if (aux && setor->fim + 1 == aux->inicio)
    {
        setor->fim = aux->fim;
        setor->prox = aux->prox;
        free(aux);
    }
}

// Grava arquivo na memória simulada
void gravarArquivo(ptnoArq *Arq, ptnoSet *Area, memoria Memo, char nome[], char texto[])
{

    if (strlen(nome) >= 13)
    {
        printf("Nome muito longo (max 12 chars)\n");
        return;
    }
    // Calcula setores necessários
    int tam = strlen(texto);
    int setoresNec = (tam + TAM_GRANULO - 1) / TAM_GRANULO;

    ptnoArq aux = *Arq;
    while (aux)
    {
        if (strcmp(aux->nome, nome) == 0)
        {
            printf("Erro: ja existe um arquivo com esse nome. Delete-o primeiro.\n");
            return;
        }
        aux = aux->prox;
    }

    // Lista de setores alocados para o novo arquivo
    ptnoSet lista = NULL, ultimo = NULL;
    int offset = 0;

    // Aloca os setores necessários
    while (setoresNec > 0)
    {
        ptnoSet bloco = retirarSetor(Area, 1); 
        // Verifica se há espaço suficiente
        if (!bloco)
        {
            printf("ERRO: sem espaco no disco. Abortando gravacao.\n");
            ptnoSet s = lista;
            while (s)
            {
                // Devolve os setores já alocados
                ptnoSet p = s->prox; 
                s->prox = NULL;
                devolverSetor(Area, s);
                s = p;
            }
            return;
        }

        // Adiciona o bloco à lista do arquivo
        if (!lista)
            lista = bloco;
        else
            ultimo->prox = bloco;
        ultimo = bloco;

        // Grava os dados no bloco alocado
        int i;
        for (i = 0; i < TAM_GRANULO; i++)
        {
            if (offset < tam)
                Memo[bloco->inicio][i] = texto[offset++];
            else
                Memo[bloco->inicio][i] = ' ';
        }

        setoresNec--; // Decrementa o número de setores necessários
    }

    // Cria o nó do arquivo e insere na lista de arquivos
    ptnoArq novo = (ptnoArq)malloc(sizeof(noArq));
    if (!novo)
    {
        printf("Erro de memoria\n");
        exit(EXIT_FAILURE);
    }
    strcpy(novo->nome, nome);
    novo->caracteres = tam;
    novo->setores = lista;

    // Insere em ordem alfabética
    ptnoArq atual = *Arq, anterior = NULL;
    while (atual && strcmp(atual->nome, nome) < 0)
    {
        anterior = atual;
        atual = atual->prox;
    }
    novo->prox = atual;
    if (anterior)
        anterior->prox = novo;
    else
        *Arq = novo;

    printf("Arquivo '%s' gravado com sucesso! (%d bytes)\n", nome, tam);
}

// Deleta arquivo da memória simulada
void deletarArquivo(ptnoArq *Arq, ptnoSet *Area, memoria Memo, char nome[])
{
    ptnoArq atual = *Arq, anterior = NULL;
    
    // Encontra o arquivo a ser deletado
    while (atual && strcmp(atual->nome, nome) != 0)
    {
        anterior = atual;
        atual = atual->prox;
    }

    if (!atual)
    {
        printf("Erro: arquivo '%s' nao encontrado.\n", nome);
        return;
    }

    // Devolve os setores alocados ao arquivo
    ptnoSet setores = atual->setores;
    while (setores)
    {
        ptnoSet proximo = setores->prox;
        setores->prox = NULL;
        devolverSetor(Area, setores);
        setores = proximo;
    }
    // Remove o arquivo da lista
    if (anterior)
        anterior->prox = atual->prox;
    else
        *Arq = atual->prox;

    free(atual);
    printf("Arquivo '%s' deletado com sucesso!\n", nome);
}

void apresentarArquivo(ptnoArq Arq, memoria Memo, char nome[])
{
    ptnoArq atual = Arq;
    // Encontra o arquivo
    while (atual && strcmp(atual->nome, nome) != 0)
        atual = atual->prox;

    if (!atual)
    {
        printf("Arquivo nao encontrado.\n");
        return;
    }

    printf("Conteudo de %s:\n", nome);
    
    int lidos = 0;
    int total = atual->caracteres;

    // Lê e imprime o conteúdo do arquivo
    ptnoSet s = atual->setores;
    while (s && lidos < total)
    {
        for (int i = 0; i < TAM_GRANULO && lidos < total; i++)
        {
            printf("%c", Memo[s->inicio][i]);
            lidos++;
        }
        s = s->prox;
    }

    printf("\n");
}

void defragmentar(ptnoArq *Arq, ptnoSet *Area, memoria Memo)
{
    // Cria uma nova memória temporária
    memoria novoMemo;
    int i, j;
    for (i = 0; i < TAM_MEMORIA; i++)
        for (j = 0; j < TAM_GRANULO; j++)
            novoMemo[i][j] = ' ';

    int liga = 0; // Próximo setor livre na nova memória

    ptnoArq a = *Arq;
    while (a)
    {
        int total = a->caracteres;
        int setoresNec = (total + TAM_GRANULO - 1) / TAM_GRANULO;

        // Copia os dados do arquivo para um buffer temporário
        char *buffer = (char *)malloc(total + 1);
        int idx = 0;
        ptnoSet s = a->setores;
        while (s)
        {
            for (int k = 0; k < TAM_GRANULO && idx < total; k++)
            {
                buffer[idx++] = Memo[s->inicio][k];
            }
            s = s->prox;
        }
        buffer[total] = '\0';

        // Copia os dados para a nova memória
        int off = 0; 
        for (int b = 0; b < setoresNec; b++)
        {
            for (int k = 0; k < TAM_GRANULO; k++)
            {
                if (off < total)
                {
                    novoMemo[liga + b][k] = buffer[off++];
                }
                else
                {
                    novoMemo[liga + b][k] = ' ';
                }
            }
        }

       // Atualiza a lista de setores do arquivo
        liberaListaSet(a->setores);
        a->setores = criaNoSet(liga, liga + setoresNec - 1);

        liga += setoresNec;
        free(buffer);
        a = a->prox;
    }

    // Copia a nova memória de volta para a memória original
    for (i = 0; i < TAM_MEMORIA; i++)
        for (j = 0; j < TAM_GRANULO; j++)
            Memo[i][j] = novoMemo[i][j];

   
    // Recria a lista de áreas livres
    ptnoSet at = *Area;
    while (at)
    {
        ptnoSet p = at->prox;
        free(at);
        at = p;
    }

    if (liga <= TAM_MEMORIA - 1)
    {
        *Area = criaNoSet(liga, TAM_MEMORIA - 1);
    }
    else
    {
        *Area = NULL; 
    }

    printf("Defragmentacao concluida.\n");
}

/*---------------------------------------------
 * Rotinas para simulacao da FAT
 *---------------------------------------------*/
void ajuda()
{
    printf("\nCOMANDOS\n");
    printf("--------\n");
    printf("G <arquivo.txt> <texto><ENTER>\n");
    printf(" -Grava o <arquivo.txt> e conteúdo <texto> no disco\n");
    printf("D <arquivo.txt>\n");
    printf(" -Deleta o <arquivo.txt> do disco\n");
    printf("A <arquivo.txt>\n");
    printf(" -Apresenta o conteudo do <arquivo.txt>\n");
    printf("M\n");
    printf(" -Mostra as estruturas utilizadas\n");
    printf("H\n");
    printf(" -Apresenta essa lista de comandos\n");
    printf("C\n");
    printf(" -Defragmenta o disco\n");
    printf("F\n");
    printf(" -Fim da simulacao\n");
}

/*------------------------------------------
 * CORPO PRINCIPAL DO PROGRAMA
 *------------------------------------------*/
int main(void)
{
    ptnoSet Area;
    ptnoArq Arq;
    memoria Memo;
    char com[3];
    char nome[13];
    char texto[TAM_MEMORIA * TAM_GRANULO];

    inicia(&Area, &Arq, Memo); // Inicializa estruturas

    printf("\nSIMULADOR DE SISTEMA DE ARQUIVOS FAT - comandos: H para ajuda\n");

    do
    {
        printf("\n=> ");
        scanf("%2s", com);
        com[0] = toupper(com[0]); // Converte para maiúscula

        switch (com[0])
        {
        case 'G':                     
            scanf("%12s", nome);      
            scanf(" %[^\n]s", texto); 
            gravarArquivo(&Arq, &Area, Memo, nome, texto);
            break;

        case 'D': 
            scanf("%12s", nome);
            deletarArquivo(&Arq, &Area, Memo, nome);
            break;

        case 'A': 
            scanf("%12s", nome);
            apresentarArquivo(Arq, Memo, nome);
            break;

        case 'M':
            mostraSetores(Area, "Area");
            mostraArquivos(Arq);
            printf("Memoria:\n");
            mostraMemoria(Memo);
            break;

        case 'H':
            ajuda();
            break;

        case 'C':
            defragmentar(&Arq, &Area, Memo);
            break;

        case 'F':
            // Finalizar
            break;

        default:

            printf("Comando desconhecido. H para ajuda.\n");
            break;
        }

    } while (com[0] != 'F');

    printf("\nFim da Execucao\n\n");
    return 0;
}