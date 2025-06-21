+++
date = '2025-06-01T12:44:49+02:00'
draft = false
linkTitle = ''
title = 'HelloWorld'
summary = ''
author = 'Comba92'
tags = []
keywords = []
+++
## Introduzione
TODO

## Perché C
- linguaggio semplice e contenuto.
- linguaggio ancora oggi popolare e utilizzato ovunque.
- linguaggio di "basso livello", utile per imparare il funzionamento dei computer.
- linguaggio con difetti di design che aiutano il programmatore novizio a sviluppare disciplina e rigore.

## Perché NON C++
TODO
https://en.wikipedia.org/wiki/Criticism_of_C%2B%2B
https://dorinlazar.ro/why-c-sucks-2016-02-edition/
https://medium.com/@gooro0/why-cpp-is-a-bad-language-b6b6f1927cc4
https://whydoesitsuck.com/cpp-sucks-for-a-reason/
https://blog.askesis.pl/post/2021/03/cpp-sucks.html
https://news.ycombinator.com/item?id=26932505
https://news.ycombinator.com/item?id=11147031

## Cos'è un linguaggio di programmazione?
TODO
https://en.wikipedia.org/wiki/Programming_language

## Cos'è C?
TODO
https://en.wikipedia.org/wiki/C_(programming_language)


## Scegliere un compilatore per C
Purtroppo, scegliere un compilatore è spesso un dilemma per i principianti. Le alternative sono poche, e confusionarie.

Se stai usando un sistema Linux, probabilmente disponi già di un compilatore open source per C, il GCC (Gnu C Compiler). Questo è il compilatore *de facto* per C.

Se invece stai usando Windows, la situazione è complicata.
Microsoft offre il proprio compilatore proprietario, MSVC (Microsoft Visual C++). L'unico modo per usarlo è installare l'intero pacchetto di Visual Studio, l'IDE per lo sviluppo di Microsoft (qualche decina di gigabyte).
Se non vuoi usare MSVC, c'è un'alternativa: [MinGW](https://www.mingw-w64.org/).
https://en.wikipedia.org/wiki/Mingw-w64

MinGW è un port di GCC per Windows, accompagniato da un insieme di altri utili strumenti GNU (come make, ld, gdb, etc).
Installarlo per Windows è un po' bizzarro, dato che la sezione download offre diverse alternative.

Se preferisci un ambiente di sviluppo completo e semplice, scegli Cygwin o il più aggiornato MSYS2: sono essenzialmente 'emulatori' di un terminale Unix, e che possono usare tutti i programmi per Linux su una macchina con Windows.
Sono molto simili a [WSL](https://learn.microsoft.com/en-us/windows/wsl/) l'emulatore di Linux ufficiale Microsoft.

Se invece sei un minimalista, e tutti questi emulatori ti sembrano troppo complicati, ti consiglio [w64devkit](https://github.com/skeeto/w64devkit), oppure [WinLibs](https://winlibs.com/).
Questi semplicemente offrono una zip con MinGw, che dovrai salvare su una cartella a tua scelta, e impostare le [variabili d'ambiente](https://it.wikipedia.org/wiki/Variabile_d%27ambiente) manualmente, in modo da poter richiamare gcc dal terminale di Windows.

Un'altra popolare alternativa è [Clang](https://clang.llvm.org/), un frontend per LLVM. Dato che stiamo già rendendo tutto più complicato di come dovrebbe essere, per i nostri scopi lo ignoreremo.

## Il nostro primo programma in C
Quando si impara un nuovo linguaggio di programmazione, si è soliti scrivere un programma di 'Hello World'. Questo è il più semplice programma che si può scrivere, che mostra su schermo solamente la frase "Hello World!", e può essere utile per capire la sintassi basilare della lingua.
Eccolo in C. Prima di esaminarlo, salvalo su un file di testo e cerchiamo di compilarlo e eseguirlo.

```c
#include <stdio.h>

void main() {
  printf("Hello World");
}
```

## Compiliamo il programma
TODO: parla di come si compila il programma
Ti invito a provare a cambiare la scritta 'Hello World!' tra doppi apici, ricompilare ed eseguirlo, e vedere cosa succede. Ti invito anche a rimuovere i doppi apici, oppure a inserire un doppio apice all'interno della frase. Il compilatore si lamenta?
TODO: parla degli errori di sintassi
TODO: parla di come automatizzare la compilazione ed esecuzione con make

## Analizziamo Hello World
C'è un bel po' da estrapolare da queste poche righe.
La riga `#include <stdio.h>` è una *direttiva*. Per ora, accontentiamoci col dire che è necessaria per potere usare la funzione printf (che vedremo a breve).

La riga `void main() {` è importante. Il 'main' è una *funzione*. Possiamo pensare ad una funzione come ad una raccolta di istruzioni, a cui diamo un nome e possiamo 'chiamare' da altre parti del codice.
Il 'main' è l'unica funzione **speciale** di C. Nota come dopo `main()` c'è una graffa aperta `{`, e a riga 5 la graffa si chiude `}`.
Le graffe contentono *blocchi*. Un blocco racchiude 'dentro' di sé un iniseme di istruzioni, in questo caso, le istruzioni della funzione main. Il main è speciale perché tutto ciò racchiuso nel main è ciò che verrà eseguito durante l'esecuzione del programma compilato!

Nota come abbiamo aggiunto un tab `\t` ad ogni riga interna al blocco.
In questo modo creiamo un aiuto visivo che ci indica che siamo dentro ad un blocco: questo aiuta notevolmente la leggibilità del codice, ed è buona norma usarle sempre. Più avanti vedremo che i blocchi si possono innestare uno dentro l'altro, e aumenteremo i tab in base alla profondità del blocco.

Voglio farvi notare che il compilatore **ignora** tutte le tab: non è necessario inserirle per avere un programma corretto (ti invito a rimuoverle e ricompilare). Le stiamo inserendo solamente per nostra comodità. Questo non significa che tu non debba usarle. USALE SEMPRE E COMUNQUE. Più avanti capiremo anche del perché il compilatore ignori tutte le tab, e anche certi spazi.

Arriviamo ora all'interno del main, alla funzione printf. Il printf (print formatted) è una funzione molto importante. Questa funzione, con l'uso di qualche magia, riesce a mostrare caratteri e parole sullo schermo del terminale!! In questo caso, sta mostrando la frase "Hello World". Notare come l'abbiamo scritto: abbiamo 'chiamato' printf scrivendo il suo nome, seguito da parentesi aperta `(`, la frase da mostrare, racchiusa da dei doppi apici `"`, poi parentesi chiusa `)`, e infine `;`.

Quando chiamiamo una funzione, questa richiederà dei parametri. Il printf richiede una *stringa*. Una stringa è semplicemente una sequenza di caratteri. Le stringhe sono racchiuse dai doppi apici `"`. Quindi, "Hello World" è una stringa. I doppi apici sono necessari. Se vengono omessi, il compilatore si lamenterà di non riconoscere il simbolo Hello. Questi linguaggi sono formali, un po' come la matematica, e devono seguire regole ferree.

Il punto e virgola finale `;` non è niente di speciale: funge da terminatore per l'istruzione. Tutte le istruzioni che scriveremo devono essere terminate da `;`.

Okay, abbiamo finito. Purtroppo, potrà ancora sembrare tutto molto ambiguo e criptico. Questo perché è ancora presto per spiegare dettagliatamente cosa sia una funzione, ogni cosa a suo tempo.
Nella prossima lezione, continueremo a giocare con printf e introdurremo le variabili.