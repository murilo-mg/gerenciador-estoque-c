#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int alocacoes_restantes = -1;

static int deve_falhar(void)
{
    if (alocacoes_restantes < 0)
        return 0;
    if (alocacoes_restantes == 0)
        return 1;
    alocacoes_restantes--;
    return 0;
}

static void *alocar_teste(size_t tamanho)
{
    return deve_falhar() ? NULL : malloc(tamanho);
}

static void *realocar_teste(void *anterior, size_t tamanho)
{
    return deve_falhar() ? NULL : realloc(anterior, tamanho);
}

/* Substituição apenas neste teste: não altera os módulos distribuídos. */
#define malloc alocar_teste
#define realloc realocar_teste
#include "produto.c"
#include "armazenamento.c"
#undef malloc
#undef realloc

int main(void)
{
    Estoque estoque = {0}, copia = {0};
    Produto produto = {1, "Produto", 10, 100};
    Produto *anterior;
    size_t invalidas;
    alocacoes_restantes = 0;
    assert(estoque_cadastrar(&estoque, &produto) == ESTOQUE_SEM_MEMORIA);
    assert(estoque.produtos == NULL && estoque.total == 0);
    alocacoes_restantes = -1;
    for (int codigo = 1; codigo <= 8; codigo++)
    {
        produto.codigo = codigo;
        assert(estoque_cadastrar(&estoque, &produto) == ESTOQUE_OK);
    }
    anterior = estoque.produtos;
    produto.codigo = 9;
    alocacoes_restantes = 0;
    assert(estoque_cadastrar(&estoque, &produto) == ESTOQUE_SEM_MEMORIA);
    assert(estoque.produtos == anterior && estoque.total == 8 && estoque.capacidade == 8);
    alocacoes_restantes = -1;
    assert(estoque_cadastrar(&copia, &produto) == ESTOQUE_OK);
    anterior = copia.produtos;
    alocacoes_restantes = 0;
    assert(estoque_copiar(&estoque, &copia) == ESTOQUE_SEM_MEMORIA);
    assert(copia.produtos == anterior && copia.total == 1 && copia.produtos[0].codigo == 9);
    FILE *arquivo = fopen("memoria.csv", "wb");
    assert(arquivo != NULL);
    for (int codigo = 1; codigo <= 9; codigo++)
        assert(fprintf(arquivo, "%d,Produto,10,1.00\n", codigo) > 0);
    assert(fclose(arquivo) == 0);
    alocacoes_restantes = 0;
    assert(carregar_estoque("memoria.csv", &copia, &invalidas, NULL) == ARMAZENAMENTO_SEM_MEMORIA);
    assert(copia.produtos == anterior && copia.total == 1 && copia.produtos[0].codigo == 9);
    alocacoes_restantes = 1;
    assert(carregar_estoque("memoria.csv", &copia, &invalidas, NULL) == ARMAZENAMENTO_SEM_MEMORIA);
    assert(copia.produtos == anterior && copia.total == 1 && copia.produtos[0].codigo == 9);
    alocacoes_restantes = 0;
    assert(verificar_temporario("memoria.csv") == ARMAZENAMENTO_SEM_MEMORIA);
    assert(salvar_estoque("memoria.csv", &estoque) == ARMAZENAMENTO_SEM_MEMORIA);
    arquivo = fopen("memoria.csv", "rb");
    assert(arquivo != NULL);
    char linha[128];
    int linhas = 0;
    while (fgets(linha, sizeof(linha), arquivo) != NULL)
        linhas++;
    assert(!ferror(arquivo) && linhas == 9);
    assert(fclose(arquivo) == 0);
    assert(remove("memoria.csv") == 0);
    estoque_liberar(&estoque);
    estoque_liberar(&copia);
    puts("Todos os testes de falha de alocação passaram.");
    return 0;
}
