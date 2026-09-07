#include <stdio.h>
#include <stdlib.h>

// Estrutura para representar um produto no estoque
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
    printf("3. Buscar Produto por Código\n");
    printf("4. Sair\n");
    printf("Opção: ");
}

int main() {
    int opcao;

    do {
        exibir_menu();
        scanf("%d", &opcao);

        switch (opcao) {
            case 1:
                printf("\n[Em breve] Cadastro de produto...\n");
                break;
            case 2:
                printf("\n[Em breve] Listagem de produtos...\n");
                break;
            case 3:
                printf("\n[Em breve] Busca por código...\n");
                break;
            case 4:
                printf("\nSaindo do sistema...\n");
                break;
            default:
                printf("\nOpção inválida! Tente novamente.\n");
        }
    } while (opcao != 4);

    return 0;
}