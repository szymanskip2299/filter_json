## Projekt z przedmiotu HPC

### Zarys problemu

Badając sieci rzeczywiste, np. sieci cytowań, mamy czasami do czynienia z plikami .json zbyt wielkimi aby załadować je w całości do pamięci i analizować, zawierającymi jednocześnie znacznie więcej informacji niż jest potrzebne w danej analizie. Przydatna zatem jest możliwość odfiltrowania interesujących nas wierzchołków sieci (np. takich dla których mamy wszystkie wymagane do danej analizy informacje), oraz wybranie tylko tych informacji na ich temat, które będą nam potrzebne. Duża wielkość tych plików oznacza, że podejście wielowątkowe jest w stanie osczędzić dużo czasu. Przykładem pliku, na którym program był testowany jest zbiór danych Citation Network Dataset w wersji V14 dostępny na stronie https://www.aminer.cn/citation, wielkości ok 20GB. Program został napisany w języku C++ z wykorzystaniem technologi openMP
 
Program przyjmuje jako dane wejściowe plik .json, będący listą obiektów json, kolejne elementy listy oddzielone są enterami. Ze względu na ograniczenia pamięci program wczystuje dane w postaci chunków, każdy wielkości nie większej niż parametr BUFF_SIZE (domyślnie 1GB). Program wybiera te dane które posiadają nie puste atrybuty id, authors, references, year oraz doc_type, ten ostatni dodatkowo przyjmuje wartość "Journal". Następnie program dla tych danych zapisuje je do pliku wyjściowego tylko z wybranymi atrybutami. Zarówno warunki wybrania danych jak i atrybuty które są zachowywane można łatwo zmodyfikować w kodzie.  Dodatkowo, ponieważ zarówno autorzy jak i artykuły posiadają dlugie stringi jako identyfikatory, aby zmniejszyć wymaganą pamięć potrzebną do analizy, oraz ułatwić ją od strony technicznej, każdy identyfikator został zamieniony na kolejną, unikalną liczbę naturalną.

### Kompilacja i uruchamianie

Do kompilacji używana jest następująca komenda 
```
g++ -std=c++11 filter_json.cpp -o filter_json -lm -O3 -fopenmp
```
wymaga ona c++11, zatem nie ma możliwości uruchmoić jej na dwarfie.

W nagłówku możemy zmienić następujące parametry:
BUFF_SIZE - wielkość danych wczytywanych na raz do pamięci w bajtach
file_in - nazwa pliku wejściowego
file_out - nazwa pliku wyjściowego
max_size - maksymalna wielkość danych, która będzie rozpatrywana w bajtach (do wyznaczania skalowania słabego)

Program uruchamiany jest za pomocą komendy
```
./filter_json
```
po wcześniejszym wykonaniu komendy 
```
export OMP_NUM_THREADS=3
```
do wybrania ilości wątków.

### Możliwe udoskonalenia kodu

Ponieważ kod wymaga kilku sekcji krytycznych, co wpływa na szybkość wykonania programu przy wielu wątkach, program mógłby zostać przyspieszony poprzez umożliwienie wątkom na prace nad kolejnami liniami kody w trakcie gdy sekcja krytyczna jest zablokowana przez inny wątek. Możnaby to osiągnąć poprzez zamienienie sekcji krytycznych przez lock, a następnie zamiast natychmiast próbować do niego wejść sprawdzić czy jest zamknięty i jeśli tak to zapisywanie obiektu jline np. do kolejki, a następnie kontunuowanie działania programu. Gdy wątek następnym razem uzyska dostęp do locku przeprowadza działania również dla obiektów jline w kolejce. Oczywiście rozmiar kolejki musiałby być ograniczony aby nie zabierać zbyt dużo pamięci, wtedy wątek normalnie czeka na otwarcie locka.

### Skalowanie silne
Testy przeprowadzono na części pliku wielkości 6GB.

![obraz](https://github.com/szymanskip2299/filter_json/assets/56300609/ec0fff6a-5e4d-4491-b182-cb10148f6cc1)


### Skalowanie słabe
Testy przeprowadzono na części pliku wielkości 2GB razy liczba wątków.

![obraz](https://github.com/szymanskip2299/filter_json/assets/56300609/f71d500d-47aa-47c4-8c2e-ba4ecf0e9bff)

### Wycieki pamięci
Program został przetestowany pod kątem wycieków pamięci z wykorzystaniem narzędzia Valgrind. Poniżej przedstawiono wynik z tego narzędzia.
```
==4326== Memcheck, a memory error detector
==4326== Copyright (C) 2002-2015, and GNU GPL'd, by Julian Seward et al.
==4326== Using Valgrind-3.11.0 and LibVEX; rerun with -h for copyright info
==4326== Command: ./filter_json
==4326== 
1073741824
134213881
268429996
402641029
536857231
671072256
805287849
939503860
1073721130
# COMPUTATION TIME: 1185.716433 sec
==4326== 
==4326== HEAP SUMMARY:
==4326==     in use at exit: 76,928 bytes in 10 blocks
==4326==   total heap usage: 130,466,086 allocs, 130,466,076 frees, 9,075,786,870 bytes allocated
==4326== 
==4326== 1,520 bytes in 5 blocks are possibly lost in loss record 4 of 6
==4326==    at 0x4C2FB55: calloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==4326==    by 0x40138E4: allocate_dtv (dl-tls.c:322)
==4326==    by 0x40138E4: _dl_allocate_tls (dl-tls.c:539)
==4326==    by 0x55FC26E: allocate_stack (allocatestack.c:588)
==4326==    by 0x55FC26E: pthread_create@@GLIBC_2.2.5 (pthread_create.c:539)
==4326==    by 0x51CB99F: ??? (in /usr/lib/x86_64-linux-gnu/libgomp.so.1.0.0)
==4326==    by 0x51C7CB9: GOMP_parallel (in /usr/lib/x86_64-linux-gnu/libgomp.so.1.0.0)
==4326==    by 0x402A5A: main (in /home/vboxuser/Desktop/proj/filter_json)
==4326== 
==4326== LEAK SUMMARY:
==4326==    definitely lost: 0 bytes in 0 blocks
==4326==    indirectly lost: 0 bytes in 0 blocks
==4326==      possibly lost: 1,520 bytes in 5 blocks
==4326==    still reachable: 75,408 bytes in 5 blocks
==4326==         suppressed: 0 bytes in 0 blocks
==4326== Reachable blocks (those to which a pointer was found) are not shown.
==4326== To see them, rerun with: --leak-check=full --show-leak-kinds=all
==4326== 
==4326== For counts of detected and suppressed errors, rerun with: -v
==4326== ERROR SUMMARY: 1 errors from 1 contexts (suppressed: 0 from 0)
```
Narzędzie znalazło potencjalny wyciek pamięci wynikający z użycia openMP (funkcja GOMP_parallel), natomiast są to najprawodpodobniej fałszywe pozytywy (na podstawie np. https://github.com/dmlc/xgboost/issues/8238). Ponadto przeprowadzenie tego samego testu na mniejszej ilości danych prowadziło do identycznej liczby potencjalnie straconych bajtów. Oznacza to, że najprawdopodobniej program nie ma żadnych wycieków pamięci, a w najgorszym nie stanowi zagrożenia nawet dla bardzo dużej ilości danych wejściowych i długiego czasu działania.
