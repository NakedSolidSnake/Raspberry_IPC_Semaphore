# _Semaphore System V_

## Tópicos
* [Introdução](#introdução)
* [Semaphore System V](#semaphore-system-v-1)
* [Systemcalls](#systemcalls)
* [ipcs](#ipcs)
* [Implementação](#implementação)
* [semaphore.h](#semaphoreh)
* [semaphore.c](#semaphorec)
* [launch_processes.c](#launch_processesc)
* [button_interface.h](#button_interfaceh)
* [button_interface.c](#button_interfacec)
* [led_interface.h](#led_interfaceh)
* [led_interface.c](#led_interfacec)
* [Compilando, Executando e Matando os processos](#compilando-executando-e-matando-os-processos)
* [Compilando](#compilando)
* [Clonando o projeto](#clonando-o-projeto)
* [Selecionando o modo](#selecionando-o-modo)
* [Modo PC](#modo-pc)
* [Modo RASPBERRY](#modo-raspberry)
* [Executando](#executando)
* [Interagindo com o exemplo](#interagindo-com-o-exemplo)
* [MODO PC](#modo-pc-1)
* [MODO RASPBERRY](#modo-raspberry-1)
* [ipcs funcionamento](#ipcs-funcionamento)
* [Matando os processos](#matando-os-processos)
* [Conclusão](#conclusão)
* [Referência](#referência)

## Introdução
Em programação Multithread ou Multiprocessos existem pontos onde é necessário compartilhar a mesma informação, normalmente conhecidos como variáveis globais, ou de forma mais charmosa: "variáveis de contexto". Onde essas variáveis guardam algum tipo de informação ou estado interno, e seu acesso de forma não sincronizada pode acarretar em um comportamento indesejável. Neste artigo será visto um IPC conhecido como Semaphore, que garante que isso tipo de coisa não aconteça.

## Semaphore System V
Semaphore System V diferente dos outros IPC's não é utilizado para transferir dados, mas sim para coordená-los. Para exemplificar tome uma variável global em um contexto onde se tem duas Threads, sendo a Thread A responsável por incrementar o conteúdo, e a Thread B responsável por decrementar esse conteúdo e estão concorrendo o acesso dessa posição de memória. Em um dado instante de tempo o conteúdo esteja com o valor 10, neste ponto a Thread A incrementa o conteúdo em 1 totalizando 11, porém no mesmo instante a Thread B decrementa totalizando 9. Por uma análise sequencial o valor esperado seria o próprio 10, mas ao fim totalizou 9 o que aconteceu aqui foi que as Threads concorreram resultando nesse resultado inesperado. Para garantir que o acesso seja feito de forma sincronizada é necessário estabelecer uma regra de acesso onde quando uma Thread estiver usando a outra precisa esperar para assim acessar. Os Semaphores podem ter dois tipos: contador e exclusão mútua.

## Systemcalls
Para usar Semaphores é necessário algumas funções sendo a primeira _semget_ que é responsável por criar o identificador do _Semaphore_.
```c
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int semget(key_t key, int nsems, int semflg);
```

_semop_ é usado para alterar os valores do _Semaphore_, essa função possui uma alternativa para ser usado com o _timeout_.
```c
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int semop(int semid, struct sembuf *sops, unsigned nsops);

int semtimedop(int semid, struct sembuf *sops, unsigned nsops, struct timespec *timeout);
```
_semctl_ controla diretamente as informações do _semaphore_
```c
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int semctl(int semid, int semnum, int cmd, ...);
```

## ipcs
A ferramenta ipcs é um utilitário para poder verificar o estado dos IPC's sendo eles: Queues, Semaphores e Shared Memory, o seu funcionamento será demonstrado mais a seguir. Para mais informações execute:
```bash
$ man ipcs
```

## Implementação
Para facilitar a implementação a API da Queue foi abstraída para facilitar o uso.

### semaphore.h
Nesse _header_ é criado dois _enums_ um para identificar o estado e o outro seu tipo. Foi criado uma estrutura que contém todos os argumentos necessários para manipular o _semaphore_
```c
typedef enum 
{
  unlocked,
  locked
} Semaphore_State;

typedef enum 
{
  slave,
  master
} Semaphore_Type;

typedef struct 
{
  int id;
  int sema_count;
  int key;
  Semaphore_State state;
  Semaphore_Type type;
} Semaphore_t;
```
Aqui é criada uma abstração que permite uma leitura mais simplificada de como manipular o _semaphore_.
```c
bool Semaphore_Init(Semaphore_t *semaphore);
bool Semaphore_Lock(Semaphore_t *semaphore);
bool Semaphore_Unlock(Semaphore_t *semaphore);
bool Semaphore_Destroy(Semaphore_t *semaphore);
```

### semaphore.c 
Para o controle do _semaphore_ é necessário uma estrutura equivalente a que foi usada na [queue](https://github.com/NakedSolidSnake/Raspberry_IPC_Queue_SystemV#systemcalls) que é usada para inicializar e destruir o _semaphore_
```c
union semun{
  int val;
  struct semid_ds *buf;
  unsigned short *array;
};
```

Aqui p _semaphore_ é inicializado utilizando uma chave, a quantidade, as permissões e flag de criação, logo em seguida é verificado qual o tipo e caso for o principal, seu valor recebe 1 isso permite que seja bloqueado em caso de uso.
```c
bool Semaphore_Init(Semaphore_t *semaphore)
{
  bool status = false;

  do 
  {
    if(!semaphore)
      break;

    semaphore->id = semget((key_t) semaphore->key, semaphore->sema_count, 0666 | IPC_CREAT);
    if(semaphore->id < 0)
      break;

    if (semaphore->type == master)
    {
      union semun u;
      u.val = 1;

      if (semctl(semaphore->id, 0, SETVAL, u) < 0)
        break;
    }

    status = true;

  } while(false);

  return status;
}
```
Para realizar o bloqueio utilizando a estrutura _sembuf_ com os valores {0, -1, SEM_UNDO} utilizando a função _semop_ após a operação setando seu estado para _locked_.
```c
bool Semaphore_Lock(Semaphore_t *semaphore)
{
  bool status = false;
  struct sembuf p = {0, -1, SEM_UNDO};

  do
  {
    if(!semaphore)
      break;

    if(semop(semaphore->id, &p, 1) < 0)
      break;

    semaphore->state = locked;
    status = true;
  } while(false);

  return status;
}
```

Para realizar a liberação utilizando a estrutura _sembuf_ com os valores {0, 1, SEM_UNDO} utilizando a função _semop_ após a operação setando seu estado para _unlocked_.
```c
bool Semaphore_Unlock(Semaphore_t *semaphore)
{
  bool status = false;
  struct sembuf v = {0, 1, SEM_UNDO};

  do
  {
    if(!semaphore)
      break;

    if(semop(semaphore->id, &v, 1) < 0)
      break;

    semaphore->state = unlocked;
    status = true;

  } while(false);

  return status;
}
```
Por fim, para remover o _semaphore_ é utilizada a função _semctl_ com a flag IPC_RMID com o identificador do _sempahore_.
```c
bool Semaphore_Destroy(Semaphore_t *semaphore)
{
  union semun sem_union;
  bool status = false;

  do 
  {
    if(!semaphore)
      break;

    if(semctl(semaphore->id, 0, IPC_RMID, sem_union) < 0)
      break;

    semaphore->state = unlocked;
    status = true;
  } while(false);

  return status;
}
```


Para demonstrar o uso desse IPC, será utilizado o modelo Produtor/Consumidor, onde o processo Produtor(_button_process_) vai escrever seu estado em uma mensagem, e inserir na queue, e o Consumidor(_led_process_) vai ler a mensagem da queue e aplicar ao seu estado interno. A aplicação é composta por três executáveis sendo eles:

* _launch_processes_ - é responsável por lançar os processos _button_process_ e _led_process_ através da combinação _fork_ e _exec_
* _button_interface_ - é responsável por ler o GPIO em modo de leitura da Raspberry Pi e escrever o estado em uma mensagem e inserir na queue
* _led_interface_ - é responsável por ler a mensagem da queue e aplicar em um GPIO configurado como saída


### *launch_processes.c*

No _main_ criamos duas variáveis para armazenar o PID do *button_process* e do *led_process*, e mais duas variáveis para armazenar o resultado caso o _exec_ venha a falhar.
```c
int pid_button, pid_led;
int button_status, led_status;
```

Em seguida criamos um processo clone, se processo clone for igual a 0, criamos um _array_ de *strings* com o nome do programa que será usado pelo _exec_, em caso o _exec_ retorne, o estado do retorno é capturado e será impresso no *stdout* e aborta a aplicação. Se o _exec_ for executado com sucesso o programa *button_process* será carregado. 
```c
pid_button = fork();

if(pid_button == 0)
{
    //start button process
    char *args[] = {"./button_process", NULL};
    button_status = execvp(args[0], args);
    printf("Error to start button process, status = %d\n", button_status);
    abort();
}   
```

O mesmo procedimento é repetido novamente, porém com a intenção de carregar o *led_process*.

```c
pid_led = fork();

if(pid_led == 0)
{
    //Start led process
    char *args[] = {"./led_process", NULL};
    led_status = execvp(args[0], args);
    printf("Error to start led process, status = %d\n", led_status);
    abort();
}
```

## *button_interface.h*
Para realizar o uso da interface de botão é necessário preencher os callbacks que serão utilizados pela implementação da interface, sendo a inicialização e a leitura do botão.
```c
typedef struct 
{
    bool (*Init)(void *object);
    bool (*Read)(void *object);
    
} Button_Interface;
```
A assinatura do uso da interface corresponde ao contexto do botão, que depende do modo selecionado, o _semaphore_, e a interface do botão devidamente preenchida.
```c
bool Button_Run(void *object, Semaphore_t *semaphore, Button_Interface *button);
```

## *button_interface.c*
A implementação da interface baseia-se em inicializar o botão, inicializar o _semaphore_, e no loop realiza o _lock_ do _semaphore_ e aguarda o pressionamento do botão que libera o _semaphore_ para outro processo(nesse caso o processo de LED) utilizar.
```c
bool Button_Run(void *object, Semaphore_t *semaphore, Button_Interface *button)
{
    if (button->Init(object) == false)
        return false;

    if(Semaphore_Init(semaphore) == false)
        return false;

    while(true)
    {
        if(Semaphore_Lock(semaphore) == true)
        {
            wait_press(object, button);
            Semaphore_Unlock(semaphore);
        }
    }

    Semaphore_Destroy(semaphore);
    return false;
}
```

## *led_interface.h*
Para realizar o uso da interface de LED é necessário preencher os callbacks que serão utilizados pela implementação da interface, sendo a inicialização e a função que altera o estado do LED.
```c
typedef struct 
{
    bool (*Init)(void *object);
    bool (*Set)(void *object, uint8_t state);
} LED_Interface;
```

A assinatura do uso da interface corresponde ao contexto do LED, que depende do modo selecionado, o _semaphore_, e a interface do LED devidamente preenchida.
```c
bool LED_Run(void *object, Semaphore_t *semaphore, LED_Interface *led);
```


## *led_interface.c*
A implementação da interface baseia-se em inicializar o LED, inicializar o _semaphore_, e no loop realiza o _lock_ do _semaphore_ e altera o seu estado e libera o _semaphore_ para outro processo(nesse caso o processo de Button) utilizar.
```c
bool LED_Run(void *object, Semaphore_t *semaphore, LED_Interface *led)
{
    int state = 0;
    if(led->Init(object) == false)
        return false;

    if(Semaphore_Init(semaphore) == false)
        return false;

    while(true)
    {
        if(Semaphore_Lock(semaphore) == true)
        {
            led->Set(object, state);
            state ^= 0x01;
            Semaphore_Unlock(semaphore);
        }
        else 
            usleep(_1ms);
    }
}
```

## Compilando, Executando e Matando os processos
Para compilar e testar o projeto é necessário instalar a biblioteca de [hardware](https://github.com/NakedSolidSnake/Raspberry_lib_hardware) necessária para resolver as dependências de configuração de GPIO da Raspberry Pi.

## Compilando
Para facilitar a execução do exemplo, o exemplo proposto foi criado baseado em uma interface, onde é possível selecionar se usará o hardware da Raspberry Pi 3, ou se a interação com o exemplo vai ser através de input feito por FIFO e o output visualizado através de LOG.

### Clonando o projeto
Pra obter uma cópia do projeto execute os comandos a seguir:

```bash
$ git clone https://github.com/NakedSolidSnake/Raspberry_IPC_Semaphore_SystemV
$ cd Raspberry_IPC_Semaphore_SystemV
$ mkdir build && cd build
```

### Selecionando o modo
Para selecionar o modo devemos passar para o cmake uma variável de ambiente chamada de ARCH, e pode-se passar os seguintes valores, PC ou RASPBERRY, para o caso de PC o exemplo terá sua interface preenchida com os sources presentes na pasta src/platform/pc, que permite a interação com o exemplo através de FIFO e LOG, caso seja RASPBERRY usará os GPIO's descritos no [artigo](https://github.com/NakedSolidSnake/Raspberry_lib_hardware#testando-a-instala%C3%A7%C3%A3o-e-as-conex%C3%B5es-de-hardware).

#### Modo PC
```bash
$ cmake -DARCH=PC ..
$ make
```

#### Modo RASPBERRY
```bash
$ cmake -DARCH=RASPBERRY ..
$ make
```

## Executando
Para executar a aplicação execute o processo _*launch_processes*_ para lançar os processos *button_process* e *led_process* que foram determinados de acordo com o modo selecionado.

```bash
$ cd bin
$ ./launch_processes
```

Uma vez executado podemos verificar se os processos estão rodando atráves do comando 
```bash
$ ps -ef | grep _process
```

O output 
```bash
cssouza  21140  2321  0 08:04 pts/6    00:00:00 ./button_process
cssouza  21141  2321  0 08:04 pts/6    00:00:00 ./led_process
```
## Interagindo com o exemplo
Dependendo do modo de compilação selecionado a interação com o exemplo acontece de forma diferente

### MODO PC
Para o modo PC, precisamos abrir um terminal e monitorar os LOG's
```bash
$ sudo tail -f /var/log/syslog | grep LED
```

Dessa forma o terminal irá apresentar somente os LOG's referente ao exemplo.

Para simular o botão, o processo em modo PC cria uma FIFO para permitir enviar comandos para a aplicação, dessa forma todas as vezes que for enviado o número 0 irá logar no terminal onde foi configurado para o monitoramento, segue o exemplo
```bash
$ echo '0' > /tmp/semaphore_file 
```

Output dos LOG's quando enviado o comando algumas vezez
```bash
Jun 30 08:12:29 dell-cssouza LED SEMAPHORE[21141]: LED Status: On
Jun 30 08:12:31 dell-cssouza LED SEMAPHORE[21141]: LED Status: Off
Jun 30 08:12:32 dell-cssouza LED SEMAPHORE[21141]: LED Status: On
Jun 30 08:12:32 dell-cssouza LED SEMAPHORE[21141]: LED Status: Off
Jun 30 08:12:33 dell-cssouza LED SEMAPHORE[21141]: LED Status: On
Jun 30 08:12:33 dell-cssouza LED SEMAPHORE[21141]: LED Status: Off
```

### MODO RASPBERRY
Para o modo RASPBERRY a cada vez que o botão for pressionado irá alternar o estado do LED.

## ipcs funcionamento
Para inspecionar as queues presentes é necessário passar o argumento -s que representa queue, o comando fica dessa forma:
```bash
$ ipcs -s
```
O Output gerado na máquina onde o exemplo foi executado
```bash
------ Semaphore Arrays --------
key        semid      owner      perms      nsems     
0x000004d2 0          cssouza    666        2 
```

## Matando os processos
Para matar os processos criados execute o script kill_process.sh
```bash
$ cd bin
$ ./kill_process.sh
```

## Conclusão
O _semaphore_ é um recurso muito útil que serve para sincronizar o acesso de regiões de memória compartilhada, conhecida também como sessão crítica, porém as vezes o seu uso pode acarretar problemas de deadlocks, o que torna difícil de depurar e encontrar o problema em um programa multithreaded ou multiprocesso. Normalmente é utilizado em conjunto com a Shared Memory outro IPC que será visto no próximo artigo. Mas para evitar esse tipo de preocupação uma solução mais adequada seria usar sockets ou queues para enviar as mensagens para os seus respectivos interessados.

## Referência
* [Link do projeto completo](https://github.com/NakedSolidSnake/Raspberry_IPC_Semaphore_SystemV)
* [Mark Mitchell, Jeffrey Oldham, and Alex Samuel - Advanced Linux Programming](https://www.amazon.com.br/Advanced-Linux-Programming-CodeSourcery-LLC/dp/0735710430)
* [fork, exec e daemon](https://github.com/NakedSolidSnake/Raspberry_fork_exec_daemon)
* [biblioteca hardware](https://github.com/NakedSolidSnake/Raspberry_lib_hardware)
