# Tema 1 SO - Loader de Executabile
### Ioan Teodorescu; 323CB

Tema are ca scop implementarea unui loader, sub forma unei biblioteci dinamice, ce plaseaza un fisier executabil in memorie, pagina cu pagina. In interfata de utilizare a bibliotecii, se afla functia 'so_init_loader', care realizeaza initializarea bibliotecii. Cu alte cuvinte, functia pregateste handler-ul pentru page fault, care este fix functia pe care trebuie sa o implementam, 'segv_handler'. Aceasta este apelata de fiecare data cand programul nostru ajunge la un page fault. Din moment ce flag-ul 'SA_SIGINFO' este setat pe campul 'sa_flags', sa_sigaction primeste functia de tip signal-handling (in cazul nostru 'segv_handler'), care va fi apelata de fiecare data cand apara o eroare cu valoare signum, cu scopul tratarii acesteia (erorii)[1][2].

Odata ce am intrat in segv_handler, confirm daca numarul semnalului (signum) este egal cu cel al semnalului `SIGSEGV` (11). Daca nu, voi apela functia signal(), setand semnalul si handler-ul default (SIG_DFL) al acestuia [3]. Stiind ca tipul erorii trimise este `SIGSEGV`, verific daca si_code este egal `SEGV_ACCERR` (si_code este o valoare care indică de ce a fost trimis acest semnal; `SEGV_ACCERR` este un flag pentru permisiunile nevalide pentru obiectul mapat). Daca este adevarat, asta inseamna ca încearcă un acces la memorie nepermis (segmentul respectiv nu are permisiunile necesare) si se apeleaza handler-ul default.

A mai ramas un caz pentru tratarea page fault-urilor si acesta are loc daca si_code este egal cu `SEGV_MAPPER`, insemnand faptul ca adresa / pagina se afla intr-o zona ne-mapata si trebuie sa o mapam, sa copiem din segmentul din fisier datele, si sa ii atribuim permisiunile necesare segmentului. Pentru inceput, apelez functia `'find_mySegment'`, care gaseste segmentul care contine adresa 'address' (adresa din memorie care a cauzat page fault-ul). Daca nu se gaseste niciun segment, returneaza `NULL`, insemnand ca nu am gasit niciun segment si apelez handler-ul default. Daca se gaseste segmentul care sa contina adresa la care s-a cauzat fault-ul, asta inseamna ca putem mapa o pagina in aceasta. Voi folosi urmatoarele variabile: 
- **char *vaddr** = adresa virtuala a segmentului
- **int pageNo** = numarul paginii din segment care contine adresa fault
- **int pageOffset** = offsetul paginii din segment
- **void *writingPos** = adresa de scriere a paginii
- **void *fileInterval** = adresa de sfarsit a intervalului de scriere din fisier*

*Pentru maparea paginii, folosesc urmatoarea instructiune [4]:
  ```  mmap(writingPos, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE, fd, mySegment->offset + pageOffset);```

	= voi mapa la adresa de inceput a paginii in care se afla adresa page fault-ului
	= intrucat vom mapa pagina cu pagina, lungimea maparii va fi fix o pagina (4096 bytes)
	= ii atribui zonei de memorie drepturi de citire si de scriere pentru a lua date si file descriptor si a le pune in memorie (eventual, pentru a seta o parte pagina cu 0) 	
	= in zona de flags, folosesc `MAP_FIXED` pentru ca memoria sa fie mapata fix la writingPos si `MAP_PRIVATE`, astfel incat modificarile aduse memoriei/datelor mapate de catre procesul apelant vor fi vizibile numai pentru acesta.
	= folosesc file descriptorul 'fd' deoarece mmap va creea o noua mapare, conectata la bitii fisierului 'fd', din intervalul (offset, offset + pageOffset) [5]
	= mySegment->offset + pageOffset = zona de memorie de la care incepe sa fie mapat fisierul*

Astfel, functia va genera o zona de memorie din spatiul de adresa al procesului.

*Exista cazuri in care adresa de mapare este in afara intervalului dimensiunii în cadrul fișierului, zona care trebuie e plina cu 0. Cu alte cuvinte, daca pagina nu se termina in fisier, atunci trebuie sa scriu 0 in restul paginii.*

*Fac diferenta dintre adresa de sfarsit a intervalului (fileInterval) si adresa de scriere a paginii (writingPos) si daca e mai mic decat 0, lungimea (length) este setata cu 0 (fill va fi lungimea unei pagini), astfel incat voi seta toata pagina cu 0. Daca diferenta e pozitiva, scriu 0 pe restul paginii (instructiunea 'int fill = PAGE_SIZE - length;'). Daca pagina nu se termina in fisier, atunci nu mai trebuie sa setez 0 in memorie (length va fi mai mare decat PAGE_SIZE si fill va fi mai mic decat 0, iar memset-ul nu se va mai executa).*

*Ca ultim pas, voi atribui paginii permisiunile necesare segmentului care contine pagina respectiva si verific daca instructiunea a esuat.*

### Consider ca tema este utila, deoarece am dobandit cunostiinte noi despre semnale, memorie virtuala si gestiunea memoriei.

### Din punct de vedere al implementarii acestei teme, consider ca tema mea este cat de cat eficienta, dar probabil se putea si mai bine :).

### RESURSE FOLOSITE PENTRU REALIZAREA TEMEI

1: https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-04
2: https://man7.org/linux/man-pages/man2/sigaction.2.html
3: https://man7.org/linux/man-pages/man2/signal.2.html
4: https://pubs.opengroup.org/onlinepubs/7908799/xsh/mmap.html
5: https://www.gnu.org/software/libc/manual/html_node/Memory_002dmapped-I_002fO.html
6: https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-05
7: https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-06
8: https://elixir.bootlin.com/linux/latest/source/include/uapi/asm-generic/siginfo.h#L232
9: https://devarea.com/linux-writing-fault-handlers/
10: https://man7.org/linux/man-pages/man2/mprotect.2.html