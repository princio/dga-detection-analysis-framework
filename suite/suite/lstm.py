from dataclasses import dataclass
import enum
from pathlib import Path
from typing import Any, List
import numpy as np
import pandas as pd
import os


os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3' 
import tensorflow as tf
from keras.utils import pad_sequences
from tensorflow import convert_to_tensor
from keras.models import load_model as keras_load_model, model_from_json

from config import Config


def load_model_json(path_json, path_h5):
    json_file = open(path_json, 'r')
    loaded_model_json = json_file.read()
    json_file.close()
    loaded_model = model_from_json(loaded_model_json)
    # load weights into new model
    loaded_model.load_weights(path_h5)
    print("Loaded model from disk")
    return loaded_model

max_len = 60

vocabulary = ['', '-', '.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z']

class Extractor(enum.Enum):
    NONE = 0
    TLD = 1
    ICANN = 2
    PRIVATE = 3
    pass

@dataclass
class NN:
    id: int
    name: str
    extractor: Extractor
    dir: Path
    model: Any = None
    pass




class LSTM:
    def __init__(self, config: Config) -> None:
        self.layer = tf.keras.layers.StringLookup(vocabulary=vocabulary[1:])

        self.config = config
        self.batch_size = config.lstm_batch_size

        self.df_nns: pd.DataFrame = pd.read_sql("SELECT * FROM nn", config.sqlalchemyconnection)

        self.nns: List[NN] = []
        for _, row in self.df_nns.iterrows():
            self.nns.append(NN(row["id"], row["name"], Extractor[row["extractor"].upper()], Path(row["directory"])))
            pass
        pass


    def count(self, nn: NN):
        cursor = self.config.psyconn.cursor()
        
        cursor.execute("""SELECT COUNT(DN.ID) FROM DN;""")

        dn_count = cursor.fetchone()[0]

        cursor.execute("""SELECT COUNT(DN_NN.DN_ID) FROM DN_NN WHERE DN_NN.NN_ID=%s;""", (nn.id,))

        dn_nn_count = cursor.fetchone()[0]
        
        return dn_count - dn_nn_count

    def load_model(self, nn: NN):
        nn.model = load_model_json(
            nn.dir.joinpath("model.json"),
            nn.dir.joinpath("model.h5")
        )
        pass

    def run(self):
        for nn in self.nns:
            if self.count(nn) == 0:
                print("DN completed for %s" % nn.name)
                continue

            self.load_model(nn)

            cursor = self.config.psyconn.cursor()
            cursor.execute("""SELECT DN.ID, DN, TLD, ICANN, PRIVATE, INVALID, NN_ID FROM DN LEFT JOIN DN_NN ON (DN.ID = DN_NN.DN_ID AND DN_NN.NN_ID = %s)  WHERE NN_ID is null""", (nn.id,))

            while True:
                rows = cursor.fetchmany(self.batch_size)
                if not rows:
                    break

                df = pd.DataFrame.from_records(rows, columns=["id", "dn", "tld", "icann", "private", "invalid", "nn_id"])

                df["icann"].fillna(value=df["tld"], inplace=True)
                df["private"].fillna(value=df["icann"], inplace=True)

                self.run_lstm(df, nn)
                pass # while true
            pass # for nns
        pass # def

    def run_lstm(self, df: pd.DataFrame, nn: NN):
        df["reversed"] = [ ".".join(row["dn"].split('.')[::-1]) for _, row in df.iterrows() ]
        if nn.extractor == Extractor.NONE:
            dn_extracted_reversed = [ row["reversed"] for _, row in df.iterrows() ]
        else:
            df["suffix"] = df[nn.extractor.name.lower()].fillna("")
            dn_extracted_reversed = [ row["reversed"][len(row["suffix"]) if len(row["suffix"]) > 0 else 0:]  for _, row in df.iterrows() ]
            pass

        Xtf = self.layer(tf.strings.bytes_split(tf.strings.substr(dn_extracted_reversed, 0, max_len)))
        Xtf = pad_sequences(
            Xtf.numpy(), maxlen=max_len, padding='post', dtype="int32",
            truncating='pre', value=vocabulary.index('')
        )
        Xtf = convert_to_tensor(Xtf)

        Y = nn.model.predict(Xtf, batch_size=128, verbose=1)[:,0]

        df["Y"] = Y
        df["logit"] = np.log(Y / (1 - Y))
        
        cursor = self.config.psyconn.cursor()

        args_str = b",".join([cursor.mogrify("(%s, %s, %s, %s)", (row["id"], nn.id, row["Y"], row["logit"])) for _, row in df.iterrows()])

        cursor.execute(
            b"""
            INSERT INTO public.dn_nn(
                dn_id, nn_id, value, logit)
                VALUES """ +
                args_str
        )

        cursor.connection.commit()

        pass
    
    pass