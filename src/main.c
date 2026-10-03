#include "armazenamento.h"
#include "produto.h"

#include <limits.h>
#include <stdio.h>

#define NOME_ARQUIVO "estoque.csv"

/* Consome a linha inteira, mesmo quando ela não cabe ou contém um byte nulo. */
static int ler_texto(const char *mensagem, char *texto, size_t tamanho)
{
    size_t usados;
    int caractere;
    int invalida;
    int recebeu;
    for (;;)
    {
        usados = 0;
        invalida = 0;
        recebeu = 0;
        fputs(mensagem, stdout);
        fflush(stdout);
        while ((caractere = getchar()) != EOF)
        {
            recebeu = 1;
            if (caractere == '\n')
                break;
            if (caractere == '\0' || usados + 1 >= tamanho)
                invalida = 1;
            else
                texto[usados++] = (char)caractere;
        }
        if (ferror(stdin) || (!recebeu && caractere == EOF))
            return 0;
        if (usados > 0 && texto[usados - 1] == '\r')
            usados--;
        texto[usados] = '\0';
        if (!invalida)
            return 1;
        puts("Entrada muito longa ou inválida. Tente novamente.");
    }
}

static int ler_inteiro(const char *mensagem, int minimo, int *valor)
{
    char texto[64];
    while (ler_texto(mensagem, texto, sizeof(texto)))
    {
        if (inteiro_converter(texto, valor) && *valor >= minimo)
            return 1;
        printf("Digite um inteiro entre %d e %d.\n", minimo, INT_MAX);
    }
    return 0;
}

static int ler_nome(const char *mensagem, char *nome)
{
    while (ler_texto(mensagem, nome, TAMANHO_NOME))
    {
        if (nome_valido(nome))
            return 1;
        puts("Nome inválido: use texto UTF-8 não vazio, sem caracteres de controle.");
    }
    return 0;
}

static int ler_produto(Produto *produto)
{
    char texto[64];
    if (!ler_inteiro("Código: ", 1, &produto->codigo) ||
        !ler_nome("Nome: ", produto->nome) ||
        !ler_inteiro("Quantidade: ", 0, &produto->quantidade))
        return 0;
    while (ler_texto("Preço (R$): ", texto, sizeof(texto)))
    {
        if (preco_converter(texto, &produto->preco_centavos))
            return 1;
        puts("Preço inválido: use valor não negativo com até duas casas decimais.");
    }
    return 0;
}

static void exibir_produto(const Produto *produto)
{
    char preco[TAMANHO_PRECO];
    if (!preco_formatar(produto->preco_centavos, preco, sizeof(preco)))
        return;
    printf("Código: %d | Nome: %s | Quantidade: %d | Preço: R$ %s%s\n",
           produto->codigo, produto->nome, produto->quantidade, preco,
           produto_estoque_baixo(produto) ? " | ESTOQUE BAIXO" : "");
}

static void listar_produtos(const Estoque *estoque, int apenas_baixos)
{
    size_t encontrados = 0;
    for (size_t i = 0; i < estoque->total; i++)
    {
        if (!apenas_baixos || produto_estoque_baixo(&estoque->produtos[i]))
        {
            exibir_produto(&estoque->produtos[i]);
            encontrados++;
        }
    }
    if (encontrados == 0)
        puts(apenas_baixos ? "Nenhum produto com estoque baixo." : "Nenhum produto cadastrado.");
}

static void buscar_codigo(const Estoque *estoque)
{
    int codigo;
    const Produto *produto;
    if (!ler_inteiro("Código: ", 1, &codigo))
        return;
    produto = estoque_buscar_codigo(estoque, codigo);
    if (produto != NULL)
        exibir_produto(produto);
    else
        puts("Produto não encontrado.");
}

static void buscar_nome(const Estoque *estoque)
{
    char nome[TAMANHO_NOME];
    size_t posicao = 0;
    size_t encontrados = 0;
    const Produto *produto;
    if (!ler_nome("Nome ou parte do nome: ", nome))
        return;
    while ((produto = estoque_buscar_nome(estoque, nome, &posicao)) != NULL)
    {
        exibir_produto(produto);
        encontrados++;
    }
    if (encontrados == 0)
        puts("Produto não encontrado.");
}

static void alterar_estoque(Estoque *estoque, int opcao)
{
    Estoque alterado = {0};
    Produto produto = {0};
    int codigo = 0;
    int quantidade = 0;
    ResultadoEstoque resultado;
    if (opcao != 1)
    {
        if (!ler_inteiro("Código do produto: ", 1, &codigo))
            return;
        if (estoque_buscar_codigo(estoque, codigo) == NULL)
        {
            puts("Produto não encontrado.");
            return;
        }
    }
    if ((opcao == 1 || opcao == 5) && !ler_produto(&produto))
        return;
    if ((opcao == 7 || opcao == 8) && !ler_inteiro("Quantidade da movimentação: ", 1, &quantidade))
        return;
    resultado = estoque_copiar(estoque, &alterado);
    if (resultado != ESTOQUE_OK)
    {
        puts(estoque_mensagem(resultado));
        return;
    }
    switch (opcao)
    {
    case 1: resultado = estoque_cadastrar(&alterado, &produto); break;
    case 5: resultado = estoque_editar(&alterado, codigo, &produto); break;
    case 6: resultado = estoque_remover(&alterado, codigo); break;
    case 7: case 8:
        resultado = estoque_movimentar(&alterado, codigo, quantidade, opcao == 7);
        break;
    default: resultado = ESTOQUE_INVALIDO; break;
    }
    if (resultado != ESTOQUE_OK)
        puts(estoque_mensagem(resultado));
    else if (salvar_estoque(NOME_ARQUIVO, &alterado) != ARMAZENAMENTO_OK)
        fputs("Erro ao salvar. A alteração foi cancelada; verifique permissões e estoque.csv.tmp.\n", stderr);
    else
    {
        estoque_liberar(estoque);
        *estoque = alterado;
        estoque_inicializar(&alterado);
        puts("Alteração salva com sucesso.");
        if (opcao != 6)
        {
            const Produto *salvo = estoque_buscar_codigo(estoque,
                                      opcao == 1 || opcao == 5 ? produto.codigo : codigo);
            if (salvo != NULL && produto_estoque_baixo(salvo))
                exibir_produto(salvo);
        }
    }
    estoque_liberar(&alterado);
}

static void exibir_menu(void)
{
    puts("\n=== GERENCIADOR DE ESTOQUE ===\n"
         "1. Cadastrar produto\n"
         "2. Listar produtos\n"
         "3. Buscar por código\n"
         "4. Sair\n"
         "5. Editar produto\n"
         "6. Remover produto\n"
         "7. Entrada de quantidade\n"
         "8. Saída de quantidade\n"
         "9. Buscar por nome\n"
         "10. Listar estoque baixo (até 5 unidades)");
}

int main(void)
{
    Estoque estoque = {0};
    size_t linhas_invalidas;
    ResultadoArmazenamento carga = carregar_estoque(NOME_ARQUIVO, &estoque,
                                                   &linhas_invalidas, stderr);
    int opcao;
    int retorno = 0;
    if (carga != ARMAZENAMENTO_OK && carga != ARMAZENAMENTO_AUSENTE)
    {
        fputs("Erro ao carregar estoque. O arquivo foi preservado.\n", stderr);
        estoque_liberar(&estoque);
        return 1;
    }
    if (carga == ARMAZENAMENTO_AUSENTE)
        puts("Arquivo ausente: iniciando estoque vazio.");
    if (linhas_invalidas > 0)
        fputs("CSV contém linhas inválidas. Consulta disponível; corrija o arquivo e reinicie para alterar.\n", stderr);
    for (size_t i = 0; i < estoque.total; i++)
        if (produto_estoque_baixo(&estoque.produtos[i]))
            exibir_produto(&estoque.produtos[i]);
    for (;;)
    {
        exibir_menu();
        if (!ler_inteiro("Opção: ", 1, &opcao) || opcao == 4)
            break;
        switch (opcao)
        {
        case 1: case 5: case 6: case 7: case 8:
            if (linhas_invalidas > 0)
                puts("Alteração bloqueada: corrija as linhas inválidas do CSV e reinicie.");
            else
                alterar_estoque(&estoque, opcao);
            break;
        case 2: listar_produtos(&estoque, 0); break;
        case 3: buscar_codigo(&estoque); break;
        case 9: buscar_nome(&estoque); break;
        case 10: listar_produtos(&estoque, 1); break;
        default: puts("Opção inválida."); break;
        }
    }
    if (ferror(stdin))
    {
        fputs("Erro de leitura da entrada.\n", stderr);
        retorno = 1;
    }
    puts("Encerrando. As alterações confirmadas já foram salvas.");
    estoque_liberar(&estoque);
    return retorno;
}
