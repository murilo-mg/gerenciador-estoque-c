#include "produto.h"

#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void estoque_inicializar(Estoque *estoque)
{
    *estoque = (Estoque){0};
}

void estoque_liberar(Estoque *estoque)
{
    free(estoque->produtos);
    estoque_inicializar(estoque);
}

/* Faixas White_Space e Default_Ignorable_Code_Point do Unicode 17.0.
 * https://www.unicode.org/Public/17.0.0/ucd/PropList.txt
 * https://www.unicode.org/Public/17.0.0/ucd/DerivedCoreProperties.txt */
static int ponto_sem_conteudo(uint32_t ponto)
{
    return ponto == 0xa0 || ponto == 0x1680 ||
           (ponto >= 0x2000 && ponto <= 0x200a) ||
           ponto == 0x202f || ponto == 0x205f || ponto == 0x3000 ||
           ponto == 0xad || ponto == 0x34f || ponto == 0x61c ||
           (ponto >= 0x115f && ponto <= 0x1160) ||
           (ponto >= 0x17b4 && ponto <= 0x17b5) ||
           (ponto >= 0x180b && ponto <= 0x180f) ||
           (ponto >= 0x200b && ponto <= 0x200f) ||
           (ponto >= 0x202a && ponto <= 0x202e) ||
           (ponto >= 0x2060 && ponto <= 0x206f) ||
           ponto == 0x3164 || (ponto >= 0xfe00 && ponto <= 0xfe0f) ||
           ponto == 0xfeff || ponto == 0xffa0 ||
           (ponto >= 0xfff0 && ponto <= 0xfff8) ||
           (ponto >= 0x1bca0 && ponto <= 0x1bca3) ||
           (ponto >= 0x1d173 && ponto <= 0x1d17a) ||
           (ponto >= 0xe0000 && ponto <= 0xe0fff);
}

/* Bloqueia controles de direção e caracteres que ocultam ou quebram o nome.
 * Junções e seletores de variação continuam disponíveis para escrita e emojis. */
static int ponto_proibido(uint32_t ponto)
{
    return ponto == 0xad || ponto == 0x61c || ponto == 0x200b ||
           (ponto >= 0x200e && ponto <= 0x200f) ||
           (ponto >= 0x2028 && ponto <= 0x202e) ||
           (ponto >= 0x2060 && ponto <= 0x2064) ||
           (ponto >= 0x2066 && ponto <= 0x2069) || ponto == 0xfeff;
}

/* Rejeita controles, sequências UTF-8 inválidas e nomes sem conteúdo visível. */
int nome_valido(const char *nome)
{
    size_t tamanho = 0;
    int visivel = 0;
    if (nome == NULL)
        return 0;
    while (tamanho < TAMANHO_NOME && nome[tamanho] != '\0')
        tamanho++;
    if (tamanho == 0 || tamanho == TAMANHO_NOME)
        return 0;
    for (size_t i = 0; i < tamanho;)
    {
        unsigned char primeiro = (unsigned char)nome[i++];
        uint32_t ponto;
        uint32_t minimo;
        size_t restantes;
        if (primeiro < 0x80)
        {
            if (primeiro < 0x20 || primeiro == 0x7f)
                return 0;
            if (primeiro != ' ')
                visivel = 1;
            continue;
        }
        if (primeiro >= 0xc2 && primeiro <= 0xdf)
        {
            ponto = primeiro & 0x1f; minimo = 0x80; restantes = 1;
        }
        else if (primeiro >= 0xe0 && primeiro <= 0xef)
        {
            ponto = primeiro & 0x0f; minimo = 0x800; restantes = 2;
        }
        else if (primeiro >= 0xf0 && primeiro <= 0xf4)
        {
            ponto = primeiro & 0x07; minimo = 0x10000; restantes = 3;
        }
        else
            return 0;
        if (restantes > tamanho - i)
            return 0;
        while (restantes-- > 0)
        {
            unsigned char seguinte = (unsigned char)nome[i++];
            if ((seguinte & 0xc0) != 0x80)
                return 0;
            ponto = (ponto << 6) | (seguinte & 0x3f);
        }
        if (ponto < minimo || ponto > 0x10ffff ||
            (ponto >= 0xd800 && ponto <= 0xdfff) ||
            (ponto >= 0x80 && ponto <= 0x9f) || ponto_proibido(ponto))
            return 0;
        if (!ponto_sem_conteudo(ponto))
            visivel = 1;
    }
    return visivel;
}

static int produto_valido(const Produto *produto)
{
    return produto != NULL && produto->codigo > 0 && nome_valido(produto->nome) &&
           produto->quantidade >= 0 && produto->preco_centavos >= 0;
}

int inteiro_converter(const char *texto, int *valor)
{
    int numero = 0;
    if (texto == NULL || valor == NULL)
        return 0;
    while (isspace((unsigned char)*texto))
        texto++;
    if (*texto < '0' || *texto > '9')
        return 0;
    while (*texto >= '0' && *texto <= '9')
    {
        int digito = *texto++ - '0';
        if (numero > (INT_MAX - digito) / 10)
            return 0;
        numero = numero * 10 + digito;
    }
    while (isspace((unsigned char)*texto))
        texto++;
    if (*texto != '\0')
        return 0;
    *valor = numero;
    return 1;
}

int preco_converter(const char *texto, int64_t *centavos)
{
    int64_t reais = 0;
    int fracao = 0;
    if (texto == NULL || centavos == NULL)
        return 0;
    while (isspace((unsigned char)*texto))
        texto++;
    if (*texto < '0' || *texto > '9')
        return 0;
    while (*texto >= '0' && *texto <= '9')
    {
        int digito = *texto++ - '0';
        if (reais > (INT64_MAX / 100 - digito) / 10)
            return 0;
        reais = reais * 10 + digito;
    }
    if (*texto == '.' || *texto == ',')
    {
        texto++;
        if (*texto < '0' || *texto > '9')
            return 0;
        fracao = (*texto++ - '0') * 10;
        if (*texto >= '0' && *texto <= '9')
            fracao += *texto++ - '0';
    }
    while (isspace((unsigned char)*texto))
        texto++;
    if (*texto != '\0' || reais > (INT64_MAX - fracao) / 100)
        return 0;
    *centavos = reais * 100 + fracao;
    return 1;
}

int preco_formatar(int64_t centavos, char *texto, size_t tamanho)
{
    int escrito;
    if (centavos < 0 || texto == NULL || tamanho == 0)
        return 0;
    escrito = snprintf(texto, tamanho, "%" PRId64 ".%02" PRId64,
                       centavos / 100, centavos % 100);
    return escrito >= 0 && (size_t)escrito < tamanho;
}

static size_t buscar_indice(const Estoque *estoque, int codigo)
{
    for (size_t i = 0; i < estoque->total; i++)
        if (estoque->produtos[i].codigo == codigo)
            return i;
    return estoque->total;
}

const Produto *estoque_buscar_codigo(const Estoque *estoque, int codigo)
{
    size_t indice = buscar_indice(estoque, codigo);
    return indice == estoque->total ? NULL : &estoque->produtos[indice];
}

const Produto *estoque_buscar_nome(const Estoque *estoque, const char *nome, size_t *posicao)
{
    if (nome == NULL || posicao == NULL || !nome_valido(nome))
        return NULL;
    while (*posicao < estoque->total)
    {
        const Produto *produto = &estoque->produtos[(*posicao)++];
        if (strstr(produto->nome, nome) != NULL)
            return produto;
    }
    return NULL;
}

ResultadoEstoque estoque_copiar(const Estoque *origem, Estoque *destino)
{
    Estoque copia = {0};
    if (origem == destino)
        return ESTOQUE_OK;
    if (origem->total > 0)
    {
        if (origem->total > SIZE_MAX / sizeof(Produto))
            return ESTOQUE_SEM_MEMORIA;
        copia.produtos = malloc(origem->total * sizeof(Produto));
        if (copia.produtos == NULL)
            return ESTOQUE_SEM_MEMORIA;
        memcpy(copia.produtos, origem->produtos, origem->total * sizeof(Produto));
        copia.total = copia.capacidade = origem->total;
    }
    estoque_liberar(destino);
    *destino = copia;
    return ESTOQUE_OK;
}

ResultadoEstoque estoque_cadastrar(Estoque *estoque, const Produto *produto)
{
    Produto copia;
    if (!produto_valido(produto))
        return ESTOQUE_INVALIDO;
    if (estoque_buscar_codigo(estoque, produto->codigo) != NULL)
        return ESTOQUE_DUPLICADO;
    copia = *produto;
    if (estoque->total == estoque->capacidade)
    {
        size_t limite = SIZE_MAX / sizeof(Produto);
        size_t capacidade;
        Produto *novos;
        if (estoque->capacidade >= limite)
            return ESTOQUE_SEM_MEMORIA;
        capacidade = estoque->capacidade == 0 ? 8 :
                     estoque->capacidade > limite / 2 ? limite : estoque->capacidade * 2;
        novos = realloc(estoque->produtos, capacidade * sizeof(Produto));
        if (novos == NULL)
            return ESTOQUE_SEM_MEMORIA;
        estoque->produtos = novos;
        estoque->capacidade = capacidade;
    }
    estoque->produtos[estoque->total++] = copia;
    return ESTOQUE_OK;
}

ResultadoEstoque estoque_editar(Estoque *estoque, int codigo, const Produto *produto)
{
    size_t indice = buscar_indice(estoque, codigo);
    if (!produto_valido(produto))
        return ESTOQUE_INVALIDO;
    if (indice == estoque->total)
        return ESTOQUE_NAO_ENCONTRADO;
    if (produto->codigo != codigo && estoque_buscar_codigo(estoque, produto->codigo) != NULL)
        return ESTOQUE_DUPLICADO;
    estoque->produtos[indice] = *produto;
    return ESTOQUE_OK;
}

ResultadoEstoque estoque_remover(Estoque *estoque, int codigo)
{
    size_t indice = buscar_indice(estoque, codigo);
    if (indice == estoque->total)
        return ESTOQUE_NAO_ENCONTRADO;
    memmove(&estoque->produtos[indice], &estoque->produtos[indice + 1],
            (estoque->total - indice - 1) * sizeof(Produto));
    estoque->total--;
    return ESTOQUE_OK;
}

ResultadoEstoque estoque_movimentar(Estoque *estoque, int codigo, int quantidade, int entrada)
{
    size_t indice = buscar_indice(estoque, codigo);
    Produto *produto;
    if (quantidade <= 0 || (entrada != 0 && entrada != 1))
        return ESTOQUE_INVALIDO;
    if (indice == estoque->total)
        return ESTOQUE_NAO_ENCONTRADO;
    produto = &estoque->produtos[indice];
    if (entrada)
    {
        if (produto->quantidade > INT_MAX - quantidade)
            return ESTOQUE_LIMITE_EXCEDIDO;
        produto->quantidade += quantidade;
    }
    else
    {
        if (produto->quantidade < quantidade)
            return ESTOQUE_SALDO_INSUFICIENTE;
        produto->quantidade -= quantidade;
    }
    return ESTOQUE_OK;
}

int produto_estoque_baixo(const Produto *produto)
{
    return produto->quantidade <= LIMITE_ESTOQUE_BAIXO;
}

const char *estoque_mensagem(ResultadoEstoque resultado)
{
    switch (resultado)
    {
    case ESTOQUE_OK: return "Operação concluída.";
    case ESTOQUE_INVALIDO: return "Dados inválidos.";
    case ESTOQUE_DUPLICADO: return "Código já cadastrado.";
    case ESTOQUE_NAO_ENCONTRADO: return "Produto não encontrado.";
    case ESTOQUE_SEM_MEMORIA: return "Memória insuficiente.";
    case ESTOQUE_SALDO_INSUFICIENTE: return "Saldo insuficiente.";
    case ESTOQUE_LIMITE_EXCEDIDO: return "Quantidade excede o limite permitido.";
    }
    return "Erro desconhecido.";
}
