/*+-------------------------------------------------------------+
 | UNIFAL – Universidade Federal de Alfenas. |
 | BACHARELADO EM CIENCIA DA COMPUTACAO. |
 | Trabalho..: SIMULACAO DE SISTEMA DE ARQUIVOS FAT |
 | Disciplina: Sistemas Operacionais |
 | Professor.: Romário Borges |
 | Aluno(s)..: Gustavo Andrade Moreira de Assis |
 | Wagner Donizete Gonçalves. |
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

typedef struct noSet *ptnoSet;
typedef struct noSet
{
    int inicio, fim;
    ptnoSet prox;
} noSet;

typedef struct noArq *ptnoArq;
typedef struct noArq
{
    char nome[13];
    int caracteres;
    ptnoSet setores;
    ptnoArq prox;
} noArq;

typedef char memoria[TAM_MEMORIA][TAM_GRANULO];

/*---------------------------------------------
 * Funcoes para manipulacao das estruturas
 *---------------------------------------------*/

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

ptnoSet retirarSetor(ptnoSet *Area, int quantidade)
{
    ptnoSet aux = *Area, anterior = NULL;

    while (aux)
    {
        int tamanho = aux->fim - aux->inicio + 1;
        if (tamanho >= quantidade)
        {
            ptnoSet setorRetirado = criaNoSet(aux->inicio, aux->inicio + quantidade - 1);
            aux->inicio += quantidade;

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

void devolverSetor(ptnoSet *Area, ptnoSet setor)
{
    if (!setor)
        return;

    ptnoSet aux = *Area, anterior = NULL;

    while (aux && aux->inicio < setor->inicio)
    {
        anterior = aux;
        aux = aux->prox;
    }

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

    if (aux && setor->fim + 1 == aux->inicio)
    {
        setor->fim = aux->fim;
        setor->prox = aux->prox;
        free(aux);
    }
}

void gravarArquivo(ptnoArq *Arq, ptnoSet *Area, memoria Memo, char nome[], char texto[])
{

    if (strlen(nome) >= 13)
    {
        printf("Nome muito longo (max 12 chars)\n");
        return;
    }

    int tam = strlen(texto);
    int setoresNec = (tam + TAM_GRANULO - 1) / TAM_GRANULO;

    /* Verifica existencia de arquivo com mesmo nome -> substituir? aqui rejeitamos */
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

    ptnoSet lista = NULL, ultimo = NULL;
    int offset = 0;

    while (setoresNec > 0)
    {
        ptnoSet bloco = retirarSetor(Area, 1); /* aloca 1 setor por vez */

        if (!bloco)
        {
            printf("ERRO: sem espaco no disco. Abortando gravacao.\n");
            /* devolver qualquer bloco ja alocado */
            ptnoSet s = lista;
            while (s)
            {
                ptnoSet p = s->prox;
                s->prox = NULL;
                devolverSetor(Area, s);
                s = p;
            }
            return;
        }

        if (!lista)
            lista = bloco;
        else
            ultimo->prox = bloco;
        ultimo = bloco;

        /* escreve no setor (linha correspondente) os TAM_GRANULO caracteres */
        int i;
        for (i = 0; i < TAM_GRANULO; i++)
        {
            if (offset < tam)
                Memo[bloco->inicio][i] = texto[offset++];
            else
                Memo[bloco->inicio][i] = ' ';
        }

        setoresNec--;
    }

    ptnoArq novo = (ptnoArq)malloc(sizeof(noArq));
    if (!novo)
    {
        printf("Erro de memoria\n");
        exit(EXIT_FAILURE);
    }
    strcpy(novo->nome, nome);
    novo->caracteres = tam;
    novo->setores = lista;

    /* insere em lista ordenada por nome (alfabetica) */
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

void deletarArquivo(ptnoArq *Arq, ptnoSet *Area, memoria Memo, char nome[])
{
    ptnoArq atual = *Arq, anterior = NULL;

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

    ptnoSet setores = atual->setores;
    while (setores)
    {
        ptnoSet proximo = setores->prox;
        setores->prox = NULL;
        devolverSetor(Area, setores);
        setores = proximo;
    }

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
    memoria novoMemo;
    int i, j;
    for (i = 0; i < TAM_MEMORIA; i++)
        for (j = 0; j < TAM_GRANULO; j++)
            novoMemo[i][j] = ' ';

    int liga = 0; /* proximo setor livre onde escrever */

    ptnoArq a = *Arq;
    while (a)
    {
        int total = a->caracteres;
        int setoresNec = (total + TAM_GRANULO - 1) / TAM_GRANULO;

        /* copia conteudo do arquivo antigo para um buffer temporario */
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

        /* escreve em novoMemo a partir de 'liga' setores contiguos */
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

        /* libera lista antiga de setores e cria nova lista com bloco unico */
        liberaListaSet(a->setores);
        a->setores = criaNoSet(liga, liga + setoresNec - 1);

        liga += setoresNec;
        free(buffer);
        a = a->prox;
    }

    /* atualiza Memo com novoMemo */
    for (i = 0; i < TAM_MEMORIA; i++)
        for (j = 0; j < TAM_GRANULO; j++)
            Memo[i][j] = novoMemo[i][j];

    /* reconstrói Area Livre */
    /* libera antiga Area */
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
        *Area = NULL; /* disco cheio */
    }

    printf("Defragmentacao concluida.\n");
}

/*---------------------------------------------
 * Implementar as rotinas para simulacao da FAT
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

    inicia(&Area, &Arq, Memo);

    printf("\nSIMULADOR DE SISTEMA DE ARQUIVOS FAT - comandos: H para ajuda\n");

    do
    {
        printf("\n=> ");
        scanf("%2s", com);
        com[0] = toupper(com[0]);

        switch (com[0])
        {
        case 'G':                     // G nome texto
            scanf("%12s", nome);      // lê nome
            scanf(" %[^\n]s", texto); // lê texto com espaços
            gravarArquivo(&Arq, &Area, Memo, nome, texto);
            break;

        case 'D': // D nome
            scanf("%12s", nome);
            deletarArquivo(&Arq, &Area, Memo, nome);
            break;

        case 'A': // A nome
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