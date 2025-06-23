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

Un'altra popolare alternativa è [Clang](https://clang.llvm.org/), un frontend per LLVM. Per i nostri scopi, lo ignoreremo.

## Il nostro primo programma in C
Quando si impara un nuovo linguaggio di programmazione, si è soliti scrivere un programma di 'Hello World'. Questo è il più semplice programma che si può scrivere, che mostra su schermo solamente la frase "Hello World!", e può essere utile per capire la sintassi basilare della lingua.
Eccolo in C. Prima di esaminarlo, salvalo su un file di testo e cerchiamo di compilarlo e eseguirlo.

```c
#include <stdio.h>

void main() {
  printf("Hello World");
}
```

## Compiliamo Hello World
Nella guida mostrerò esempi usando gcc/mingw.
Il compilatore GCC è un programma da terminale, non ha alcuna interfaccia grafica.
È un programma abbastanza complesso, con una miriade di opzioni e flags passabili come argomenti.
Per i nostri scopi però, il suo utilizzo è abbastanza semplice.
Per compilare un singolo file sorgente in C, basta richiamare gcc e indicargli il nome del file:
```bash
gcc file_sorgente.c
```
Questo comando genera un file eseguibile chiamato di default "a.out" (o "a.exe" se sei su Windows), nella cartella corrente, che può essere eseguito immediatamente.
Se il file sorgente non è un programma valido, ovvero, sono risultati degli errori di sintassi, la compilazione verrà annullata, e verrà mostrata una lista degli errori con una breve descrizione. Dovete **assolutamente** prendere dimestichezza con gli errori di compilazione, nonostante possano sembrare criptici in certe occasioni. Questa abilità è di vitale importanza, come vedremo nel corso delle lezioni.

Se stai usando un terminale Unix o un emulatore come MSYS2 su Windows, può tornare utile concatenare i comandi con `&&`:
```bash
gcc file_sorgente.c && 
```
Possiamo scegliere un nome per il file eseguibile generato con la flag `-o`
```bash
gcc file_sorgente.c -o nome_eseguibile
```

Tenete BENE a mente, che GCC mostra solamente gli errori fatali durante la compilazione. Esiste una categoria di errori, i warnings (o avvisi), che sono nascosti di default. I warnings non invalidano la correttezza del programma, quindi un programma con warnings può comunque venire compilato. I warnings però potrebbero causare errori semantici o logici, quindi è sempre buona norma correggerli.
Per far mostrare a GCC i warning di compilazione, esistono diverse flag.
Consiglio **vivamente** di compilare sempre con le flag `-Wall` (Warning all) e `-Wextra` (Warning extra).

Per ora, ci basta sapere solo questo. Nelle prossime lezioni vedremo come compilare multipli file sorgente in un solo eseguibile, linkare librerie, e automatizzare il comando di compilazione con tool come make.

Ti invito a provare a cambiare la scritta 'Hello World!' tra doppi apici, ricompilare ed eseguirlo, e vedere cosa succede. Ti invito anche a rimuovere i doppi apici, oppure a inserire un doppio apice all'interno della frase. Il compilatore si lamenta?
Gioca col sorgente e vedi che errori vengono lanciati.

## Analizziamo Hello World
C'è un bel po' da estrapolare da queste poche righe.
Non preoccuparti se non capirai cosa sta succedendo anche dopo aver letto l'analisi a seguire. É del tutto normale trovare fuorvianti le mie spiegazioni al momento. Ogni cosa a suo tempo: per ora siamo solo interessati ad avere un programma funzionante. Nelle prossime lezioni, inizieremo a sporcarci le mani e a capire dettagliatamente tutti i concetti del C.

La riga 1, `#include <stdio.h>` è una *direttiva*. Per ora, la ignoreremo. Accontentiamoci col dire che è necessaria per potere usare la funzione printf (che vedremo a breve).

Ora, un programma in C consiste da un'insieme di **funzioni** e **variabili**.
Una funzione è una sequenza di **statement** (o, italianizzato, istruzioni). Le istruzioni sono ordinate, ovvero eseguite nell'ordine in cui si presentano, dall'alto verso il basso.
Una variabile è un contenitore a cui vengono assegnati dei valori, usati durante la sequenza di istruzioni.

Funzioni e variabili hanno un nome, e siamo liberi di assegnargnene qualunque noi vogliamo.
La funzione 'main', che vediamo a riga 3, `void main() {`, è speciale. Il programma compilato inizierà l'esecuzione proprio da questa funzione. Ogni programma deve avere una funzione main.

Nota come dopo `main()` c'è una graffa aperta `{`, e a riga 5 la graffa si chiude `}`.
Le graffe contengono un *blocco*. Un blocco racchiude 'dentro' di sé un iniseme di istruzioni, in questo caso, le istruzioni della funzione main.

Nota come abbiamo aggiunto un tab `\t` ad ogni riga interna al blocco.
In questo modo creiamo un aiuto visivo, che ci indica che siamo dentro ad un blocco: questo aiuta notevolmente la leggibilità del codice, ed è buona norma usarle sempre. Più avanti vedremo che blocchi possono essere innestati uno dentro l'altro, e aumenteremo i tab in base alla profondità del blocco.

A riga 4, all'interno del main, chiamiamo la funzione printf. Il printf (print formatted) è una funzione molto importante. Con l'uso di qualche magia (che decifreremo piú avanti!), riesce a mostrare caratteri e parole sullo schermo del terminale!! In questo caso, sta mostrando la frase "Hello World!".
Notare come l'abbiamo scritto: abbiamo 'chiamato' printf scrivendo il suo nome, seguito da parentesi aperta `(`, la frase da mostrare, racchiusa da dei doppi apici `"`, poi parentesi chiusa `)`, e infine `;`.

Alcune funzioni richiedono dei parametri. Il printf richiede una *stringa*. Una stringa è semplicemente una sequenza di caratteri. Le stringhe sono racchiuse dai doppi apici `"`.Quindi, "Hello World!\n" è una stringa. I doppi apici sono necessari. Se vengono omessi, il compilatore si lamenterà di non riconoscere il simbolo Hello. Non farti spaventare dal `\n` alla fine della stringa. Ti invito a toglierlo e vedere cosa succede. Eh si, è semplicemente un carattere di nuova linea.
I caratteri seguiti da `\` sono chiamati caratteri di 'escape', e vengono interpretati come un carattere unico, quando dentro una stringa.
Per ora, useremo le stringhe solamente con la funzione printf.

Il punto e virgola finale `;` non è niente di speciale: funge da terminatore per l'istruzione. Tutte le istruzioni che scriveremo devono essere terminate da `;`.

Voglio farvi notare che il compilatore **ignora** tutte le tab, e anche certi spazi: non è necessario inserirle per avere un programma corretto (ti invito ancora a togliere e aggiungere spazi a caso e ricompilare). Le stiamo inserendo solamente per nostra comodità visiva. Questo non significa che tu non debba usarle. USALE SEMPRE E COMUNQUE. Più avanti capiremo anche del perché il compilatore ignori questi spazi.

Okay, abbiamo finito. Purtroppo, potrà ancora sembrare tutto molto ambiguo e criptico. Questo perché è ancora presto per spiegare dettagliatamente cosa sia una funzione, ma ogni cosa a suo tempo. Ti chiedo di resistere ancora un po'.

Nella prossima lezione, continueremo a giocare con printf e introdurremo le variabili.

## Esercizi
- Prepara un ambiente di sviluppo caldo e amorevole.
- Copia il sorgente di Hello World e assicurati di riuscire a compilarlo dal terminale ed eseguirlo.
- Gioca con il sorgente di Hello World. Prova a separare la stringa di "Hello World\n" in piú chiamate di printf.
