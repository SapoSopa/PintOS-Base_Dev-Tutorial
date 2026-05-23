#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "devices/timer.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif

#define THREAD_MAGIC 0xcd6abf4b
#define A 55

/* cada thread tem 4 estados possíveis:
  * RUNNING, READY, BLOCKED, DYING 

  VARIÁVEIS GLOBAIS: 
  essas variáveis só ficam visíveis dentro de thread.c, 
  para que não possam ser manipuladas por outros arquivos, 
  para acessar essas variáveis, outros arquivos usam funções
  
  isso significa que as variáveis existem globalmente durante toda a execução, 
  mas apenas o código dentro de thread.c pode acessá-las pelo nome
  outros arquivos são obrigados a usar as funções fornecidas por thread.c, 
  o que protege a consistência das estruturas internas do escalonador.

  */
static struct list sleep_list; // lista de threads bloqueadas (dormindo)
static struct list ready_list; // lista de threads prontas
static struct list all_list; // lista de todas as threads 
static struct thread *idle_thread; // thread ociosa (só roda quando a ready_list está vazia) 
static struct thread *initial_thread; //thread principal (representa o código que já rodava antes do sistema de threads) -> guarda ela numa struct
static struct lock tid_lock; // garantir que os ids das threads é único 
static fixed_point load_avg;  //declarar load_avg

static bool insert_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);

struct kernel_thread_frame 
  {
    void *eip;                  /* Return address. */
    thread_func *function;      /* Function to call. */
    void *aux;                  /* Auxiliary data for function. */
  };

static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

#define TIME_SLICE 4           
static unsigned thread_ticks;  

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
bool thread_mlfqs;

static void kernel_thread (thread_func *, void *aux);
static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);

//funcao que compara a prioridade das threads:
bool thread_compare_priority (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  struct thread *thread_a = list_entry (a, struct thread, elem);
  struct thread *thread_b = list_entry (b, struct thread, elem);

  return thread_a->priority > thread_b->priority;
}

/*função que checa se é nececessário mudar a thread atual com base na prioridade*/
void thread_mlfqs_yield(void)
{
  enum intr_level old_level = intr_disable (); //disabilida interrupções para usar a lista
  if (!list_empty(&ready_list))
    {
      struct thread *highest = list_entry(list_front(&ready_list), struct thread, elem);
      if (thread_current()->priority < highest->priority)
        {
          if (intr_context ()) 
            {
              intr_set_level(old_level); 
              intr_yield_on_return ();
              return;
            }
          else 
            {
              intr_set_level(old_level); 
              thread_yield ();
              return;
          }
        }
    }
  intr_set_level(old_level); 
}

//funcao auxiliar (usada no LESS de list_insert_ordered quando chamada na função)
bool wake_up_order(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED){
  struct thread *thread_a = list_entry(a, struct thread, sleep_elem); 
  struct thread *thread_b = list_entry(b, struct thread, sleep_elem);
  
  // se o tempo for igual, desempata pela maior prioridade
  if (thread_a->wake_up_tick == thread_b->wake_up_tick){
    return thread_a->priority > thread_b->priority;
  }
  
  // caso contrário, quem tem o menor tempo vem na frente
  return thread_a->wake_up_tick < thread_b->wake_up_tick;
}


/* coloca a Thread pra dormir pelo tempo determinado */
void thread_sleep (int64_t wake_up_tick){
  enum intr_level old_level; //pega estado de interrupcao atual
  old_level = intr_disable (); //disabilito interrupção

  struct thread *t = thread_current ();   //pego a thread atual 
  t->wake_up_tick = wake_up_tick;  //adiciono o tempo que a thread precisa
  list_insert_ordered (&sleep_list, &t->sleep_elem, wake_up_order, NULL);  //adicono a thread na lista de thread bloqueadas

  thread_block(); //bloqueio a thread
  intr_set_level (old_level); //volto para a interrupção que estava anteriormente
}


void thread_wake_up (int64_t ticks){
  enum intr_level old_level =  intr_disable(); //desligar interrupcoes

  while (!list_empty(&sleep_list)) //se a lista de threads bloqueadas NÃO estiver vazia
  {
    struct thread *t = list_entry(list_begin(&sleep_list), struct thread, sleep_elem); // pega a thread
    if (t->wake_up_tick > ticks){// se ainda nao atingiu a qtd de ticks necessaria 
      break; // permanece dormindo
    }
    list_pop_front(&sleep_list);
    thread_unblock(t);
  }
    
  intr_set_level (old_level); //volto para a interrupção que estava anteriormente

}


/* thread_init() e thread_start() vão inicializar o sistema de threads 
    quando o sistema pintos inicia, tem-se:
    main -> thread_init() -> thread_start() -> scheduler comeca a funcionar
    novas threads podem ser criadas

    quando o kernel começa a executar, já existe uma pilha (stack) sendo usada na cpu, 
    mas não existe struct thread, ready_list e scheduler configurados
*/
void
thread_init (void) 
{ // primeira parte de inicialização do sistema:
    //cria o sistema de threads e transforma a main numa thread
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock); //inicializa estruturas globais (configurador de IDs e as listas (all e ready))
  
  list_init(&sleep_list); // cria e inicializa as listas das threads
  list_init (&ready_list); 
  list_init (&all_list);

  /* pega a thread que já tá rodando 
      transforma o código que já está rodando numa thread 
      isto é, a main vira oficialmente initial_thread, 
      mas as interrupções ainda estão desligadas*/
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT); 
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();

  load_avg = 0; 
}

/* habilita interrupções e cria a idle_thread */
void
thread_start (void) 
{ // segunda parte de inicialização do sistema 
  struct semaphore idle_started; // cria idle_thread mas ela está bloqueada 
  sema_init (&idle_started, 0);
  thread_create ("idle", PRI_MIN, idle, &idle_started);

  intr_enable (); // escalonamento preemptivo começa 
  // timer fica habilitado a gerar interrupções 
  sema_down (&idle_started);  //espera idle_thread inicializar 
}

/* a cada tick, essa função é chamada pelo timer interrupt 
  essa funcao possui um mecanismo de preempcao adiada
  quando faz intr_yield_on_return(), não chama schedule, 
  ela marca uma flag pra que, quando a interrupcao terminar, 
  thread_yield seja chamada e depois o escalonador 
  isso acontece porque NÃO queremos 
  realizar trocas de contexto enquanto 
  uma interrupcao está sendo tratada
  */
void
thread_tick (void) 
{
  struct thread *t = thread_current ();
  /* Update statistics. */
  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;
    if (thread_mlfqs){
  
      if (t != idle_thread) mlfqs_increment_recent_cpu();
        int64_t ticks = timer_ticks ();

      if (ticks%TIMER_FREQ == 0){
        mlfqs_recalc_load_avg();
        mlfqs_recalc_all_recent_cpu();
      }
      if (ticks%4==0){
        thread_foreach(mlfqs_recalc_priority, NULL);
        list_sort (&ready_list, thread_compare_priority, NULL);
        thread_mlfqs_yield(); /*fazer reordenação de threads de acordo com a prioridade atualizada*/
      }
  }
  /* impoe a preempcao*/
  if (++thread_ticks >= TIME_SLICE) // se atingir a qtd de ticks, chama intr_yield_on_return
    intr_yield_on_return (); //termine a interrupcao e faca um yield da thread atual
}

void
thread_print_stats (void) 
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
          idle_ticks, kernel_ticks, user_ticks);
}

/* ao criar uma thread, precisa: 
    * alocar memória pra nova thread (4KB)
    * criar e preencher struct da thread
    * montar pilha (stack)
    * colocar a thread na ready_list
    depois o scheduler coloca a thread pra executar

  no início da página fica a struct da thread e no final a pilha (stack)
  
*/
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux) 
{
  struct thread *t;
  struct kernel_thread_frame *kf; 
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;

  ASSERT (function != NULL);

  /* aloca página de 4KB */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL) // se não conseguir memória, 
    return TID_ERROR; //a criação falha

  init_thread (t, name, priority); // inicia estrutura básica da thread (status, prioridade, ect)
  tid = t->tid = allocate_tid (); //gera o id da thread

  kf = alloc_frame (t, sizeof *kf); //configura a pilha (stack), que cresce pra baixo
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* quando uma thread é criada, ela nunca executou antes 
    isso quer dizer que a tread não tem contexto, ñ tem end. de retorno, nem pilha
    então o pintOS fabrica contexto na pilha, para que possa ser usada posteriormente, 
    na troca de contexto, é procedimento de inicialização pra evitar que a cpu execute
    com lixo de memória
    obs: para isso servem as seguintes structs:
    (kernel_thread_frame, switch_entry_frame e switch_threads_frame)
    são como pedacos de pilha

  */

  ef = alloc_frame (t, sizeof *ef); // ajusta o contexto 
  ef->eip = (void (*) (void)) kernel_thread;

  // switch_threads() é a rotina assembly responsável por trocar contexto
  sf = alloc_frame (t, sizeof *sf); //frame pra switch_threads()
  sf->eip = switch_entry; 
  sf->ebp = 0;

  t->nice = thread_current()->nice;
  t->recent_cpu = thread_current()->recent_cpu;

  thread_unblock (t); //torna a thread pronta pra executar

  return tid; //thread criada (com sucsseo)
}

/* coloca a thread pra dormir (estado vira BLOCKED)
   e a thread não pode ser escalonada, até que acorde novamente
   essa função precisa das interrupções desligadas pra ser chamada
 */
void
thread_block (void) 
{
  ASSERT (!intr_context ()); // verificar se uma rotina de interrupção NÃO tá sendo executada
  ASSERT (intr_get_level () == INTR_OFF); // verifica se as interrupções estão desligadas
  //define o status da thread atual como BLOCKED
  thread_current ()->status = THREAD_BLOCKED; 
  schedule (); //chama o escalonador pra escolher outra thread da ready_list
}

/* colocar a thread bloqueada na lista de threads prontas e 
   mudar o estado para READY (fica APTA a executar)
  */
void
thread_unblock (struct thread *t) 
{
  enum intr_level old_level; //guarda uma cópia do estado anterior das interrupcoes

  ASSERT (is_thread (t));
  /* garantir que a operação seja "atômica" -> integridade da lista!
     interrupcoes podem estar ON ou OFF, aqui, 
     queremos que não haja interrupcoes, pra garantir a integridade da lista, 
     então vamos salvar o estado atual (ON ou OFF) das interrupções, 
     executar a modificação na lista e depois retornar ao estado inicial
  */
  old_level = intr_disable (); // retorna o antigo estado de interrupceos e depois desabilita interrupções
  ASSERT (t->status == THREAD_BLOCKED);
  list_insert_ordered (&ready_list, &t->elem, thread_compare_priority, NULL); //mudança para adicionar o elemento na lista já ordenado
  t->status = THREAD_READY; // atualiza status da thread pra READY
  intr_set_level (old_level); // restaura o estado anterior de interrupçoes (ON ou OFF)
}

const char *
thread_name (void) 
{
  return thread_current ()->name; /* retorna o NOME da thread que tá rodando */
}

struct thread *
thread_current (void) 
{
  struct thread *t = running_thread ();

  ASSERT (is_thread (t)); // confere se realmente é uma thread
  ASSERT (t->status == THREAD_RUNNING); //confere se está rodando

  return t; //retorna thread que está rodando
}

tid_t
thread_tid (void) 
{
  return thread_current ()->tid; //retorna o id da thread que tá rodando
}

/* FINALIZAR A THREAD:
  a thread já concluiu e deve ser eliminada do sistema, 
  mas ela não pode liberar sua memória sozinha
  o processo de destruição é feito em duas partes: 
    1. a thread atualiza o status pra DYING
    2. outra thread libera a memória dela depois da troca de contexto
*/
void
thread_exit (void) 
{ 
  // ESSA FUNCAO NÃO PODE SER CHAMADA DENTRO DE UMA INTERRUPCAO:
  ASSERT (!intr_context ()); // verifica se as interrupções estão DESLIGADAS

#ifdef USERPROG
  process_exit (); //fecha arquivos, destroi page tables, libera recursos
#endif
  intr_disable (); //desliga interrupcoes (integridade das estruturas)
  list_remove (&thread_current()->allelem); //remove a thread de todas as listas
  thread_current ()->status = THREAD_DYING; // atualiza o status da thread
  schedule (); // escolhe outra pra executar, troca de contexto
  NOT_REACHED ();
}

/* FUNCAO QUE PERMITE 'ABRIR MAO' DA CPU VOLUNTARIAMENTE
   a thread em execução simplesmente "cede" a cpu por um momento, 
   mas pode ser solicitada novamente pelo escalonador (continua em estado READY)
   se a thread for uma thread comum (não for a idle_thread), ela vai pra ready_list
   NÃO COLOCA A THREAD PRA DORMIR, APENAS CEDE A CPU!!
  */
void
thread_yield (void) 
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable (); //desliga interrupcoes
  //verificar se não é a idle_thread, pois idle_thread não vai pra ready_list
  if (cur != idle_thread)  // se não for:
    list_insert_ordered (&ready_list, &cur->elem, thread_compare_priority, NULL); //mudança para adicionar o elemento na ready_list já ordenado
  cur->status = THREAD_READY; //muda o estado pra READY
  schedule (); //aciona o escalonador pra colocar outra thread pra rodar 
  intr_set_level (old_level); //volta ao estado antigo de interrupções 
}

/* Invoke function 'func' on all threads, passing along 'aux'.
   This function must be called with interrupts off. */
void
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, allelem);
      func (t, aux);
    }
}

/* Sets the current thread's priority to NEW_PRIORITY. */
void
thread_set_priority (int new_priority) 
{
  thread_current ()->priority = new_priority;
}

/* Returns the current thread's priority. */
int
thread_get_priority (void) 
{
  return thread_current ()->priority;
}

/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice UNUSED) 
{
  thread_current()->nice = nice;  /*seta a thread atual e atualiza o nice*/
  /*refaz a ordem após atualizar o nice*/
  mlfqs_recalc_priority(thread_current(), NULL);
  thread_mlfqs_yield(); 
}

/* retorna o nice da thread */
int
thread_get_nice (void) 
{
  return thread_current()->nice;
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void) 
{
  return FP_INT_ROUND(FP_MUL_INT(load_avg,100));
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void) 
{
  return FP_INT_ROUND(FP_MUL_INT(thread_current()->recent_cpu,100));
}

/*criação das funções mlfqs para realização dos calculos e atualizações por clock*/
void mlfqs_increment_recent_cpu()
{
  if (thread_current() != idle_thread) /*checa tem thread em execuçaõ*/
        thread_current()->recent_cpu = FP_ADD_INT(thread_current()->recent_cpu, 1);
}

void mlfqs_recalc_priority(struct thread *t, void* aux UNUSED)
{
  if (t == idle_thread)
    return;

  // Formula: priority = PRI_MAX - (recent_cpu / 4) - (nice * 2)
  fixed_point cpu_div_4 = FP_DIV_INT (t->recent_cpu, 4);
  fixed_point sub1 = FP_SUB (INT_FP (PRI_MAX), cpu_div_4); 
  fixed_point sub2 = FP_SUB_INT (sub1, t->nice * 2);  
  
  t->priority = FP_INT_ROUND (sub2);

  /* manter a prioridade no limite exigido do PintOS */
  if (t->priority > PRI_MAX) t->priority = PRI_MAX;
  if (t->priority < PRI_MIN) t->priority = PRI_MIN;
}



void mlfqs_recalc_load_avg()
{
  int ready_threads = list_size(&ready_list) + (thread_current()!=idle_thread ? 1:0);
  load_avg = FP_ADD(FP_MUL(FP_DIV_INT(INT_FP(59), 60),load_avg),FP_MUL_INT(FP_DIV_INT(INT_FP(1), 60),ready_threads));
}

void recalc_recent_cpu(struct thread *t, void *aux UNUSED)
{
  t->recent_cpu = FP_ADD_INT(FP_MUL(FP_DIV(FP_MUL_INT(load_avg,2),FP_ADD_INT(FP_MUL_INT(load_avg,2),1)),t->recent_cpu),t->nice);
}

void mlfqs_recalc_all_recent_cpu()
{
  thread_foreach(recalc_recent_cpu, NULL); //thread_foreach faz um for para percorrer cada thread
}

void mlfqs_sort_ready_list(void)
{
    list_sort(&ready_list, insert_priority, NULL);
}

/*função para ordenar por ordem de prioridade*/
bool insert_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  struct thread *thread_a = list_entry(a, struct thread, elem); 
  struct thread *thread_b = list_entry(b, struct thread, elem);
  
  if (thread_a->priority == thread_b->priority) 
  {
    return false; //manter igual
  }
  
  // Caso contrário, quem tem a maior prioridade vem na frente
  return thread_a->priority > thread_b->priority;
}


/* Idle thread: executa quando nenhuma outra thread está pronta
    é usada em thread_start, mas depois disso nunca aparece na ready_list
    quando a ready_list está VAZIA, next_thread_to_run() retorna idle_thread!
  */
static void
idle (void *idle_started_ UNUSED) 
{
  struct semaphore *idle_started = idle_started_;
  idle_thread = thread_current ();
  sema_up (idle_started); //avisa thread_start que inicializou

  for (;;) 
    {
      /* Let someone else run. */
      intr_disable ();
      thread_block ();
      asm volatile ("sti; hlt" : : : "memory");
    }
}

/* inicio de toda thread criada com thread_create() */
static void
kernel_thread (thread_func *function, void *aux) 
{
  ASSERT (function != NULL);
  intr_enable ();       /* liga interrupçoes */
  function (aux);       /* executa funcao funcao do usuario*/
  thread_exit ();   
}

/* função utilizada pra saber qual thread está rodando */
struct thread *
running_thread (void) 
{
  uint32_t *esp;
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

/* verificar se é uma thread mesmo */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* inicia a thread (nome, prioridade, etc., toda estrutura)*/
static void
init_thread (struct thread *t, const char *name, int priority)
{
  enum intr_level old_level;

  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;
  t->priority = priority;
  t->magic = THREAD_MAGIC;

  old_level = intr_disable ();
  list_push_back (&all_list, &t->allelem);
  intr_set_level (old_level);
}

/* configuração da pilha */
static void *
alloc_frame (struct thread *t, size_t size) 
{
  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);
  t->stack -= size;
  return t->stack;
}

/*
  escolhe e retorna a próxima thread a ser escalonada
  a idle_thread é como se fosse um modo de suspensão
  -> quando não tem nada pra fazer, a cpu 'descansa', 
  colocando a idle_thread pra executar, q é uma thread que não faz nada
  */
static struct thread *
next_thread_to_run (void) 
{
  if (list_empty (&ready_list)) //verifica se ready_list está vazia 
    return idle_thread; // thread ociosa (só roda quando a ready_list está vazia)
  else // se ñ estiver vazia, retorna o primeiro elemento da ready_list (round-robin)
    return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/*  função chamada DEPOIS da troca de contexto, 
    nova thread que está rodando entra aqui
    essa função serve pra finalizar a troca de contexto
      aqui também ocorre a etapa 2 do thread_exit() 
      -> a nova thread libera a memória da thread 
      que estava em estado de DYING
    geralmente é chamada pelo thread_schedule() 
   */
void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();
  
  ASSERT (intr_get_level () == INTR_OFF); //interrupcoes desligadas 

  cur->status = THREAD_RUNNING; //nova thread é RUNNING
  thread_ticks = 0; //reseta o time slice

#ifdef USERPROG
  /* Activate the new address space. */
  process_activate (); 
#endif

  /* se a troca de contexto ocorreu de uma threda q está morrendo, 
    libera a memória */
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread) 
    {
      ASSERT (prev != cur);
      palloc_free_page (prev); //libera a memoria 
    }
}

/* ESCALONADOR: ESCOLHE QUEM VAI EXECUTAR E TROCA CONTEXTO ENTRE AS THREADS
   
*/
static void
schedule (void) 
{
  struct thread *cur = running_thread (); // thread que está rodando
  struct thread *next = next_thread_to_run (); //proxima thread
  struct thread *prev = NULL; //inicialização

  ASSERT (intr_get_level () == INTR_OFF); // checar se interrupções estao desligadas
  /* TODO:
   * Ver de usar o thread_block, mas para o schedule 
   * tem de verificar se uma thread esta bloqueada, alem de implementar 
   * o unblock com o tempo
   * */
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next) //verifica a necessidade da troca
    prev = switch_threads (cur, next); //troca contextos e salva a thread anterior
  thread_schedule_tail (prev); //chama a funcao pra finalizar a troca de contexto
}

/* determina o tid (id) de uma thread nova*/
static tid_t
allocate_tid (void) 
{
  static tid_t next_tid = 1;
  tid_t tid;
  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);
  return tid;
}

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);
