#include <stdio.h>
#include <stdlib.h>

#define MAX_PRODUTOS 100

typedef struct {
    int codigo;
    char nome[50];
    int quantidade;
    float preco;
} Produto;

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
    scanf("%d", &p.codigo);
    
    printf("Nome: ");
    scanf(" %[^\n]", p.nome); // O espaço antes de % e [^\n] permite ler nomes com espaço
    
    printf("Quantidade: ");
    scanf("%d", &p.quantidade);
    
    printf("Preco: ");
    scanf("%f", &p.preco);

    lista[*total] = p;
    (*total)++;

    printf("\nProduto cadastrado com sucesso!\n");
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

int main() {
    Produto estoque[MAX_PRODUTOS];
    int total_produtos = 0;
    int opcao;

    do {
        exibir_menu();
        scanf("%d", &opcao);

        switch (opcao) {
            case 1:
                cadastrar_produto(estoque, &total_produtos);
                break;
            case 2:
                listar_produtos(estoque, total_produtos);
                break;
            case 3:
                printf("\n[Em breve] Busca por codigo...\n");
                break;
            case 4:
                printf("\nSaindo do sistema...\n");
                break;
            default:
                printf("\nOpcao invalida! Tente novamente.\n");
        }
    } while (opcao != 4);

    return 0;
}