# Reazione a Catena - Progetto Sistemi Operativi 2023/24

## Descrizione
Il progetto simula una reazione a catena utilizzando processi multipli per gestire la simulazione e la produzione di energia. Sono incluse sincronizzazioni con lock, IPC per la comunicazione tra i vari processi e l'utilizzo della memoria condivisa.

## Struttura del Progetto
Il progetto si compone di diversi processi principali:

- **Processo Master**: gestisce la simulazione, inizializza le strutture, crea processi figlio e raccoglie statistiche.
- **Processi Atomo**: si scindono in nuovi processi atomo, generando energia.
- **Processo Attivatore**: decide quando gli atomi devono scindersi.
- **Processo Alimentazione**: aggiunge nuovi atomi alla simulazione.
- **(Versione completa)** **Processo Inibitore**: controlla la reazione assorbendo energia e limitando le scissioni.

## Modalità di Terminazione
La simulazione termina quando si verifica una delle seguenti condizioni:
- **Timeout**: la durata della simulazione supera il limite impostato.
- **Explode**: l'energia liberata supera una soglia prestabilita.
- **Blackout**: l'energia consumata dal master supera quella disponibile.
- **Meltdown**: fallimento della creazione di nuovi processi.

## Configurazione
I parametri di configurazione sono letti da file o variabili di ambiente, senza necessità di ricompilazione.

## Installazione
1. Clonare il repository:
   ```sh
   git clone <repository-url>
   cd <repository-folder>
   ```
2. Compilare il progetto utilizzando `make`:
   ```sh
   make
   ```

## Esecuzione
Eseguire la simulazione con:
```sh
./master
```

## Output
Ogni secondo il sistema stampa:
- Numero di attivazioni del processo attivatore.
- Numero di scissioni e quantità di energia prodotta.
- Quantità di energia consumata e scorie prodotte.
- Energia assorbita e bilanciamento del processo inibitore.


