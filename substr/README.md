# Substr-task
Все сниппеты приведённые ниже, начинающиеся со знака `$`, должны рассматриваться как команды `bash`.

## Задание
Вам требуется написать программу на языке **C++** в файле `solution.cpp`, которая:
- Принимает два аргумента командной строки: имя входного файла и некоторую непустую строку. В случае, если количество входных параметров не равно двум (`argc != 3`) - необходимо вернуть ошибку и вывести некоторое информативное сообщение в `stderr`.
- Выводит `Yes` если удалось найти заданную строку как подстроку в заданном файле;
- Возвращает некоторый ненулевой (например, -1) код ошибки в случае возникновения ошибки, а также читаемое сообщение об ошибке на `stderr`;
- Выводит `No` в случае, если не удалось найти строку и ошибки не произошло.
- Решение должно использовать функции из стандартной библиотеки C, а не stream I/O из C++. Также стоит предпочитать `malloc` вместо `new` и не стоит использовать `std::vector`.
- Программа должна работать за `O(n + m)`, где `n` - размер файла, а `m` - длина строки из аргумента..


## Пример
Допустим, в файле `foo.txt` лежит строка `ababar`, а файла `bar.txt` не существует вовсе.
```
$ ./solution foo.txt 'abar'
```
Должен вывести `Yes` и вернуть код ошибки 0.

```
$ ./solution foo.txt 'abaz'
```
Должен вывести `No` и вернуть код ошибки 0.
```
$ ./solution bar.txt 'abaz'
```
Должен лишь вернуть код ошибки `-1` и вывести любое (на ваше усмотрение) сообщение об ошибке на stderr, информирующее, что файла не существует.

## CMake
Помимо этого вам предлагается начать использовать систему автоматизации сборки `CMake`. Она широко распространена, а также будет использована во всех последующих заданиях. Все `CMake`-проекты определяются файлом `CMakeLists.txt`

1. `CMake` - это не система сборки, а способ её автоматизации. Определимся, где будут располагаться наши конфигурационные файлы и результаты сборки. Для этого предлагается создать отдельную директорию непосредственно **внутри** папки проекта:
```
$ mkdir _build_debug && cd _build_debug
```
2. Запустим процесс конфигурации для отладочного билда с санитайзерами:
```
$ cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -lasan -lpthread -fno-omit-frame-pointer" ..
```
3. Запустим процесс сборки:
```
$ cmake --build . -- -j4
```
4. После успешной сборки проекта в папке `_build_debug` должен появиться исполняемый файл `solution` - результат сборки, можем запустить его:
```
$ ./solution
```
## IDE
Весь предыдущий пункт может быть автоматизирован при помощи IDE, например, `CLion`.