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

Kod mógłby zostać udoskonalony poprzez dodanie możliwości wprowadzania argumentów przy pomocy lini komend bez kompilacji, w tym umożliwienie specyfikacji warunków filtorwania, np. z pomocą składni SQL(warto tutaj zwrócić uwagę, że wykorzystanie zwykłych narzędzi do wprowadzenia pliku json do bazy danych nie są owocne, gdzyż atrybuty references oraz authors są listami a zatem nie mogą być bezpośrednio umieszczone w tabeli). Dodatkowo ponieważ kod wymaga kilku sekcji krytycznych, co wpływa na szybkość wykonania programu przy wielu wątkach, program mógłby zostać przyspieszony poprzez umożliwienie wątkom na prace nad kolejnami liniami kody w trakcie gdy sekcja krytyczna jest zablokowana przez inny wątek.

### Skalowanie silne
Testy przeprowadzono na części pliku wielkości 6GB.
