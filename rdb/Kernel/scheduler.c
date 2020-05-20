#include <scheduler.h>

int next_pid = 0;
int proc_counter = 0;

int started = 0;

s_node *curr;
s_node *aux_node = NULL;

void *scheduler(void *old_rsp)
{

    if (started == 0)
    {
        return old_rsp;
    }

    if ((uint64_t)old_rsp >= HEAP_START)
    {
        curr->pcb->rsp = old_rsp;
    }

    curr->pcb->p_counter--;
    if (curr->pcb->p_counter == 0 || curr->pcb->state != READY)
    {
        curr->pcb->p_counter = curr->pcb->priority;
        curr = findNextReady();
    }

    void *new_rsp = curr->pcb->rsp;

    if (aux_node != NULL)
    {
        free(aux_node->pcb->stack_start);
        free(aux_node->pcb);
        free(aux_node);
        aux_node = NULL;
    }

    return new_rsp;
}

s_node *findNextReady()
{
    int counter = 0;
    s_node *aux = curr->next;
    while (counter < proc_counter)
    {
        if (aux->pcb->state == READY && aux->pcb->blocked_by_sem == 0)
        {
            return aux;
        }
        aux = aux->next;
        counter++;
    }
    counter = 0;
    aux = curr->next;
    while (counter < proc_counter)
    {
        if (aux->pcb->state == INIT)
        {
            return aux;
        }
        aux = aux->next;
        counter++;
    }
    return NULL;
}

int addPCB(void *rsp, size_t priority, void *stack_start, void *bp, char fg, char *name, uint64_t input_id, uint64_t output_id)
{
    s_pcb *new_pcb;
    malloc(sizeof(s_pcb), (void **)&new_pcb);

    new_pcb->rsp = rsp;
    new_pcb->stack_start = stack_start;
    new_pcb->bp = bp;
    new_pcb->name = name;
    new_pcb->pid = next_pid;
    new_pcb->priority = priority;
    new_pcb->p_counter = priority;
    new_pcb->is_deletable = UN_BUEN_DIA_PARA_MORIR;
    new_pcb->fg = fg;
    new_pcb->input_id = input_id;
    new_pcb->output_id = output_id;
    new_pcb->state = READY;
    new_pcb->blocked_by_sem = 0;

    openPipeForProc(input_id, next_pid);
    openPipeForProc(output_id, next_pid);

    next_pid++;

    if (started == 0)
    {
        new_pcb->caller_pid = new_pcb->pid;
        new_pcb->is_deletable = DURO_DE_MATAR;

        init(new_pcb);
    }
    else
    {
        new_pcb->caller_pid = curr->pcb->pid;

        addProcess(new_pcb);

        if (fg == 1)
        {
            blockCurrentProcess();
        }
    }
    proc_counter++;

    return new_pcb->pid;
}

void init(s_pcb *new_pcb)
{
    s_node *aux;
    malloc(sizeof(s_node), (void **)&aux);

    aux->pcb = new_pcb;
    aux->next = aux;
    aux->prev = aux;

    curr = aux;

    started = 1;
}

void addProcess(s_pcb *new_pcb)
{
    s_node *new_node;
    malloc(sizeof(s_node), (void **)&new_node);

    new_node->pcb = new_pcb;
    new_node->next = curr;
    new_node->prev = curr->prev;
    curr->prev->next = new_node;
    curr->prev = new_node;
}

int changePriority(uint64_t pid, char priority)
{
    int counter = 0;
    s_node *aux = curr;
    while (counter < proc_counter)
    {
        if (aux->pcb->pid == pid)
        {
            aux->pcb->priority = priority;
            return 0;
        }
        aux = aux->next;
        counter++;
    }
    return -1;
}

int changeState(uint64_t pid, char state)
{
    int counter = 0;
    s_node *aux = curr;
    while (counter < proc_counter)
    {
        if (aux->pcb->pid == pid)
        {
            aux->pcb->state = state;
            return 0;
        }
        aux = aux->next;
        counter++;
    }
    return -1;
}

int unlockFromSem(uint64_t pid)
{
    int counter = 0;
    s_node *aux = curr;
    while (counter < proc_counter)
    {
        if (aux->pcb->pid == pid)
        {
            aux->pcb->blocked_by_sem = 0;
            return 0;
        }
        aux = aux->next;
        counter++;
    }
    return -1;
}

int lockToSem(uint64_t pid)
{
    int counter = 0;
    s_node *aux = curr;
    while (counter < proc_counter)
    {
        if (aux->pcb->pid == pid)
        {
            aux->pcb->blocked_by_sem = 1;
            return 0;
        }
        aux = aux->next;
        counter++;
    }
    return -1;
}

int kill(uint64_t pid)
{
    int counter = 0;
    s_node *aux = curr;
    while (counter < proc_counter)
    {
        if (aux->pcb->pid == pid)
        {
            if (aux->pcb->is_deletable == 0)
            {
                return -2;
            }
            if (aux->pcb->fg == 1)
            {
                s_pcb *parent = getProcessInfo(aux->pcb->caller_pid);
                if (parent->state == BLOCKED)
                {
                    parent->state = READY;
                }
            }

            uint64_t p_aux; //basura
            p_closePipe(aux->pcb->input_id, aux->pcb->pid, &p_aux);
            p_closePipe(aux->pcb->output_id, aux->pcb->pid, &p_aux);

            free(aux->pcb->stack_start);
            aux->prev->next = aux->next;
            aux->next->prev = aux->prev;
            proc_counter--;
            free(aux->pcb);
            free(aux);
            return 0;
        }
        aux = aux->next;
        counter++;
    }
    return -1;
}

int killCurrent()
{
    if (curr->pcb->is_deletable == 1)
    {
        if (curr->pcb->fg == 1)
        {
            s_pcb *parent = getProcessInfo(curr->pcb->caller_pid);
            if (parent->state == BLOCKED)
            {
                parent->state = READY;
            }
        }
        uint64_t p_aux; //basura
        p_closePipe(curr->pcb->input_id, curr->pcb->pid, &p_aux);
        p_closePipe(curr->pcb->output_id, curr->pcb->pid, &p_aux);

        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
        proc_counter--;
        aux_node = curr;
        return 0;
    }
    return -2;
}

s_pcb *getProcessInfo(uint64_t pid)
{
    int counter = 0;
    s_node *aux = curr;
    while (counter < proc_counter)
    {
        if (aux->pcb->pid == pid)
        {
            return aux->pcb;
        }
        aux = aux->next;
        counter++;
    }
    return NULL;
}

int getCurrentPid()
{
    if (curr == NULL)
    {
        return -1;
    }
    return (int)(curr->pcb->pid);
}

void blockCurrentProcess()
{
    curr->pcb->state = BLOCKED;
}

int blockProcess(uint64_t pid)
{
    int counter = 0;
    s_node *aux = curr;
    while (counter < proc_counter)
    {
        if (aux->pcb->pid == pid)
        {
            if (aux->pcb->state == READY)
            {
                aux->pcb->state = BLOCKED;
            }
            else if (aux->pcb->state == BLOCKED)
            {
                aux->pcb->state = READY;
            }
            return 0;
        }
        aux = aux->next;
        counter++;
    }
    return -1;
}

void p_getMyI(uint64_t *resp)
{
    *resp = curr->pcb->input_id;
}

void p_getMyO(uint64_t *resp)
{
    *resp = curr->pcb->output_id;
}

void printProcessInfo(uint64_t pid)
{
    int counter = 0;
    s_node *aux = curr;
    while (counter < proc_counter)
    {
        if (aux->pcb->pid == pid)
        {
            printString("--------------------PROCESS--------------------", 47);
            printNewLine();
            printPCB(aux->pcb);
            printString("-----------------------------------------------", 47);
            printNewLine();
        }
        aux = aux->next;
        counter++;
    }
    return;
}

void printAllProcessInfo()
{
    printString("--------------------PROCESS--------------------", 47);
    printNewLine();
    int counter = 0;
    s_node *aux = curr;
    while (counter < proc_counter)
    {
        printPCB(aux->pcb);
        aux = aux->next;
        counter++;
    }
    printString("-----------------------------------------------", 47);
    printNewLine();
    return;
}

void printPCB(s_pcb *pcb)
{
    printString("name: ", 6);
    printString(pcb->name, 20);
    printString("  |  ", 5);
    printString("pid: ", 5);
    printDec(pcb->pid);
    printString("  |  ", 5);
    printString("st: ", 4);
    printDec(pcb->state);
    printString("  |  ", 5);
    printString("bs: ", 4);
    printDec(pcb->blocked_by_sem);
    printString("  |  ", 5);
    printString("fg: ", 4);
    printDec(pcb->fg);
    printString("  |  ", 5);
    printString("prty: ", 6);
    printDec(pcb->priority);
    printString("  |  ", 5);
    printString("i_id: ", 6);
    printDec(pcb->input_id);
    printString("  |  ", 5);
    printString("o_id: ", 6);
    printDec(pcb->output_id);
    printString("  |  ", 5);
    printString("cpid: ", 6);
    printDec(pcb->caller_pid);
    printNewLine();
    printString("             | ", 15);
    printString("sp: ", 4);
    print64Hex(((uint64_t)pcb->rsp));
    printString("  |  ", 5);
    printString("bp: ", 4);
    print64Hex(((uint64_t)pcb->bp));
    printString("  |  ", 5);
    printString("ss: ", 4);
    print64Hex(((uint64_t)pcb->stack_start));
    printNewLine();
}
