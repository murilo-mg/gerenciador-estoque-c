#include "armazenamento.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define TAMANHO_LINHA 1024

/* Uma linha física por produto; nomes não admitem quebras de linha. */
static int ler_linha(FILE *arquivo, char *linha, size_t tamanho)
{
    size_t usados = 0;
    int caractere;
    int invalida = 0;
    int recebeu = 0;
    while ((caractere = fgetc(arquivo)) != EOF)
    {
        recebeu = 1;
        if (caractere == '\n')
            break;
        if (caractere == '\0' || usados + 1 >= tamanho)
            invalida = 1;
        else
            linha[usados++] = (char)caractere;
    }
    if (ferror(arquivo))
        return -2;
    if (!recebeu)
        return 0;
    if (usados > 0 && linha[usados - 1] == '\r')
        usados--;
    linha[usados] = '\0';
    return invalida ? -1 : 1;
}

static int ler_campo(const char **cursor, char *campo, size_t tamanho, int ultimo)
{
    const char *texto = *cursor;
    size_t usados = 0;
    int entre_aspas = *texto == '"';
    if (entre_aspas)
        texto++;
    for (;;)
    {
        char caractere = *texto;
        if (entre_aspas)
        {
            if (caractere == '\0')
                return 0;
            if (caractere == '"')
            {
                texto++;
                if (*texto != '"')
                    break;
            }
        }
        else
        {
            if (caractere == '\0' || caractere == ',')
                break;
            if (caractere == '"')
                return 0;
        }
        if (usados + 1 >= tamanho)
            return 0;
        campo[usados++] = caractere;
        texto++;
    }
    campo[usados] = '\0';
    if (ultimo ? *texto != '\0' : *texto != ',')
        return 0;
    *cursor = ultimo ? texto : texto + 1;
    return 1;
}

static int interpretar_linha(const char *linha, Produto *produto)
{
    char codigo[32], quantidade[32], preco[TAMANHO_PRECO];
    const char *cursor = linha;
    return ler_campo(&cursor, codigo, sizeof(codigo), 0) &&
           ler_campo(&cursor, produto->nome, sizeof(produto->nome), 0) &&
           ler_campo(&cursor, quantidade, sizeof(quantidade), 0) &&
           ler_campo(&cursor, preco, sizeof(preco), 1) &&
           inteiro_converter(codigo, &produto->codigo) &&
           inteiro_converter(quantidade, &produto->quantidade) &&
           preco_converter(preco, &produto->preco_centavos);
}

ResultadoArmazenamento carregar_estoque(const char *caminho, Estoque *estoque,
                                      size_t *linhas_invalidas, FILE *avisos)
{
    FILE *arquivo;
    Estoque carregado = {0};
    ResultadoArmazenamento resultado = ARMAZENAMENTO_OK;
    size_t numero_linha = 0;
    char linha[TAMANHO_LINHA];
    int leitura;
    if (caminho == NULL || estoque == NULL || linhas_invalidas == NULL)
        return ARMAZENAMENTO_ERRO;
    *linhas_invalidas = 0;
    arquivo = fopen(caminho, "rb");
    if (arquivo == NULL)
        return errno == ENOENT ? ARMAZENAMENTO_AUSENTE : ARMAZENAMENTO_ERRO;
    while ((leitura = ler_linha(arquivo, linha, sizeof(linha))) != 0)
    {
        Produto produto = {0};
        ResultadoEstoque cadastro = ESTOQUE_INVALIDO;
        if (leitura == -2)
        {
            resultado = ARMAZENAMENTO_ERRO;
            break;
        }
        if (numero_linha == SIZE_MAX)
        {
            resultado = ARMAZENAMENTO_ERRO;
            break;
        }
        numero_linha++;
        if (leitura == 1 && interpretar_linha(linha, &produto))
            cadastro = estoque_cadastrar(&carregado, &produto);
        if (cadastro == ESTOQUE_SEM_MEMORIA)
        {
            resultado = ARMAZENAMENTO_SEM_MEMORIA;
            break;
        }
        if (cadastro != ESTOQUE_OK)
        {
            (*linhas_invalidas)++;
            if (avisos != NULL)
                fprintf(avisos, "Aviso: linha %zu ignorada: %s\n", numero_linha,
                        estoque_mensagem(cadastro));
        }
    }
    if (fclose(arquivo) != 0)
        resultado = ARMAZENAMENTO_ERRO;
    if (resultado == ARMAZENAMENTO_OK)
    {
        estoque_liberar(estoque);
        *estoque = carregado;
    }
    else
        estoque_liberar(&carregado);
    return resultado;
}

static int escrever_produto(FILE *arquivo, const Produto *produto)
{
    char preco[TAMANHO_PRECO];
    if (produto->codigo <= 0 || produto->quantidade < 0 || !nome_valido(produto->nome) ||
        !preco_formatar(produto->preco_centavos, preco, sizeof(preco)))
        return 0;
    if (fprintf(arquivo, "%d,\"", produto->codigo) < 0)
        return 0;
    for (const char *letra = produto->nome; *letra != '\0'; letra++)
    {
        if (*letra == '"' && fputc('"', arquivo) == EOF)
            return 0;
        if (fputc((unsigned char)*letra, arquivo) == EOF)
            return 0;
    }
    return fprintf(arquivo, "\",%d,%s\n", produto->quantidade, preco) >= 0;
}

static char *caminho_temporario(const char *caminho)
{
    size_t tamanho = strlen(caminho);
    char *temporario;
    if (tamanho > SIZE_MAX - sizeof(".tmp"))
        return NULL;
    temporario = malloc(tamanho + sizeof(".tmp"));
    if (temporario == NULL)
        return NULL;
    memcpy(temporario, caminho, tamanho);
    memcpy(temporario + tamanho, ".tmp", sizeof(".tmp"));
    return temporario;
}

ResultadoArmazenamento verificar_temporario(const char *caminho)
{
    char *temporario;
    FILE *arquivo;
    ResultadoArmazenamento resultado;
    if (caminho == NULL || *caminho == '\0')
        return ARMAZENAMENTO_ERRO;
    temporario = caminho_temporario(caminho);
    if (temporario == NULL)
        return ARMAZENAMENTO_SEM_MEMORIA;
    arquivo = fopen(temporario, "rb");
    if (arquivo == NULL)
        resultado = errno == ENOENT ? ARMAZENAMENTO_AUSENTE : ARMAZENAMENTO_ERRO;
    else
        resultado = fclose(arquivo) == 0 ? ARMAZENAMENTO_OK : ARMAZENAMENTO_ERRO;
    free(temporario);
    return resultado;
}

ResultadoArmazenamento salvar_estoque(const char *caminho, const Estoque *estoque)
{
    char *temporario;
    FILE *arquivo;
    int sucesso = 1;
    if (caminho == NULL || *caminho == '\0' || estoque == NULL)
        return ARMAZENAMENTO_ERRO;
    temporario = caminho_temporario(caminho);
    if (temporario == NULL)
        return ARMAZENAMENTO_SEM_MEMORIA;
    /* O modo exclusivo de C11 evita truncar temporários de outra execução. */
    arquivo = fopen(temporario, "wbx");
    if (arquivo == NULL)
    {
        free(temporario);
        return ARMAZENAMENTO_ERRO;
    }
    for (size_t i = 0; i < estoque->total; i++)
    {
        if (!escrever_produto(arquivo, &estoque->produtos[i]))
        {
            sucesso = 0;
            break;
        }
    }
    if (fflush(arquivo) != 0)
        sucesso = 0;
    if (fclose(arquivo) != 0)
        sucesso = 0;
    if (sucesso && rename(temporario, caminho) != 0)
        sucesso = 0;
    if (!sucesso)
        (void)remove(temporario);
    free(temporario);
    return sucesso ? ARMAZENAMENTO_OK : ARMAZENAMENTO_ERRO;
}
