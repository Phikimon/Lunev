## Task 1. FIFO
Write two programs, which have no parent-child relationship(i.e. one
of them doesn't `fork` and `execve` another when executed), and can be
executed in arbitrary order from different terminals.

***Producer*** program opens, reads the file and passes it to ***consumer*** via
**FIFO**, then ***consumer*** prints file contents to the screen.

***Requirements:***
0. Deadlocks are prohibited.
1. There can be arbitrary number of ***producers*** and ***consumers***.
2. Usage of file locks, semaphores, signals(even handling) and other
communication and synchronization facilities but FIFO is permitted.
3. Since no blocking synchronization syscalls must be present in
the program, one busy wait is allowed.
4. If after having a handshake ***producer*** or ***consumer*** dies, his
***partner*** should die too, and other ***producers***/***consumers*** should
not be prevented from transmitting/receiving.
5. If both ***partners*** die simultaneously(e.g. computer resets),
other ***producers***/***consumers*** should continue transmitting/receiving.

#### My solution explanation:
This task is kind of training, and requirement #2 forced me to spend about two
weeks to find some indirect and not obvious way to solve it.There are two key
ideas in this solution:

1. The first one is to use `rename` of a **FIFO** file as the atomic synchronization
function. Why **FIFO**, not regular file? In order to find whether the ***owner***,
i.e. the one who last renamed the file is alive.
The owner opens the **FIFO** with `O_RDWR` flag, and the process that checks
tries to open **FIFO** with `O_WRONLY|O_NONBLOCK` flag, which proceeds in case the
owner of the **FIFO** is dead, and otherwise returns `errno==ENXIO`. This is how
the `is_fifo_dead()` function works.
After calling `open`, process that wills to become ***owner*** of the **FIFO**(***capture*** it),
tries to rename **FIFO** to "*<fraction>\_LOCK\_<pid>*", where <fraction>={"CONS"; "PROD"};
<pid> is the id of the process obtained by `getpid` syscall. In case rename was
successful, the process is guaranteed to be exclusive ***owner*** of the FIFO. Otherwise
the procedure repeats. This is where the busy wait hides, though in practice the
consensus is obtained relatively quickly. This is how `capture_lock()` works.
**Example:** if at certain point of time in the directory there are **FIFO**s named
'*CONS_LOCK_158*' and '*PROD_LOCK_976*', this means that currently ***consumer***
with PID 158 is communicating to ***producer*** with PID 976.

2. Second idea is to use two additional synchonization **FIFO** files: "*SYNC_FIFO1*" and
"*SYNC_FIFO2*". "*SYNC_FIFO1*" is used by consumer to receive data from producer
(opened with `O_RDONLY|O_NONBLOCK`), and by producer to transmit data to
consumer(opened with `O_RDWR`). "*SYNC_FIFO2*" is used symmetrically vise-versa.
`O_RDWR` is used, first, in order not to block on `open`, if there is no ***partner*** alive
(to meet requirement #4), and in order not to die from `write` to the ***partner***
that was not born yet. Let\`s explain, why cannot nor `O_WRONLY` neither
`O_WRONLY|O_NONBLOCK` be used for the latter reason. Let's imagine there
is a newborn ***consumer*** that appeared to replace the abnormally died predecessor.
The ***producer***, which was the partner of previous ***consumer***, didn't get
enough time by the scheduler yet to close the file descriptors, so the newborn
***consumer*** doesn't block on open of *SYNC_FIFO*, then ***producer*** wakes up
and closes descriptors, and only then ***consumer*** tries to make first write in
this **FIFO**. Because nobody has opened this **FIFO** from reading side yet, he dies
from `SIGPIPE`, which he is disallowed to handle by the requirement #2.

**Synchonization sequence(key functions):**
1. Both ***consumers*** and ***producers*** call `capture_lock()`, and if the fraction
lock is free, one ***consumer***/***producer*** becomes the exclusive owner of the
"*<CONS/PROD>\_LOCK*"
2) After pushing other ***consumers***/***producers***, process seeks for handshake.
Handshake is the `MSG_BEGIN` message(sent in `enter_region`), which
is sent before the data transmission(which is performed in `pump_data()`
function. Also the `MSG_END` message(`leave_region()` function) is sent
after the transmission. Each of both partners sends the message and waits
for the answer or the `EPIPE`, which allows him to determine that current
***partner*** is dead. Also this check is made in `pump_data`.

### Tests
"*Die*" test looks approximately like this
```sh
    for (( i=1; i <= 1000; i++ ))
    do
        ./fifo_program $(INPUT_FILE) &
        ./fifo_program
    done
```
The result must be *1000* times printed contents of (arbitrary big) ```$(INPUT_FILE)```,
not intermixed with each other and not corrupted. Also, according to the requirements
\#4 and \#5, no *./fifo_program* process instances should be left alive or blocked. If we
manually kill *N* producers at arbitrary points of time, the contents of the ```$(INPUT_FILE)```
should appear on the screen *1000-N* times.

## Task 2. Message queues.

Write program that produces N children(N is the command line argument).
Then, after all of them are born, they print their numbers(in order of appearence).

***Requirements:***
1. `wait()` syscall family is permitted to be used.
2. Numbers shouldn't intermix in this test:
```sh
    for (( i=1; i <= 100; i++ ))
    do
        ./messages
    done
```
3. Only System V IPC message queues are allowed to be used. The queue
must be removed after the program is executed.

***Pitfall***:
By default buffering of *stdout* is linewise.

## Task 3. Shared memory and semaphores.
Write two programs, which have no parent-child relationship(i.e. one
of them doesn't `fork` and `execve` another when executed), and can be
executed in arbitrary order from different terminals.

***Producer*** program opens, reads the file and passes it to ***consumer*** via
**shared memory**, then ***consumer*** prints file contents to the screen.

***Requirements:***
0. Deadlocks are prohibited.
1. There can be arbitrary number of ***producers*** and ***consumers***.
2. Usage of any kind of communication and synchronization facilities but
System V IPC **semaphores** and **shared memory** is permitted.
3. No busy waits are allowed.
4. Any ***producer*** or ***consumer*** can die at any time, this should cause
***partner*** to die, but not prevent any other producers or consumers from
transmitting/receiving data.

### Extra goal achieved
The solution is easy to understand by just reading the code. You can note that
data transmission and synchronyzation of arbitrary number of process is
performed **by using only one semaphore**, and this is what I am proud of.

While trying to solve this task, I've comen across bug in Linux kernel, which
I a bit later [succeeded to fix](https://bit.ly/2qP0iht).

## Task 4. Signals
Write program that produces child.

Then child process opens file and transmits its contents to the parent. Parent
prints them to the screen.

***Requirements:***
1. Only UNIX signals `SIGUSR1`, `SIGUSR2` are allowed to be used.
2. If one of interlocutors dies, another should die too.

## Task 5. Proxy
Write program that produces *N* childs-"***slaves***"(*N* is the command line argument).
Each ***slave*** have buffer of size *X* bytes, and parent-***master*** has N buffers of size
*X \* 3^(N - i)* bytes, where *i* is the number of the buffer.

***Slave*** #0 opens file, then reads it and passes its contents to the parent-***master***.
***Master*** receives data in buffer #0(of the biggest size), then passes it to ***slave*** #1, then
receives it back in buffer #1 and so on.

The point is that the size of ***parent`s*** buffers decreases as the file contents goes towards the final
destination - ***slave*** #N, which prints what it receives to the screen.

***Requirements:***
1. Only **pipes** and `select()` syscall are allowed to be used as the communication and
synchronization facilities.
2. If one of ***slaves*** that didn't finish their work or ***master*** dies, transmission should stop
and all other processes should die.
3. No busy waites nor deadlocks are allowed.



















