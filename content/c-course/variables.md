+++
date = '2025-04-23T11:29:44+02:00'
draft = false
linkTitle = ''
title = 'Variables'
summary = ''
author = 'Comba92'
tags = []
keywords = []
+++
https://en.wikipedia.org/wiki/Variable_(computer_science)
https://en.wikipedia.org/wiki/Uninitialized_variable
https://en.wikipedia.org/wiki/Initialization_(programming)

https://en.wikipedia.org/wiki/Static_variable
https://en.wikipedia.org/wiki/Automatic_variable
https://en.wikipedia.org/wiki/Constant_(computer_programming)
https://en.wikipedia.org/wiki/Const_(computer_programming)
https://en.wikipedia.org/wiki/Assignment_(computer_science)

https://en.wikipedia.org/wiki/Declaration_(computer_programming)

https://en.wikipedia.org/wiki/Printf
https://en.wikipedia.org/wiki/Variadic_function

TODO: explain printf BEFORE

## Variabili e valori numerici
Le variabili sono l'atomo dei nostri programmi. Possiamo pensarle come dei 'contenitori', i quali hanno un nome, un tipo, e un valore assegnatoli del corrispettivo tipo.
Per ora, supponiamo esista un solo ed unico tipo, il tipo numerico.
Il tipo numerico è indicato dalla parola chiave `int` (integer, o numero intero in italiano).
Creare una variabile è facile.
Bisgona prima indicare il suo tipo, poi il suo nome, e un eventuale valore.
```c
int numero = 10;
```
Qui, ho creato una variabile di tipo intero, chiamata 'numero', e ci ho assegnato il valore 10. Il nome di una variabile spesso è anche tecnicamente chiamato 'identificatore'.
Il valore 10 invece, è chiamato costante, o valore letterale.
Il simbolo `=` indica un assegnamento. Attenzione che non è esattamente come l'uguale al quale sei abituato in matematica.
Una volta creata una variabile, possiamo riutilizzarla, assegnandoci altri valori.
```c
int numero = 10;
numero = 5;
```
Qui, abbiamo prima creato una variabile chiamata 'nome', assegnandogli 10. Subito dopo, ci assegniamo il valore 5. Il valore 10 di 'numero' è andato perso, sovrascritto da quello nuovo.
Quindi, a riga 1, numero conteneva 10, mentre a riga 2 e successive, numero conterrà 5.
Nota che non dobbiamo piú indicare 'int' prima del nome, dato che il tipo serve solamente quando creiamo per la prima volta una variabile.
```c
int numero1 = 5;
int numero2 = 6;
numero1 = numero2;
```
Qui, creiamo due variabili, assegnandogli dei valori. Poi, assegniamo il valore contenuto in 'numero2', a 'numero1'. In pratica, 'numero1' ora contiene il valore 6.
'numero2' rimane invariato, quindi contiene ancora 6.

Nota inoltre la *direzione* dell'assegnamento: *il valore di destra viene assegnato all'identificatore di sinistra*.
Se ti può aiutare, sappi che certi linguaggi usano questa notazione per l'assegnamento:
```c
numero <- 5;
```

Fare questo risulta in un errore di sintassi:
```c
5 = numero
```

Stiamo assegnando il valore contenuto in una variabile chiamata 'numero', alla costante 5. Ma questo non ha senso: il valore 5 non è un identificativo (non è una variabile valida). Tornando all'esempio delle scatole, 5 non è una scatola, ma è uno degli oggetti che puoi inserire nelle scatole, come un libro o un giocattolo. Capisci che non ha senso inserire qualcosa dentro ad un giocattolo???


## Dichiarazione e definizione
Dato che ci siamo, voglio anche introdurre la differenza tra *dichiarazione* e *definizione*.
Scrivere questo è legale:
```c
int numero;
```
Questo crea una variabile, senza assegnarci nulla.
Quando introduciamo un identificatore in questo modo, lo stiamo *dichiarando*.
Il programma a questo punto è a conoscenza dell'esistenza della variabile 'numero'. la variabile è però non inizializzata, non ci è stato ancora assegnato nulla.

Che numero contiene questo intero??
Una domanda ancora difficile da rispondere. Ma se vuoi uno spoiler: conterrà un valore casuale, spazzatura. Piú precisamente, conterrà il valore nella cella di memoria nel quale la variabile si ritroverà quando viene creata. Paura?

```c
int numero = 0;
```
Questo oltre che a creare una variabile, ci assegna anche un valore iniziale.
Quando introduciamo un identificatore insieme ad un inizializzazione, lo stiamo *definendo*.
Per ora, la dichiarazione sembra non avere molto senso, ma lo avrà piú avanti, soprattutto quando studieremo le funzioni. Per ora, inizializza sempre le variabili.
Linguaggi piú moderni spesso obbligano sempre l'inizializzazione delle variabili.

## Operatori aritmetici
Con le variabili di tipo intero, è possibile eseguire esepressioni numeriche. Possiamo usare le operazioni elementari, `+`, `-`, `*` (moltiplicazione), `/` (divisione). Possiamo usare C come una calcolatrice (w-wow...)!!

```c
int numero = 10;
int somma = 10 + 2;
int prodotto = somma * numero;
int divisione = prodotto / 2;
```
Copia questo snippet di codice nel tuo hello world, dopo la chiamata a printf, e verifica compili senza errori.
Tutto questo è molto bello, però, non vediamo nulla oltre che al familiare 'Hello World!'.
Il programma ha eseguito i calcoli che gli abbiamo indicato, ma non abbiamo indicato nulla riguardo al visualizzarli.
Dobbiamo scoprire come usare nuove utili funzionalità del nostro amico printf.


TODO: explain printf BEFORE
## La funzione printf()
Printf non solo può mostrare stringhe su schermo, ma anche valori numerici. Farlo è un po' peculiare però.
```c
int numero1 = 69;
int numero2 = 420;
printf("La variabile 'numero1' contiene %d, mentre 'numero2' contiene %d\n", numero1, numero2);
```
Copialo nel tuo programma ed eseguilo. Cosa vedi??
Nella stringa di printf, possiamo specificare dei 'formatter', caratteri speciali preceduti da `%` che indicano a printf di sostituire ai caratteri 'formatter' un valore.
Il formatter `%d` viene usato per indicare un valore intero. I valori effettivi vengono elencati dopo, come parametri della funzione. I parametri di una funzione sono separati da virgole `,`. In questo caso, il primo è la stringa, il secondo è il valore da sostituire al primo %d, e il terzo è il valore da sostituire al secondo %d.
(ti ricordo inoltre che `\n` è semplicemente l'escape di nuova linea, il fatto che sia attaccato a '%d\n' non causa nulla di particolare)
Le funzioni hanno di norma hanno un numero di parametri statico (che non cambia), ma printf è particolare, perché può cambiare il numero di parametri in base a quanti formatter si presentano nella stringa di formato. In gergo, printf è una [funzione variadica](https://en.wikipedia.org/wiki/Variadic_function).
Potrà sembrarti tutto molto complicato, ma non ti preoccupare.
Concentrati solamente a comprendere il funzionamento di printf per ora.

Ora possiamo finalmente vedere i risultati dell'esempio precedente:
```c
int numero = 10;
int somma = 10 + 3;
int prodotto = somma * numero;
int divisione = prodotto / 7;
printf("Numero = %d\n", numero);
printf("Somma = %d\n", somma);
printf("Prodotto = %d\nDivisione = %d\nUn numero simpatico = %d", prodotto, divisione, 69);
```
L'utilizzo di printf non solo è utile per visualizzare i risultati dei nostri programmi (sarebbero programmi inutili senza), ma è anche utile per il [debugging](https://en.wikipedia.org/wiki/Debugging). Con debugging si intende il processo di comprensione del codice. Vorrai spesso controllare i valori all'interno delle tue variabili, per assicurarti che tutto funzioni come hai immaginato.
Inizia a giocare con printf, con i valori interi e gli operatori. Sapere sfruttare al meglio printf per visualizzare i valori delle variabili sarà la tua skill piú importante, soprattuto durante il tuo apprendimento. **Sii sempre curioso, e spamma printf ovunque per scoprire cosa sta succedendo tra un istruzione e l'altra**.

Questo ultimo esempio esegue una divisione. Il risultato torna? Dove sono i decimali? Semplice, gli interi non possono rappresentare numeri con la virgola!! Esploreremo presto altri tipi di valori.

## L'importanza della documentazione
Printf ha molta piú funzionalità di quel che pensi (e penso). Risulta comodo esaminare la sua documentazione a volte. Printf è una funzione che non è stata scritta da noi, ma bensì deriva dalla liberia standard del C. Ricordi quel `#include <stdio.h>` in cima al file? stdio.h è un file header che include le *dichiarazioni* di funzioni provvedute dalla libreria standard, 
tra le quali quella di printf. Tutti i pezzi del puzzle si stanno lentamente connettendo.
Avremo tempo di esplorare i file di header in seguito.
C non dispone di una documentazione ufficiale, a differenza di altri linguaggi. Esistono svariati siti che ne offrono una diversa, ma penso che questo sito sia il piú chiaro e semplice. Dacci un'occhiata e prova a decifrarci qualcosa.
https://cplusplus.com/reference/cstdio/printf/

## Nominare una variabile
Se hai giocato un po' a dichiarare variabili, avrai notato che non tutti i nomi sono validi.
Le regole sintattiche di nominazione sono abbastanza semplici. Un identificativo può contenere solo lettere e numeri (quindi no spazi, e no simboli come `'` `"` `$` `!` e cosi via).
Il nome non può mai iniziare con un numero, ma può contenerne dopo la prima lettera (questo per non creare ambiguità tra identificativi e numeri costanti).
L'underscore `_` è considerato una lettera, ed è utile per simulare lo spazio.

Non sono validi identificativi uguali a parole chiavi del C, come 'if' o 'for'. Variabili con questi nomi sono errori di sintassi.
Inoltre, non è possibile definire una variabile o funzione con lo stesso identificativo due volte. Una volta creata una variabile chiamata 'numero', non è possibile crearne un'altra con lo stesso nome.

Per favore, non scegliere nomi come 'pippo' o 'paperino' (sto guardando te, programmatore boomer medio). Cerca di evitare anche nomi come 'x1', 'y2', 'n', o semplificazioni del genere. I nomi dovrebbero sempre essere descrittivi, e non troppo lunghi.

## Esercizi
- Implementa il calcolo dell'ipotenusa dati due cateti, usando il teorema di Pitagora. Avrai bisogno della funzione di radice quadrata, che è presente nella libreria standard. Riesci a scoprire come utilizzarla nel tuo programma?