#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PRODUTOS 100
#define NOME_ARQUIVO "estoque.csv"

typedef struct {
    int codigo;
    char nome[50];
    int quantidade;
    float preco;
} Produto;

void limpar_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void carregar_estoque(Produto lista[], int *total) {
    FILE *arquivo = fopen(NOME_ARQUIVO, "r");
    if (arquivo == NULL) return;

    *total = 0;
    while (fscanf(arquivo, "%d,%49[^,],%d,%f\n", 
                  &lista[*total].codigo, 
                  lista[*total].nome, 
                  &lista[*total].quantidade, 
                  &lista[*total].preco) == 4) {
        (*total)++;
        if (*total >= MAX_PRODUTOS) break;
    }
    fclose(arquivo);
}

void salvar_estoque(Produto lista[], int total) {
    FILE *arquivo = fopen(NOME_ARQUIVO, "w");
    if (arquivo == NULL) {
        printf("\nErro ao salvar os dados!\n");
        return;
    }

    for (int i = 0; i < total; i++) {
        fprintf(arquivo, "%d,%s,%d,%.2f\n", 
                lista[i].codigo, lista[i].nome, lista[i].quantidade, lista[i].preco);
    }
    fclose(arquivo);
}

void exibir_menu() {
    printf("\n====================================\n");
    printf("    GERENCIADOR DE ESTOQUE (C)\n");
    printf("====================================\n");
    printf("1. Cadastrar Produto\n");
    printf("2. Listar Produtos\n");
    printf("3. Buscar Produto por Codigo\n");
    printf("4. Sair\n");
    printf("Opcao: ");
}

void cadastrar_produto(Produto lista[], int *total) {
    if (*total >= MAX_PRODUTOS) {
        printf("\nErro: Estoque cheio!\n");
        return;
    }

    Produto p;
    printf("\n--- Cadastrar Produto ---\n");
    
    printf("Codigo: ");
    while (scanf("%d", &p.codigo) != 1) {
        printf("Codigo invalido. Digite um numero: ");
        limpar_buffer();
    }
    limpar_buffer();

    printf("Nome: ");
    scanf("%49[^\n]", p.nome);
    limpar_buffer();

    printf("Quantidade: ");
    while (scanf("%d", &p.quantidade) != 1) {
        printf("Quantidade invalida. Digite um numero: ");
        limpar_buffer();
    }
    limpar_buffer();

    printf("Preco: ");
    while (scanf("%f", &p.preco) != 1) {
        printf("Preco invalido. Digite um valor numérico: ");
        limpar_buffer();
    }
    limpar_buffer();

    lista[*total] = p;
    (*total)++;

    salvar_estoque(lista, *total);
    printf("\nProduto cadastrado e salvo com sucesso!\n");
}

void listar_produtos(Produto lista[], int total) {
    if (total == 0) {
        printf("\nNenhum produto cadastrado no estoque.\n");
        return;
    }

    printf("\n--- Lista de Produtos ---\n");
    for (int i = 0; i < total; i++) {
        printf("Codigo: %d | Nome: %s | Qtd: %d | Preco: R$ %.2f\n",
               lista[i].codigo, lista[i].nome, lista[i].quantidade, lista[i].preco);
    }
}

void buscar_produto(Produto lista[], int total) {
    if (total == 0) {
        printf("\nNenhum produto cadastrado no estoque para buscar.\n");
        return;
    }

    int codigo_busca;
    printf("\n--- Buscar Produto ---\n");
    printf("Digite o codigo do produto: ");
    while (scanf("%d", &codigo_busca) != 1) {
        printf("Codigo invalido. Digite um numero: ");
        limpar_buffer();
    }
    limpar_buffer();

    for (int i = 0; i < total; i++) {
        if (lista[i].codigo == codigo_busca) {
            printf("\nProduto Encontrado:\n");
            printf("Codigo: %d | Nome: %s | Qtd: %d | Preco: R$ %.2f\n",
                   lista[i].codigo, lista[i].nome, lista[i].quantidade, lista[i].preco);
            return;
        }
    }

    printf("\nProduto com o codigo %d nao foi encontrado.\n", codigo_busca);
}

int main() {
    Produto estoque[MAX_PRODUTOS];
    int total_produtos = 0;
    int opcao;

    carregar_estoque(estoque, &total_produtos);

    do {
        exibir_menu();
        if (scanf("%d", &opcao) != 1) {
            printf("\nOpcao invalida! Digite apenas numeros.\n");
            limpar_buffer();
            continue;
        }
        limpar_buffer();

        switch (opcao) {
            case 1:
                cadastrar_produto(estoque, &total_produtos);
                break;
            case 2:
                listar_produtos(estoque, total_produtos);
                break;
            case 3:
                buscar_produto(estoque, total_produtos);
                break;
            case 4:
                printf("\nSaindo e salvando dados...\n");
                break;
            default:
                printf("\nOpcao invalida! Tente novamente.\n");
        }
    } while (opcao != 4);

    return 0;
}