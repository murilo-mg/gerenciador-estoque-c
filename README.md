# Gerenciador de estoque em C

Gerenciador de estoque de terminal em C11, sem bibliotecas externas. Os produtos ficam em um vetor dinâmico e são persistidos em `estoque.csv`, no diretório de onde o programa é executado. Código, comentários e mensagens usam português; os textos são UTF-8.

## Funcionalidades

- Cadastro com código único e validação de nome, quantidade e preço.
- Listagem e busca por código ou parte do nome.
- Edição de código, nome, quantidade e preço, mantendo a unicidade do código.
- Remoção de produtos.
- Entrada e saída de quantidade, bloqueando saldo negativo e estouro numérico.
- Alerta de estoque baixo (até 5 unidades), na abertura, nas listagens e após alterações.
- Preços exatos em centavos, sem aritmética de ponto flutuante.
- Gravação por arquivo temporário, com verificação de escrita, fechamento e renomeação.

## Compilar e executar

Requisitos: compilador C11 (GCC por padrão), Make e terminal com suporte a UTF-8. A configuração de integração contínua usa Linux.

```sh
make
make run
```

Ou, diretamente:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic main.c produto.c armazenamento.c -o sistema_estoque
./sistema_estoque
```

`make all` gera `sistema_estoque`. `make clean` remove os binários, objetos e arquivos de teste em `build/`; preserva o CSV usado pelo programa.

## Exemplo de uso

Selecione `1` para cadastrar e informe:

```text
Código: 101
Nome: Café, "especial"
Quantidade: 10
Preço (R$): 12,90
Alteração salva com sucesso.
```

O preço aceita ponto ou vírgula decimal, com zero, uma ou duas casas. Valores negativos, notação científica, `nan`, `inf`, casas excedentes e lixo após o número são rejeitados.

Use `7` para registrar uma entrada e `8` para uma saída. Uma saída de 11 unidades desse produto será recusada por saldo insuficiente. Use `5` para editar, preenchendo novamente os quatro campos, e `6` para remover por código. A busca por nome (`9`) encontra trechos e diferencia maiúsculas, minúsculas e acentos. A opção `10` lista apenas produtos com estoque baixo.

A opção `4` encerra. EOF (Ctrl+D no Linux) também encerra, inclusive no meio de um formulário; um formulário incompleto não é cadastrado. Cada alteração é salva imediatamente. Se a gravação falhar, a alteração é cancelada também em memória.

## Formato do CSV

UTF-8, sem cabeçalho ou BOM, um produto por linha, com quatro campos nesta ordem:

```text
codigo,nome,quantidade,preco_em_reais
101,"Café, ""especial""",10,12.90
102,"Arroz",3,7.50
```

O preço no arquivo usa ponto decimal e duas casas; internamente, `12.90` corresponde a 1290 centavos. Nomes são gravados entre aspas duplas; aspas no nome são duplicadas. A leitura também aceita nomes sem aspas do formato antigo, linhas LF ou CRLF e a última linha sem quebra final. Campos excedentes e aspas malformadas são rejeitados.

Código deve ser positivo. Quantidade e preço podem ser zero. Códigos repetidos, valores negativos, nomes vazios, UTF-8 inválido, controles, bytes nulos e linhas grandes demais são considerados inválidos. A leitura avisa o número de cada linha inválida e continua até o fim, sem truncar a quantidade de produtos.

Se houver linhas inválidas, o menu permite consultar os produtos válidos e bloqueia alterações para preservar o arquivo original. Faça uma cópia de segurança, corrija as linhas indicadas e reinicie. Um erro de leitura ou de alocação interrompe a abertura sem sobrescrever o CSV. A ausência do arquivo inicia um estoque vazio.

A gravação cria `estoque.csv.tmp` no mesmo diretório, usando criação exclusiva de C11. Só após escrita, `fflush` e `fclose` bem-sucedidos o temporário é renomeado para `estoque.csv`. Uma falha preserva o CSV anterior. Um temporário existente nunca é truncado. Se sobrar após uma interrupção, confira seu conteúdo e remova-o ou mova-o manualmente antes de tentar salvar novamente, com o programa encerrado.

## Testes

```sh
make test
make debug
```

Os testes usam C11 e `assert`, sem frameworks ou bibliotecas externas. `make test` executa três conjuntos: regras e CSV; interação do menu com entradas simuladas; falhas de `malloc` e `realloc`. Os arquivos de teste ficam em `build/` e são removidos ao concluir.

A cobertura inclui cadastro, duplicidade, edição, remoção, saldo insuficiente, estouro de quantidade, estoque baixo, buscas, vetor com mais de 100 produtos, limites de preço, ida e volta do CSV, vírgulas e aspas no nome, formato antigo, arquivo ausente, linhas malformadas, UTF-8 inválido, linhas longas, bytes nulos, EOF durante formulários e cancelamento de alterações quando a gravação falha.

`make debug` gera `build/debug/sistema_estoque` e executa os mesmos testes com AddressSanitizer, UndefinedBehaviorSanitizer e LeakSanitizer. Para usar o menu instrumentado:

```sh
./build/debug/sistema_estoque
```

Execute LeakSanitizer fora de depuradores que usam `ptrace`. O GitHub Actions compila com `-Wall -Wextra -Wpedantic -Werror` e executa os testes normais e com sanitizers a cada push e pull request.

## Organização

| Arquivo | Responsabilidade |
| --- | --- |
| `produto.h`, `produto.c` | Produtos, vetor dinâmico, validações e regras de estoque |
| `armazenamento.h`, `armazenamento.c` | Leitura, diagnóstico e gravação do CSV |
| `main.c` | Menu e interação com o usuário |
| `testes.c` | Regras, conversões e persistência |
| `testes_interacao.c` | Fluxos do menu, entradas inválidas e EOF |
| `testes_memoria.c` | Falhas de alocação sem perda do estado anterior |
| `Makefile` | Compilação, execução, testes, sanitizers e limpeza |

## Limitações conhecidas

- Nomes têm no máximo 127 bytes UTF-8, incluindo espaços; não aceitam controles ou quebras de linha. O CSV admite linhas físicas de até 1023 bytes, suficientes para qualquer produto gerado pelo programa.
- Código e quantidade usam `int`, limitados por `INT_MAX` (normalmente 2147483647). O preço máximo é R$ 92233720368547758.07 (`INT64_MAX` centavos). A quantidade de produtos é limitada pela memória disponível; buscas são lineares e uma alteração usa uma cópia temporária do estoque.
- Use uma única instância por arquivo. Não há bloqueio do CSV contra execuções simultâneas ou alterações externas.
- A substituição por `rename` é atômica em sistemas POSIX, como Linux. A biblioteca C11 não oferece sincronização física do disco (`fsync`): não há garantia de durabilidade em falta de energia. Em sistemas que não permitem substituir um arquivo existente por `rename`, a operação falha e preserva o arquivo anterior.
- Um CSV corrompido exige correção manual antes de novas alterações; o programa não tenta adivinhar os dados perdidos.
