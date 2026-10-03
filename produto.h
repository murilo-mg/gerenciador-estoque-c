#ifndef PRODUTO_H
#define PRODUTO_H

#include <stddef.h>
#include <stdint.h>

#define TAMANHO_NOME 128
#define TAMANHO_PRECO 32
#define LIMITE_ESTOQUE_BAIXO 5

typedef struct
{
    int codigo;
    char nome[TAMANHO_NOME];
    int quantidade;
    int64_t preco_centavos;
} Produto;

typedef struct
{
    Produto *produtos;
    size_t total;
    size_t capacidade;
} Estoque;

typedef enum
{
    ESTOQUE_OK,
    ESTOQUE_INVALIDO,
    ESTOQUE_DUPLICADO,
    ESTOQUE_NAO_ENCONTRADO,
    ESTOQUE_SEM_MEMORIA,
    ESTOQUE_SALDO_INSUFICIENTE,
    ESTOQUE_LIMITE_EXCEDIDO
} ResultadoEstoque;

void estoque_inicializar(Estoque *estoque);
void estoque_liberar(Estoque *estoque);
ResultadoEstoque estoque_copiar(const Estoque *origem, Estoque *destino);
ResultadoEstoque estoque_cadastrar(Estoque *estoque, const Produto *produto);
ResultadoEstoque estoque_editar(Estoque *estoque, int codigo, const Produto *produto);
ResultadoEstoque estoque_remover(Estoque *estoque, int codigo);
ResultadoEstoque estoque_movimentar(Estoque *estoque, int codigo, int quantidade, int entrada);
const Produto *estoque_buscar_codigo(const Estoque *estoque, int codigo);
const Produto *estoque_buscar_nome(const Estoque *estoque, const char *nome, size_t *posicao);
int produto_estoque_baixo(const Produto *produto);
int nome_valido(const char *nome);
int inteiro_converter(const char *texto, int *valor);
int preco_converter(const char *texto, int64_t *centavos);
int preco_formatar(int64_t centavos, char *texto, size_t tamanho);
const char *estoque_mensagem(ResultadoEstoque resultado);

#endif
