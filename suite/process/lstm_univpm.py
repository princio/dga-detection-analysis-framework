from pathlib import Path
import pandas as pd
import os

os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3' 
import tensorflow as tf # type: ignore
from keras.utils import pad_sequences # type: ignore
from tensorflow import convert_to_tensor # type: ignore
from keras.models import model_from_json # type: ignore

max_len = 60

vocabulary = ['', '-', '.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z']

def lstm_univpm_run(s_dn: pd.Series, model_json: str, model_hf5: Path):
    model = model_from_json(model_json)
    model.load_weights(model_hf5)

    codes, uniques = s_dn.factorize()

    dn_reversed = [ ".".join(dn.split('.')[::-1]) for dn in uniques ]

    layer = tf.keras.layers.StringLookup(vocabulary=vocabulary[1:])

    Xtf = layer(tf.strings.bytes_split(tf.strings.substr(dn_reversed, 0, max_len)))
    Xtf = pad_sequences(
        Xtf.numpy(), maxlen=max_len, padding='post', dtype="int32",
        truncating='pre', value=vocabulary.index('')
    )
    Xtf = convert_to_tensor(Xtf)

    Y = model.predict(Xtf, batch_size=128, verbose=1)[:,0]

    dn_reversed = pd.Series(dn_reversed).take(codes).reset_index(drop=True) # type: ignore
    Y = pd.Series(Y).take(codes).reset_index(drop=True) # type: ignore

    return dn_reversed, Y
