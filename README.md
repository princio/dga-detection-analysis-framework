# Postgres

La versione è la 17.

## Dopo l'installazione: utente

Impostare la password dell'utente `postgres`:
> sudo -i -u postgres
> ALTER USER postgres PASSWORD 'postgres';

### Creare utente nuovo con `pgadmin`

Installato `pgadmin`, si può creare un nuovo utente per non usare postgres che fa schifo.

# Python

## Pyenv

- Installare pyenv

```
pyenv install 3.12.12           # installa la versione 3.12.12
pyenv local 3.12.12             # attiva localmente la versione 3.12.12
pyenv virtualenv 3.12.12 phd    # crea un virtualenv con la versione 3.12.12 dal nome phd
pyenv activate 3.12.12 phd      # attiva il phd environment
```

## Link simbolico a `psl_list`

Creare nella cartella `suite2\suite2\libs` un collegamento simbolico a `suite\psl_list`.

## Dipendenze

Eseguo prima l'installazione di tensorflow così gli altri si adattano a lui:

> pip install tensorflow==2.13.0
> pip install pandas psycopg2-binary sqlalchemy dependency_injector requests tabulate

## Avvio `scripts`

Dentro ogni script dovrebbe esserci la configurazione:

```python
    application.config.from_dict({
        "env": "debug",
        "db": {
            "host": "localhost",
            "user": "princio",
            "password": "postgres",
            "dbname": "ti2016",
            "port": 5432
        },
        ...
```

```sh
python scripts/[nome-script].py
```
