# Malware detection

- **ApplyConfiguration**, a data interface used to save the following parameters:
    - `name`: str
    - `model_id`: Model
    - `wtype`: str
    - `top10m`: Tuple[bool, int]
    - `wsize`: int
    - `windowing`: str
    - `inf`: Tuple[int, int]

- **PCAP**, a class that fetch a PCAP and can generate PCAPApplied.

- **PCAPApplied**, a class that given a PCAP, *represents* an Apply with a specific ApplyConfiguration applied to it.

- **Apply**, a class that given a PCAP, *represents* an Apply with a specific ApplyConfiguration applied to it.

##

Fare un analisi main fold dove prendo il 90% dei 

Problema dei falsi positivi.

Finestratura parametro piu' importante.

Altri parametri possono essere omessi dall'analisi.

Gli altri parametri sono:
- wtype llr o nx
- top10m
- windowing: request, both o response.

=> applicare l'apply senza postgre

pcap -> finestratura -> lascio fuori n finestre infette e non

per ogni finestratura:
- applico metodo di predizione

pcap -> finestratura -> applico metodi di predizione -> risultati

bisogna fare un analisi non tanto confusion matrix, ma numero di allarmi sollevati.

Una finestra appartenente ad un pcap infetto può essere:
- non-infetta
- **potenzialmente** infetta
- l'unica verità certa è quella per cui i False Positive sono nulli: in quel caso la rete neurale sta funzionando al top, almeno per i falsi positivi.
- per cui dato che non tutte le finestre potrebbero presentare delle infezioni, più aumento la grandezza della finestra, più rischio che mi perdo qualcosa.
- la soglia per cui il numero di falsi positivi è minimo è la massima soglia per cui ho `FP=0`
> `th in [wvalue_min, wvalue_max], th  tale che FP=0`

Tra i parametri non c'è WSIZE perché è un parametro che non influenza la detection allo stesso modo degli altri.


## Finestratura ovvero shuffle


CREATE TABLE MESSAGES__NN_7 AS
SELECT LOGIT,
       M.FN_REQ,
       M.IS_RESPONSE
  FROM MESSAGE_MASTER AS M
  LEFT JOIN
        (SELECT DN.ID,
               DN.DN,
               DN.TOP10M,
               LOGIT
          FROM DN
          LEFT JOIN DN_NN
            ON (DN.ID = DN_NN.DN_ID
                         AND NN_ID = 7)
       ) AS DNN
    ON (M.DN_ID = DNN.ID)
 ORDER BY PCAP_ID, FN_REQ


# Predict

The file `predict.py` is the one where there is the pre processing.