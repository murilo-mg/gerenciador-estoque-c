#ifndef ARMAZENAMENTO_H
#define ARMAZENAMENTO_H

#include "produto.h"
#include <stdio.h>

typedef enum
{
    ARMAZENAMENTO_OK,
    ARMAZENAMENTO_AUSENTE,
    ARMAZENAMENTO_ERRO,
    ARMAZENAMENTO_SEM_MEMORIA
} ResultadoArmazenamento;

/* Inicialize o estoque antes de carregar. Falhas fatais preservam seu conteúdo.
 * Linhas inválidas são ignoradas e contabilizadas, com aviso no fluxo informado. */
ResultadoArmazenamento carregar_estoque(const char *caminho, Estoque *estoque,
                                      size_t *linhas_invalidas, FILE *avisos);
/* Não substitui um temporário existente; uma falha preserva o CSV anterior. */
ResultadoArmazenamento salvar_estoque(const char *caminho, const Estoque *estoque);
/* OK indica temporário presente; AUSENTE indica que o arquivo não existe. */
ResultadoArmazenamento verificar_temporario(const char *caminho);

#endif
