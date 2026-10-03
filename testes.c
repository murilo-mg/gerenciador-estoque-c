#include "armazenamento.h"
#include "produto.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define ARQUIVO_TESTE "teste-estoque.csv"
#define TEMPORARIO_TESTE ARQUIVO_TESTE ".tmp"

static Produto criar_produto(int codigo, const char *nome, int quantidade, int64_t preco)
{
    Produto produto = {0};
    produto.codigo = codigo;
    produto.quantidade = quantidade;
    produto.preco_centavos = preco;
    assert(strlen(nome) < sizeof(produto.nome));
    strcpy(produto.nome, nome);
    return produto;
}

static void escrever_arquivo(const char *caminho, const char *texto)
{
    FILE *arquivo = fopen(caminho, "wb");
    assert(arquivo != NULL);
    assert(fputs(texto, arquivo) >= 0);
    assert(fclose(arquivo) == 0);
}

static void verificar_arquivo(const char *caminho, const char *esperado)
{
    char texto[1024];
    FILE *arquivo = fopen(caminho, "rb");
    size_t tamanho;
    assert(arquivo != NULL);
    tamanho = fread(texto, 1, sizeof(texto) - 1, arquivo);
    texto[tamanho] = '\0';
    assert(!ferror(arquivo));
    assert(feof(arquivo));
    assert(fclose(arquivo) == 0);
    assert(strcmp(texto, esperado) == 0);
}

static void testar_conversoes(void)
{
    int inteiro = 123;
    int64_t centavos = 123;
    char texto[TAMANHO_PRECO];
    const char *invalidos[] = {"", " ", "-1", "+1", "1abc", "999999999999999999999999", "1.2"};
    for (size_t i = 0; i < sizeof(invalidos) / sizeof(invalidos[0]); i++)
    {
        assert(!inteiro_converter(invalidos[i], &inteiro));
        assert(inteiro == 123);
    }
    assert(inteiro_converter(" 0 ", &inteiro) && inteiro == 0);
    assert(snprintf(texto, sizeof(texto), "%d", INT_MAX) > 0);
    assert(inteiro_converter(texto, &inteiro) && inteiro == INT_MAX);
    assert(preco_converter("0", &centavos) && centavos == 0);
    assert(preco_converter(" 12,3 ", &centavos) && centavos == 1230);
    assert(preco_converter("12.34", &centavos) && centavos == 1234);
    assert(preco_converter("0.01", &centavos) && centavos == 1);
    assert(preco_converter("92233720368547758.07", &centavos) && centavos == INT64_MAX);
    assert(preco_formatar(centavos, texto, sizeof(texto)));
    assert(strcmp(texto, "92233720368547758.07") == 0);
    assert(preco_formatar(1001, texto, sizeof(texto)) && strcmp(texto, "10.01") == 0);
    assert(!preco_formatar(-1, texto, sizeof(texto)));
    assert(!preco_formatar(100, texto, 2));
    const char *precos_invalidos[] = {"", " ", "-0.01", "+1", "nan", "inf", "1e3", "1.", ".50", "1.234", "1,2,3", "1.00x", "92233720368547758.08", "92233720368547759", "9999999999999999999999999999999"};
    centavos = 123;
    for (size_t i = 0; i < sizeof(precos_invalidos) / sizeof(precos_invalidos[0]); i++)
    {
        assert(!preco_converter(precos_invalidos[i], &centavos));
        assert(centavos == 123);
    }
}

static void testar_nomes(void)
{
    char longo[TAMANHO_NOME + 1];
    memset(longo, 'a', sizeof(longo) - 1);
    longo[sizeof(longo) - 1] = '\0';
    assert(!nome_valido(longo));
    longo[TAMANHO_NOME - 1] = '\0';
    assert(nome_valido(longo));
    assert(nome_valido("Café, \"especial\" 😀"));
    assert(!nome_valido(""));
    assert(!nome_valido("   "));
    assert(!nome_valido("a\nb"));
    assert(!nome_valido("a\rb"));
    assert(!nome_valido("a\tb"));
    assert(!nome_valido("\033[2J"));
    assert(!nome_valido("\xc0\xaf"));
    assert(!nome_valido("\xe2\x82"));
    assert(!nome_valido("\xed\xa0\x80"));
    assert(!nome_valido("\xf4\x90\x80\x80"));
    assert(!nome_valido("\xc2\x85"));
}

static void testar_regras(void)
{
    Estoque estoque = {0};
    Estoque copia = {0};
    Produto primeiro = criar_produto(1, "Café", 10, 1234);
    Produto segundo = criar_produto(2, "Café especial", 5, 2000);
    Produto alterado = criar_produto(3, "Arroz", 20, 500);
    size_t posicao = 0;
    assert(estoque_buscar_codigo(&estoque, 1) == NULL);
    assert(estoque_cadastrar(&estoque, &primeiro) == ESTOQUE_OK);
    assert(estoque_cadastrar(&estoque, &primeiro) == ESTOQUE_DUPLICADO);
    assert(estoque.total == 1);
    assert(estoque_cadastrar(&estoque, &segundo) == ESTOQUE_OK);
    assert(estoque_buscar_nome(&estoque, "Café", &posicao)->codigo == 1);
    assert(estoque_buscar_nome(&estoque, "Café", &posicao)->codigo == 2);
    assert(estoque_buscar_nome(&estoque, "Café", &posicao) == NULL);
    posicao = 0;
    assert(estoque_buscar_nome(&estoque, "", &posicao) == NULL);
    assert(estoque_editar(&estoque, 1, &segundo) == ESTOQUE_DUPLICADO);
    assert(estoque_editar(&estoque, 9, &alterado) == ESTOQUE_NAO_ENCONTRADO);
    assert(estoque_editar(&estoque, 1, &alterado) == ESTOQUE_OK);
    assert(estoque_buscar_codigo(&estoque, 1) == NULL);
    assert(strcmp(estoque_buscar_codigo(&estoque, 3)->nome, "Arroz") == 0);
    assert(estoque_buscar_codigo(&estoque, 3)->preco_centavos == 500);
    assert(estoque_movimentar(&estoque, 3, 21, 0) == ESTOQUE_SALDO_INSUFICIENTE);
    assert(estoque_buscar_codigo(&estoque, 3)->quantidade == 20);
    assert(estoque_movimentar(&estoque, 3, 20, 0) == ESTOQUE_OK);
    assert(estoque_buscar_codigo(&estoque, 3)->quantidade == 0);
    assert(produto_estoque_baixo(estoque_buscar_codigo(&estoque, 3)));
    assert(estoque_movimentar(&estoque, 3, 6, 1) == ESTOQUE_OK);
    assert(!produto_estoque_baixo(estoque_buscar_codigo(&estoque, 3)));
    assert(estoque_movimentar(&estoque, 3, 0, 1) == ESTOQUE_INVALIDO);
    assert(estoque_movimentar(&estoque, 3, -1, 0) == ESTOQUE_INVALIDO);
    assert(estoque_movimentar(&estoque, 3, 1, 2) == ESTOQUE_INVALIDO);
    assert(estoque_movimentar(&estoque, 99, 1, 0) == ESTOQUE_NAO_ENCONTRADO);
    assert(estoque_movimentar(&estoque, 3, INT_MAX, 1) == ESTOQUE_LIMITE_EXCEDIDO);
    assert(estoque_buscar_codigo(&estoque, 3)->quantidade == 6);
    assert(estoque_copiar(&estoque, &copia) == ESTOQUE_OK);
    assert(copia.total == estoque.total && copia.produtos != estoque.produtos);
    assert(estoque_remover(&estoque, 3) == ESTOQUE_OK);
    assert(estoque.total == 1 && estoque.produtos[0].codigo == 2);
    assert(copia.total == 2 && estoque_buscar_codigo(&copia, 3) != NULL);
    assert(estoque_remover(&estoque, 3) == ESTOQUE_NAO_ENCONTRADO);
    assert(estoque_remover(&estoque, 2) == ESTOQUE_OK && estoque.total == 0);
    primeiro.codigo = 0;
    assert(estoque_cadastrar(&estoque, &primeiro) == ESTOQUE_INVALIDO);
    primeiro.codigo = 1; primeiro.quantidade = -1;
    assert(estoque_cadastrar(&estoque, &primeiro) == ESTOQUE_INVALIDO);
    primeiro.quantidade = 0; primeiro.preco_centavos = -1;
    assert(estoque_cadastrar(&estoque, &primeiro) == ESTOQUE_INVALIDO);
    primeiro.preco_centavos = 0; primeiro.nome[0] = '\0';
    assert(estoque_cadastrar(&estoque, &primeiro) == ESTOQUE_INVALIDO);
    for (int i = 1; i <= 1000; i++)
    {
        Produto produto = criar_produto(i, "Produto", i, i);
        assert(estoque_cadastrar(&estoque, &produto) == ESTOQUE_OK);
    }
    assert(estoque.total == 1000);
    assert(estoque_buscar_codigo(&estoque, 1000)->quantidade == 1000);
    assert(estoque_copiar(&estoque, &copia) == ESTOQUE_OK && copia.total == 1000);
    assert(estoque_copiar(&estoque, &estoque) == ESTOQUE_OK);
    estoque_liberar(&copia);
    estoque_liberar(&estoque);
    assert(estoque.produtos == NULL && estoque.total == 0 && estoque.capacidade == 0);
    estoque_liberar(&estoque);
}

static void testar_csv(void)
{
    Estoque estoque = {0}, carregado = {0};
    size_t invalidas = 99;
    Produto produto = criar_produto(1, "Café, \"especial\"", 5, 1234);
    FILE *avisos = tmpfile();
    assert(avisos != NULL);
    assert(carregar_estoque("arquivo-inexistente.csv", &carregado, &invalidas, avisos) == ARMAZENAMENTO_AUSENTE);
    assert(invalidas == 0 && carregado.total == 0);
    assert(estoque_cadastrar(&estoque, &produto) == ESTOQUE_OK);
    assert(salvar_estoque(ARQUIVO_TESTE, &estoque) == ARMAZENAMENTO_OK);
    verificar_arquivo(ARQUIVO_TESTE, "1,\"Café, \"\"especial\"\"\",5,12.34\n");
    assert(carregar_estoque(ARQUIVO_TESTE, &carregado, &invalidas, avisos) == ARMAZENAMENTO_OK);
    assert(invalidas == 0 && carregado.total == 1);
    assert(strcmp(carregado.produtos[0].nome, produto.nome) == 0);
    assert(carregado.produtos[0].preco_centavos == 1234);
    assert(carregado.produtos[0].quantidade == 5);
    escrever_arquivo(TEMPORARIO_TESTE, "temporário preservado");
    assert(salvar_estoque(ARQUIVO_TESTE, &estoque) == ARMAZENAMENTO_ERRO);
    verificar_arquivo(TEMPORARIO_TESTE, "temporário preservado");
    verificar_arquivo(ARQUIVO_TESTE, "1,\"Café, \"\"especial\"\"\",5,12.34\n");
    assert(remove(TEMPORARIO_TESTE) == 0);
    estoque.produtos[0].quantidade = -1;
    assert(salvar_estoque(ARQUIVO_TESTE, &estoque) == ARMAZENAMENTO_ERRO);
    verificar_arquivo(ARQUIVO_TESTE, "1,\"Café, \"\"especial\"\"\",5,12.34\n");
    estoque.produtos[0].quantidade = 5;
    assert(salvar_estoque("diretorio-inexistente/estoque.csv", &estoque) == ARMAZENAMENTO_ERRO);
    escrever_arquivo(ARQUIVO_TESTE,
        "1,Legado,1,2.50\r\n"
        "linha quebrada\n"
        "1,Duplicado,1,1.00\n"
        "2,,1,1.00\n"
        "2,Nome,-1,1.00\n"
        "2,Nome,1,nan\n"
        "2,\"aspas abertas,1,1.00\n"
        "2,\"aspas\"lixo,1,1.00\n"
        "2,Nome,1,1.00,extra\n"
        "2,No\"me,1,1.00\n"
        "0,Nome,1,1.00\n"
        "2,Nome,999999999999999999999,1.00\n"
        "2,\"Válido, depois\",0,0.01");
    assert(carregar_estoque(ARQUIVO_TESTE, &carregado, &invalidas, avisos) == ARMAZENAMENTO_OK);
    assert(invalidas == 11 && carregado.total == 2);
    assert(estoque_buscar_codigo(&carregado, 2)->preco_centavos == 1);
    assert(fflush(avisos) == 0);
    rewind(avisos);
    char aviso[256];
    assert(fgets(aviso, sizeof(aviso), avisos) != NULL);
    assert(strstr(aviso, "linha 2 ignorada") != NULL);
    assert(fclose(avisos) == 0);
    FILE *arquivo = fopen(ARQUIVO_TESTE, "wb");
    assert(arquivo != NULL);
    for (int i = 0; i < 3000; i++)
        assert(fputc('x', arquivo) != EOF);
    assert(fputs("\n3,Nulo", arquivo) >= 0);
    assert(fputc(0, arquivo) != EOF);
    assert(fputs(",1,1.00\n", arquivo) >= 0);
    for (int i = 1; i <= 250; i++)
        assert(fprintf(arquivo, "%d,Produto,0,0.00\n", i) > 0);
    assert(fclose(arquivo) == 0);
    assert(carregar_estoque(ARQUIVO_TESTE, &carregado, &invalidas, NULL) == ARMAZENAMENTO_OK);
    assert(invalidas == 2 && carregado.total == 250);
    assert(salvar_estoque(ARQUIVO_TESTE, &carregado) == ARMAZENAMENTO_OK);
    assert(carregar_estoque(ARQUIVO_TESTE, &estoque, &invalidas, NULL) == ARMAZENAMENTO_OK);
    assert(invalidas == 0 && estoque.total == 250);
    assert(carregar_estoque("diretorio-inexistente/estoque.csv", &estoque, &invalidas, NULL) == ARMAZENAMENTO_AUSENTE);
    assert(estoque.total == 250);
    estoque_liberar(&estoque);
    assert(salvar_estoque(ARQUIVO_TESTE, &estoque) == ARMAZENAMENTO_OK);
    verificar_arquivo(ARQUIVO_TESTE, "");
    assert(carregar_estoque(ARQUIVO_TESTE, &carregado, &invalidas, NULL) == ARMAZENAMENTO_OK);
    assert(invalidas == 0 && carregado.total == 0);
    estoque_liberar(&carregado);
    assert(remove(ARQUIVO_TESTE) == 0);
}

int main(void)
{
    testar_conversoes();
    testar_nomes();
    testar_regras();
    testar_csv();
    puts("Todos os testes passaram.");
    return 0;
}
