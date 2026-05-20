## Introdução PintOS

###### Mudanças no ./src

- Para facilitar, em geral os comandos de make já estão com os path's dos programas certos, mas se precisar em geral tem que usar o comando `export PATH=$PATH:<Pintos>/src/utils`; Assim em geral para rodar só precisa do comando de `make check`

- Para funcionar no Arch Linux modifiquei o src/Makefile.build:93 para ele reduzir o tamanho do loader.bin em todas os testes;

- Adicionado logica para ir executando os testes em especifico, no caso do threads, basicamente usa `make test TEST=<nome_do_test>`;

- Mudei também o código em Perl para ele mostrar o resultado dos testes de forma colorida para melhorar o debug;

- Adicionei uma função de debug mais simples no `lib/debug.h`, basicamente ela é um define e a sua forma de uso está melhor explicada lá, recomendo usar para não ficar colocando e tirando `printf` várias vezes;

- Adicionei a linha de código `_Static_assert(sizeof(struct inode_disk) == 512, "inode_disk must be 512 bytes");` no `filesys/inode.c` para caso a struct `inode_disk` tenha tamanho diferente de 512 bytes ele não compile e mostre esse erro na compilação, facilitando não ter que rodar para verificar isso;

##### Recursos necessários

###### 1. Make

- O make serve para ajudar nossa vida. Não há uma obrigatoriedade de usá-lo, mas iremos;
- Caso necessite instalá-lo, utilize o comando `sudo apt install make`;

###### 2. GCC e GDB

- Compilador e Depurador para C;
- Caso necessite instalá-los, utilize os comandos `sudo apt install gcc` e `sudo apt install gdb`;

###### 3. Qemu

- Recurso necessário para executar o sistema nos casos testes;
- Caso necessite instalá-lo, utilize o comando `sudo apt install qemu-system-i386`;

###### 4. Boch

- Alternativa mais rápida ao Qemu, entretanto não utilizaremos ele;
- Se desejar saber como fazê-lo funcionar, acesse `https://web.stanford.edu/class/cs140/projects/pintos/pintos_12.html#SEC167`;

#### WSL

- Caso opte por não fazer em sua máquina com Linux ou não possua permissão para baixar algum recurso no computador, siga o tutorial oficial oficial `https://learn.microsoft.com/pt-br/windows/wsl/install`;

##### Tutorial de comandos inicias

- Os comandos em geral já estão com os respectivos paths de forma automática, assim só entender o principal comando de `make` para compilar o kernel e rodar os testes.
- Cada parte do projeto vai trabalhar principalmente em uma pasta específica, dentro de `src/`:
  1. A primeira parte vai trabalhar e rodar os testes na pasta `threads/`(Cadeira de Sistemas Operacionais-CIN015 irá fazer apenas uma parte dessa parte, não precisa avançar mais).
  2. A segunda parte vai trabalhar e rodar os testes na pasta `userprog/`(Trabalhar com progamas de usuários).
  3. A terceira parte vai trabalhar e rodar os testes na pasta `vm/`(Trabalhar com memória virtual).
  4. A quarta parte vai trabalhar e rodar os testes na pasta `filesys/`(Trabalhar com o sistema de arquivos).
- Dentro da pasta certa execute o comando de `make`, ele vai criar o diretório `build/`, em vamos ter o kernel compilado do sistema.
- Para executar os testes usa o comando `make check` dentro desse diretório(dentro do `build/` ou na pasta diretamente acima), por padrão executando no terminal e mostrando os testes que passaram.
- Caso deseje executar novamente, pode dar `make clean`, dentro da pasta `build/`, antes de usar o próximo `make check`;
- Podemos rodar um teste em específico usando basicamente `make <nome do teste>.result`, mas o nome do teste vai ser algo como por exemplo`tests/vm/page-linear`.
- Existe o comando `make check VERBOSE=1` fará com que tudo seja executado de maneira mais limpa em que cada teste aparecerá no terminal apenas durante sua execução;
- Caso você queira, pode ir na pasta `src/tests/`, os subdiretórios basicamente contém os códigos dos respectivos testes;
- Todos os testes executados geram alguns arquivos relatando a saída e se passou ou não, que ficam na pasta `build/tests` do respectivo diretório de trabalho do projeto;

##### Observações

- É possível executar os comandos do Boch normalmente, recomendado usar o QEMU, porém se você não tiver ajustado ele, os resultados dos testes ficarão quase todas de falhas (testado no WSL2), visto que o Boch meio que acelera o tempo do teste oque pode quebrar alguns deles;
- Há a opção de seguir o tutorial no arquivo `Exec_pintos.pdf` caso queira fazer as modificações por si mesmo;

### Objetivos

#### Parte 1

- [ ] Alarm Clock;
- [ ] Priority;
- [ ] Advanced Scheduler - Multi-Level Feedback Queue (mlfqs);

##### OBS
O projeto da cadeira de Sistemas Operacionais-CIN015 deve fazer apenas o *Alarm Clock* e o *Advanced Scheduler*.

#### Parte 2

- [ ] Alrgument Passing;
- [ ] User Memory Acess;
- [ ] System Calls;
- [ ] Process Termination;
- [ ] Denying Writes to Executables;

#### Parte 3

- [ ] Paging;
- [ ] Stack Growth;
- [ ] Memory Mapped Files;
- [ ] Acessing User Memory;

#### Parte 4

- [ ] Indexed and Extensible Files;
- [ ] Subdirectories;
- [ ] Buffer Cache;
- [ ] Synchronization;

---

### Detalhamentos

#### Parte 1

##### Objetivos Principais

   Modificar o PintOS para que a lógica de sleep/wake funcione no alarme de forma devida e implementar devidamente a mlfqs. Nesse processo, os arquivos a serem modificados devem ser apenas os `src/device/timer.` e o `src/threads/thread.`;

##### Alarm

   Reimplementar `timer_sleep()` no `device/time.c` que ta originalmente implementado com 'busy wait', que fica chamando `thread_yield()` enquanto o tempo não tiver passado;

- Ideia:
   `Implementar um estado bloqueado para auxiliar e permitir corrigir o alarme`

##### Scheduler

   Na documentação oficial, ele sugere para dar opção de ter o mlfqs ou o por prioridade, ou seja, implementar ambos, porém nosso projeto não almeja a implementação do por prioridade; com o mlfqs as prioridades definidas pelas threads devem ser ignoradas e controladas pelo escalonador;
   [Fila esquema](https://www.google.com/url?sa=i&url=https://medium.com/@francescofranco_39234/multilevel-feedback-queue-3ae862436a95&psig=AOvVaw0uPvTNvKvDx0bKwYGvKyn_&ust=1718223750727000&source=images&cd=vfe&opi=89978449&ved=0CBIQjRxqFwoTCLD727Sw1IYDFQAAAAAdAAAAABAI)

   Segundo o apêndice que fala do scheduler devemos implementar o conceito de `avg_load`, `thread_nice` e o `cpu_recent_time`;
   O `avg_load` é a carga média do sistema levando em conta a quantidade de threads em ready_list, sem incluir thread ociosa:

$$
avg = (\frac{59}{60}) * avg + (\frac{1}{60}) * (tamanho-da-ready-list)
$$

   O `cpu_recent_time` é uma média móvel exponencial, específica de cada thread e que começa em 0, que serve como peso na hora de calcular a prioridade, que consiste em considerar uma função exponencial em que com o passar do temp os cpu-time antigos tenham pesos menores e os mais recentes os pesos maiores; todas as threads devem ter seu recent time recalculados 1 vez por segundo (timer_ticks() % TIMER_FREQ == 0) usando:

$$
CpuTime = ( \frac{2 * avg}{2 * avg + 1} * CpuTime + nice) * 100
$$
   O `nice` é específico de cada thread, há funções a se implementar e fazê-lo funcionar corretamente; ele deve estar entre -20 e 20 e vai servir para calcular a prioridade em que quanto mais positivo, menor a prioridade, que será calculada usando o `recent_time` (apenas se ele mudar) para alterar a thread de fila na mlfq, usando a fórmula:

$$
p = floor(PriMax - (\frac{RecentCpuTime}{4}) - (nice * 2))
$$

###### Pontos Flutuantes

   O kernel não suporta float nem double, então a documentação recomenda usar o formato de 17.14, 17 bits para a parte inteira e 14 para a fracionária; Para transformar reais nesses tipos basta multiplicar por 2^Q, onde Q é o numero de bits separado para a parte fracionária, e truncar para int, a documentação recomenda usar isso no recent cpu time e no avg, basicamente simulando operações em float usando inteiros (ver [aqui](https://www.scs.stanford.edu/23wi-cs212/pintos/pintos_7.html) como as operações podem ser feitas);

###### Detalhes
- Os testes do alarm, quando executado, podem sinalizar que estão funcionando, porém, estão em espera ocupado, portanto não estão devidamente implementado;
- Há esses vídeos de guia sobre o assunto, caso necessite de ajuda: `https://www.youtube.com/watch?v=myO2bs5LMak` e `https://www.youtube.com/watch?v=57r9OCN1EfA` (são aulas sobre a implementação do projeto completo do PintOS);

#### Parte 2

##### Objetivos Principais

Implementação da lógica referente a execução de programas no sistema operacional, passando por passagem de argumentos, acesso de memória, system calls, término de processos e permissões de execução; Vai modificar basicamente os arquivos nos diretórios `userprog/`  e `filesys/`;  O código da primeira fase não é estritamente necessário para essa funcionar;

###### OBS

Para os testes inicialmente funcionarem, o sistema em si deve ter a capacidade de printar e ter o exit do processo, ter um wait minimamente implementado para ele não "passar direto" e por fim implementar o argument parssing, a ordem das implementações deve ser basicamente essa, os primeiros testes devem funcionar logo de cara se tudo tiver certo, sem os argumentos sendo passados de forma certa o resto basicamente não pega.
Outra observação é de como rodar os testes nessa fase, por base podemos usar o comando `T=nome; ../../utils/pintos --qemu --filesys-size=2 -p tests/userprog/$T -a $T -- -q -f run $T`
onde nome deve ser o nome do teste(no caso do binário do teste, pois diferente da primeira fase que os testes eram funções, nessa eles são arquivos .c que são compilados e depois passados para um sistema de arquivos 'fake' temporário, a criação dele está descrita na documentação em inglês), além disso tem alguns testes que dependem de outros arquivos, nesse caso só usar novamente a flag `-p` e `-a` para move o arquivo para o sistema de arquivos temporário, exemplo de `T=nome; ../../utils/pintos --qemu --filesys-size=2 -p tests/userprog/%T -a $T -p <path para o arquivio> -a <nome dele dentro do filesys>`.

##### Process Termination and Wait

Basicamente é o início das implementações de syscall's, necessário para rodar os testes, em  que a syscall de exit deve printar o aviso de exit `printf ("%s: exit(%d)\n", ...);` `%s` é ó nome do processo e o `%d` é o código; A parte do Wait basicamente vai indicar para o processo esperar outro terminar, na documentação tem um exemplo básico só para rodar o teste.

```C
int
process_wait(tid_t child_tid)
{
 while(true)
 {
  thread_yield();
 }
}
```

Esse código basicamente vai permitir que as threads filhas criadas terminem sua execução e gerem os resultados dos testes, mas a implementação no final dessa fase deve ser bem diferente para conseguir passar em outros testes.

##### Argument Passing

Empilhamento dos argumentos de executável, empilhar os argumentos do `argc` e `argv[]` na ordem certa; A ordem de empilhar, lembrando que é uma pilha inversa aumentar a pilha é subtrair o endereço, basicamente empilha, usando memcpy, primeiro o nome do executável, depois os argumentos na ordem que foram passados(a documentação sugere um limite de 128 argumentos), além disso guardando em um vetor os endereços deles nessa pilha, empilha o alinhamento de memória de 4 bytes, empilha os endereços, dos argumentos na pilha, na ordem inversa e por último o endereço do último endereço, empilha o argc e por fim um endereço de retorno fake, podendo ser NULL; Na documentação do CSI  e na oficial tem exemplos da ordem para conferir, além do exemplo com o uso do hexdump.

| Address      | Name           | Data          | Type        | Descrição                                |
| ------------ | -------------- | ------------- | ----------- | ---------------------------------------- |
| `0xbffffffc` | `argv[3][...]` | `"bar\0"`     | `char[4]`   | 4º argumento (string)                    |
| `0xbffffff8` | `argv[2][...]` | `"foo\0"`     | `char[4]`   | 3º argumento (string)                    |
| `0xbffffff5` | `argv[1][...]` | `"-l\0"`      | `char[3]`   | 2º argumento (string)                    |
| `0xbfffffed` | `argv[0][...]` | `"/bin/ls\0"` | `char[8]`   | Nome do executável                       |
| `0xbfffffec` | `word-align`   | `0`           | `uint8_t`   | Alinhamento (3 bytes de padding)         |
| `0xbfffffe8` | `argv[4]`      | `0`           | `char*`     | Sentinel (NULL terminator, 4 bytes de 0) |
| `0xbfffffe4` | `argv[3]`      | `0xbffffffc`  | `char*`     | Ponteiro para `"bar"`                    |
| `0xbfffffe0` | `argv[2]`      | `0xbffffff8`  | `char*`     | Ponteiro para `"foo"`                    |
| `0xbfffffdc` | `argv[1]`      | `0xbffffff5`  | `char*`     | Ponteiro para `"-l"`                     |
| `0xbfffffd8` | `argv[0]`      | `0xbfffffed`  | `char*`     | Ponteiro para `"/bin/ls"`                |
| `0xbfffffd4` | `argv`         | `0xbfffffd8`  | `char**`    | Array de ponteiros `argv[]`              |
| `0xbfffffd0` | `argc`         | `4`           | `int`       | Contagem de argumentos                   |
| `0xbfffffcc` | `return addr`  | `0`           | `void(*)()` | Endereço de retorno (fake/NULL)          |

##### System Calls

Essa parte é a mais extensa, basicamente tendo que implementar quase todas as syscall's do sistema e só vai funcionar qualquer coisa depois de implementar o write e o wait como dito acima; As descrições são bem escassas nos documentos, as principais recebem um pouco mais de atenção, mas por causa disso é a parte mais difícil dessa fase do projeto;
O padrão que geralmente é usado, para se manter organizado, é criar um handler para cada função e depois criar a função por si só, pois o handler vai extrair os argumentos do `intr_frame`, mais especificamente do `esp` dele, onde vai estar salvo o ponteiro da stack do processo;
Para a extração dos argumentos a documentação sugere uma manipulação que pode ficar confusa, então sugiro usar isso `int *args = (int *)f->esp;` e acessar os argumentos simplesmente com `args[1]`, `args[2]` e `args[3]`, sendo que não pode passar de 3, pois o máximo de argumentos que serão passados é 3 e o `args[0]` é o próprio `f->esp`; Lembrando que será cobrado muito a validade do dados e dos endereços desses argumentos e tem funções para verificar eles, verificando se os endereços que eles passam estão em áreas válidas.

###### OBS

- Alguns testes testam a respeito do comportamento das threads filhas que o programa principal cria então uma das estruturas que deve ser criada é algo para guardar algumas informações sobre os filhos de uma thread, como exitstatus, load_sucess, tid....
- Outra estrutura que deve ser criada é uma representação de um file descriptor, que basicamente deve conter seu fd(outra observação é que os fd de arquivos são específicos por thread/processo), um ponteiro para o arquivo e uma forma da thread acessar os arquivos que ela abriu.
- Quando o código de uma syscall não puder se identificado ou for um endereço inválido(tem teste que chama direto syscall com algumas partes dele inválida, por isso bom verificar) deve ser usado o exit com status de -1; Lembrando que a verificação de testes não deve acessar ou derreferenciar o ponteiro, pois isso pode causar page_fault's que devem ser tratados de forma melhor no próximo tópico.

###### Exit e Write

- A implementação básica do write envolve basicamente colocar os dados no buffer do terminal, basicamente checar o o `fd`, se ele for 0, deve retornar `-1`, pois o `fd == 0` representa o stdin que não deve ser possível escrever nele; A segunda verificação é o `fd == 1` que é o stdout, para mandar escrever na saída padrão devemos usar a função `putbuf` que esta implementada no arquivo `lib/kernel/console.c`, aí só passar para ela o buffer e o size e depois retorna o size, lembrando de verificar a validade do buffer de onde vai vir as informações.
- A implementação do exit envolve primeiro printar o código da saída, o status, junto com o nome da thread, o status ele está na pilha de argumentos e o nome pode ser conseguido pela função `thread_name()`, com isso deve-se printar no seguinte formato `<nome_thread>: exit(<status>)\n`.

###### Write out of bounds

- Em alguns testes de write, isso foi notado especificamente no teste de `write-normal`, que esse teste tenta aumentar o arquivo escrevendo nele e por enquanto sem a fase 4 do projeto isso não é possível, para isso funcionar foi necessário alterar o código do arquivo `filesys/inode.c`, na lógica do `inode_write_at`, basicamente verificando se o endereço final que o user deseja escrever estiver fora do tamanho do inode, devemos aumentar ele, trocar a informação de `data.length` dele e usar a função `block_write` para persistir a mudança.

##### User Memory Acess

O user memory acess basicamente é a parte da implementação que diz respeito a verificar as faltas da memória, mas especificamente os testes que vão validar essa etapa verificam se o kernel consegue lidar com endereços `NULL` ou outro apontando para fora da memória válida sem simplesmente morrer; Os testes que fazem isso são os de prefixo `bad-*`, eles vão tentar fazer com que o sistema leia ou escreva em endereços inválidos e com isso ele cai em um page_fault, basicamente oque deve ser feito é mexer no arquivo `userprog/exception.c` e achar uma forma de não matar o kernel em caso de page_fault, isso será feito na função `page_fault` usando as flags da função para determinar a origem do page_fault.
Basicamente ele vai usar matar o kernel se for algum ponteiro dele mesmo(flag `user` verifica isso), mas vai apenas sair com o status de -1 se o ponteiro for do usuário e o endereço for inválido ou se for uma tentativa de escrita em uma página de somente leitura por exemplo.

##### Denying writes to executables

Como o próprio nome diz não permitir escrever em algum programa que está executando, para isso na hora de carregar os arquivos nas funções do `process.c`, logo após iniciar o processo, deve-se usar a função `file_deny_write`  e logo depois de terminar o processo, ou em caso de erro, deve-se usar a função `file_allow_write` para gerenciar qual arquivo está ou não permitido a escrita durante o momento da execução do processo/thread; Para conseguir identificar qual o executável de uma thread recomenda-se criar um campo para armazenar informações sobre ele na estrutura de um thread

#### Parte 3

##### Objetivos Principais

Toda essa parte do projeto basicamente vai lidar com diferentes tipos de paginação para diferentes objetivos, em resumo vai ser necessário implementar 4 estruturas de dados, cada uma com uma lógica e servindo para completar um dos objetivos dessa parte; Vai precisar modificar as pastas `vm/`,  `devices/` e `userprog/`

###### OBS

- Primeira coisa é que tem que modificar o Makefile.build, para incluir a pasta e os arquivos específicos, lembrar de que a cada arquivo adicionado tem que modificar lá, se não ele não vai ser compilado e incluído no sistema.
- Uma observação é usar em alguns casos a função pg_round_down, principalmente na parte de page_fault, ele vai "arredondar" o endereço para o início da página em si.

##### Paging

A primeira ideia é dar um jeito de linkar as paginas de kernel às de user, nos pontos principais, criar uma estrutura(geralmente se usa o hashmap), para relacionar um upage(user page) ao kpage(kernel page), basicamente endereços, algo como uma tabela de frames que vai relacionar eles, ai para isso deve-se criar funções para alocar, pegar... endereços sugerindo seguir o molde de nomes e as funções que ja tem para alocar endereços de kernel(no caso as funções de nome palloc), além de que se quiser iniciar alguma coisa logo no começo do sistema deve colocar isso em uma função e chamar ela no arquivo `src/threads/init.c` na função `main(void)`, ela é a primeira função a ser chamada dentro do sistema, nela tem outras funções relacionadas ao setup do sistema, alocação de estruturas e coisas parecidas.

###### Lazy

Depois disso a parte de lazy loading basicamente é modificar a função load_segment, no arquivo `src/userprog/process.c`, por exemplo, para ele não carregar de cara as páginas, mas sim salvar elas de alguma forma, aí cabe a você decidir como fazer isso, para depois quando tiver um page_fault(que a função está no `exceptions.c`) puder carregar a certa, além de gerenciar as páginas com base na lógica que foi feita na parte anterior, mantendo informações se ele é de um arquivo, uma página em branco(apenas alocada para uso posterior)... tudo isso em geral é recomendado usar a hashtable implementada em `lib/kernel/hash.h|c`(para usar essa estrutura tem que criação uma função de hash e uma função de less, para definir uma ordem entre dois elementos, que ambas tem a assinatura definida no `.h`).

##### Stack Growth

Cada thread deve ter um controle sobre a própria stack, basicamente um ponteiro para ela na struct thread, além disso ela deve lidar com o crescimento dela no sentido de alocar se tiver um page_fault, mas lembrando que a base da stack é a variável PHYS_BASE, mas como é uma stack invertida, ela é um endereço alto e cresce para endereços menores, tem que ficar ligado na lógica para fazer a aritmética certa(além disso um valor interessante de máximo de tamanho na stack é de $1~mb$).

##### Memory Mapped Files

A questão da mmf é praticamente igual à lógica de gerenciar os arquivos abertos por uma thread, gerenciar elas por meio de um id e ir incrementando a medida que for colocando mais arquivos em memória, e a munmap é o inverso, vai desalocar esse arquivo, mas lembrando que temos que escrever oque foi mudado no disco, tudo com base na lógica feita antes de gerenciamento dos frames da memória(lembrar de desaloca os arquivos na função de process_exit, nesse caso só usar a syscall ja implementada).

##### Acessing User Memory

A última estrutura vai ser uma tabela de swap, que vai supervisionar os slots disponíveis e usados na memória gerenciando como vai ocorrer a cópia dos dados.
Swap é basicamente uma unidade de bloco a parte, para criar ela usa a função `block_get_role`, passando o argumento de `BLOCK_SWAP`, isso é como se fosse criar uma partição própria para isso dentro do armazenamento, retornando um `struct block *`. A partir daí é totalmente livre para gerenciar essa partição, só definindo onde trocar as páginas e lembrando de iniciar a pagina e swap apenas depois do sistema de arquivos, geralmente no final da função `main`  em `init.c`.

#### Parte 4

##### Objetivos Principais

Completar o sistema de arquivos atual; Essa fase do projeto pode ser criada em cima da fase 2(user programs) ou da fase 3(virtual memory); Em questão de código é talvez a mais extensa, além disso debugar ela bastante chato visto que em todo local basicamente vai estar trabalhando com blocos de memória e tal, para anexo aqui alguns comandos que podem ajudar a debugar os blocos de memória do sistema de arquivos:

- Para criar o arquivo que é usado como um disco e formatar ele automaticamente usa o comando:

```bash
../../utils/pintos-mkdisk filesys.dsk --filesys-size=2
```

- Para mostrar um byte em específico dentro do arquivo todo para debug é:

```bash
xxd -s <indice do byte> -l 1 <dsk file>
```

- Podemos separar os blocos do sistema de arquivos(de 512 bytes) e mostra no terminal de forma melhor formatada usando o comando, o problema é que ele mostra todos os blocos:

```bash
hexdump -v -e '512/1 "%02x " "\n"' <dsk file> | nl | more
```

- Outro comando que é mais controlado e pode indicar o índice do bloco e a quantidade de blocos adiante é:

```bash
dd if=filesys.dsk bs=<bloco, 512> skip=<indice do bloco> count=<qtd a partir do indice> status=none | xxd
```

##### Buffer Cache

Implementar um buffer para utilizar de cache dos blocos de arquivos, sugerindo pela documentação de no máximo 64 setores; O algoritmo para gerenciar o cache deve ter desempenho parecido com o algoritmo de clock wise e ser write back/behind, além de que a inserção de blocos subsequentes deve ser feito de forma assíncrona(background). Esse ponto é bem livre e nem tem algum teste diretamente para ele.

##### Extensible Files

A maioria dos testes precisam desse ponto para começar, ja que o arquivo de teste que eles usam já passa do tamanho de arquivo que o sistema sustenta antes de implementar isso.
Criação de estruturas de indexação direta, indireta e/ou duplamente indireta para gerenciar a fragmentação dos blocos de arquivos, distribuindo os blocos nos espaços disponíveis; A partição de arquivos será no máximo de 8Mb nos testes, mas deve suportar arquivos maiores que esse limite, permitindo também o crescimento dos arquivos.
A lógica base que é indicada na doc oficial é criar uma mistura de lista de blocos diretamente, indiretamente e duplamente indireta, os valores que ele indica é 12 blocos de setores diretos, cada um tem 512 bytes, depois um bloco indireto que vai ter 128 ponteiros para blocos e depois outro bloco indireto que vai ter mais 128 ponteiros para outros blocos com 128 ponteiros, oque da de memória:

$$
\text{Diretos = } 12 * 512 \to 6144~bytes \to 6~kb
$$

![diretos.png](diretos.png)

 $$
\text{Indiretos = } 128 * 512 \to 65536~bytes \to 65~kb
$$

![indiretos.png](indiretos.png)

$$
\text{Duplamente Indiretos = } 128 * 128 * 512 \to 8388608~bytes \to 8~mb
$$

![duplamente_indiretos.png](duplamente_indiretos.png)

##### Subdirectories

Modificação das syscall's para habilitar os padrões de path usados no UNIX(no caso '/', '.', '..' ..,) criando junto disso a lógica de um namespace hierárquico, diferenciando os tipos de arquivos(diretório ou arquivo "plano") e a separação do sistema entre processos; Permitir também a criação de arquivos com nomes maiores; Além disso outra modificação deve ser no tamanho permitido dos paths que deve ser permitido no mínimo 14 caracteres, podendo ou não extender essa capacidade mas deve permitir que os paths tenham mais que 14; Outro requisito é a trava de diretório, onde um processo deve ter acesso a uma cópia do diretório "atual".
Dessa parte o mais difícil é a questão de como tratar o path, tem várias maneiras de como você pode tratar a lógica e os testes vão sempre pegar nos pontos mais difíceis, seja testando a resiliência do sistema criando múltiplos pastas e arquivos aninhados, seja tentando encher uma pasta de arquivos para ver o limite... sendo sempre bom ler o código dos testes, que a maioria não é tão literal no nome.

##### New Syscall

As extensões que operam sobre arquivos, no caso `open`, `close` e `remove` devem ser modificadas para conseguir operar com os diretórios e para aceitar as modificações referentes a "entender" o path; Além dessas deve-se criar novas syscall para lidar com outras operações envolvendo diretórios, no caso as seguintes syscall: `chdir`, `makedir`, `readdir`, `is dir` e `inumber`.

##### Synchronization

Lógica de leitura e escrita síncrona no sistema de arquivos, sendo que apenas 1 threads pode modificar o sistema por vez, operações em caches diferentes devem ser independentes e múltiplas leituras devem ser feitas em paralelo;

###### Obs

- Nessa fase, dependendo de como você vai tratar as informações que tem na struct de thread pode facilmente ocorrer um erro, aparentemente sem motivo, na função `is_thread`, isso pode ocorrer por 2 motivos, o ponteiro ser NULL, oque dado o primeiro projeto feito é meio difícil e/ou a área de memória da thread foi corrompida(ele verifica isso verificando o valor do `MAGIC` da thread), isso pode ocorrer até ocasionalmente mesmo com o projeto 100% finalizado, mas é mais difícil e deve ser analisado com calma.
- Outro erro que pode muito ocorrer é o output do teste ficar repetindo várias vezes, não consegui determinar exatamente o que acontece, mas provavelmente alguma corrupção de memória da thread esta acontecendo.
- Os testes de persistence são os mais difíceis visto que o sistema deve estar basicamente finalizado pois usa praticamente todas as operações implementadas
