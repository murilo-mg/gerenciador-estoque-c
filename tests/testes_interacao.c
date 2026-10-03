/* Exercita o mesmo menu distribuído, com entrada e saída em arquivos C padrão. */
#define main executar_menu
#include "main.c"
#undef main

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void escrever(const char *caminho, const char *texto)
{
    FILE *arquivo = fopen(caminho, "wb");
    assert(arquivo != NULL);
    assert(fputs(texto, arquivo) >= 0);
    assert(fclose(arquivo) == 0);
}

static void executar(const char *entrada)
{
    escrever("entrada.txt", entrada);
    assert(freopen("entrada.txt", "rb", stdin) != NULL);
    assert(freopen("saida.txt", "wb", stdout) != NULL);
    assert(executar_menu() == 0);
    assert(fflush(stdout) == 0);
}

static int contem(const char *caminho, const char *trecho)
{
    char texto[32768];
    FILE *arquivo = fopen(caminho, "rb");
    size_t lidos;
    assert(arquivo != NULL);
    lidos = fread(texto, 1, sizeof(texto) - 1, arquivo);
    texto[lidos] = '\0';
    assert(!ferror(arquivo) && feof(arquivo));
    assert(fclose(arquivo) == 0);
    return strstr(texto, trecho) != NULL;
}

static void testar_menu(void)
{
    executar("texto\n999999999999999999999\n99\n"
             "1\n-1\n0\n1\n\n   \nCafé, \"especial\"\n-1\n10\nnan\n1.234\n12,34\n"
             "1\n1\nDuplicado\n1\n2.00\n"
             "3\n1\n9\nCafé\n"
             "8\n1\n11\n7\n1\n5\n8\n1\n10\n10\n"
             "5\n1\n2\nArroz\n2\n3.45\n2\n6\n2\n2\n4\n");
    assert(contem("saida.txt", "Opção inválida."));
    assert(contem("saida.txt", "Nome inválido"));
    assert(contem("saida.txt", "Preço inválido"));
    assert(contem("saida.txt", "Código já cadastrado."));
    assert(contem("saida.txt", "Saldo insuficiente."));
    assert(contem("saida.txt", "ESTOQUE BAIXO"));
    assert(contem("saida.txt", "Nome: Arroz | Quantidade: 2 | Preço: R$ 3.45"));
    assert(contem("saida.txt", "Nenhum produto cadastrado."));
    assert(!contem("estoque.csv", "Arroz"));
    assert(remove("estoque.csv") == 0);
}

static void testar_fim_entrada(void)
{
    const char *entradas[] = {"", "inválida", "1\n", "1\n1\n", "1\n1\nNome\n", "1\n1\nNome\n1\n", "3\n", "5\n", "9\n"};
    for (size_t i = 0; i < sizeof(entradas) / sizeof(entradas[0]); i++)
    {
        executar(entradas[i]);
        assert(contem("saida.txt", "Encerrando."));
        FILE *arquivo = fopen("estoque.csv", "rb");
        assert(arquivo == NULL);
    }
}

static void testar_preservacao(void)
{
    escrever("estoque.csv", "1,Válido,10,2.50\ncorrompido\n2,Outro,10,3.00\n");
    executar("2\n1\n4\n");
    assert(contem("saida.txt", "Código: 2 | Nome: Outro"));
    assert(contem("saida.txt", "Alteração bloqueada"));
    assert(contem("estoque.csv", "corrompido"));
    assert(remove("estoque.csv") == 0);
    escrever("estoque.csv.tmp", "temporário de outra execução");
    executar("1\n1\nProduto\n1\n2.00\n2\n4\n");
    assert(contem("saida.txt", "Nenhum produto cadastrado."));
    assert(contem("estoque.csv.tmp", "temporário de outra execução"));
    assert(remove("estoque.csv.tmp") == 0);
}

static void testar_entradas_extensas(void)
{
    FILE *arquivo = fopen("entrada.txt", "wb");
    assert(arquivo != NULL);
    for (int i = 0; i < 5000; i++)
        assert(fputc('9', arquivo) != EOF);
    assert(fputs("\n1\n1\n", arquivo) >= 0);
    for (int i = 0; i < 5000; i++)
        assert(fputc('a', arquivo) != EOF);
    assert(fputs("\nNome", arquivo) >= 0);
    assert(fputc(0, arquivo) != EOF);
    assert(fputs("injetado\n\033[2J\n\xc0\xaf\nNome válido\n0\n0.01\n4", arquivo) >= 0);
    assert(fclose(arquivo) == 0);
    assert(freopen("entrada.txt", "rb", stdin) != NULL);
    assert(freopen("saida.txt", "wb", stdout) != NULL);
    assert(executar_menu() == 0);
    assert(fflush(stdout) == 0);
    assert(contem("saida.txt", "Entrada muito longa ou inválida"));
    assert(contem("estoque.csv", "1,\"Nome válido\",0,0.01"));
    assert(remove("estoque.csv") == 0);
}

int main(void)
{
    testar_menu();
    testar_fim_entrada();
    testar_preservacao();
    testar_entradas_extensas();
    assert(fclose(stdin) == 0);
    assert(fclose(stdout) == 0);
    assert(remove("entrada.txt") == 0);
    assert(remove("saida.txt") == 0);
    fputs("Todos os testes de interação passaram.\n", stderr);
    return 0;
}
