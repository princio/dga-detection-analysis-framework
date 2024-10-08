import json
import logging
from pathlib import Path
from typing import Optional, Union
import pandas as pd
import os

os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3' 
import tensorflow as tf # type: ignore
from keras.utils import pad_sequences # type: ignore
from tensorflow import convert_to_tensor # type: ignore
from keras.models import model_from_json # type: ignore


class LSTMService:
    def __init__(self):
        self.max_len = 60
        self.vocabulary = ['', '-', '.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z']
        self.layer = tf.keras.layers.StringLookup(vocabulary=self.vocabulary[1:])
        pass

    def remove_suffix(self, s_dn: pd.Series, suffix: Optional[pd.Series]):
        if suffix is not None:
            suffix_lens = suffix.fillna('').str.len().to_list()
            s_X = pd.Series([ dn[:len(dn) - 1 - suffix_lens[idx]]  for idx, dn in enumerate(s_dn.to_list()) ])
            pass
        else:
            s_X = s_dn.copy()
        return s_X

    def load_model(self, model_json: str, model_hf5: Path):
        if isinstance(model_json, dict):
            model_json = json.dumps(model_json)
        model = model_from_json(model_json)
        model.load_weights(model_hf5)
        return model

    def run(self, model, dn: pd.Series, suffix: Optional[pd.Series]):
        s_X = self.remove_suffix(dn, suffix)

        codes, uniques = s_X.factorize()

        dn_reversed = [ ".".join(dn.split('.')[::-1]) for dn in uniques ]

        Xtf = self.layer(tf.strings.bytes_split(tf.strings.substr(dn_reversed, 0, self.max_len)))
        Xtf = pad_sequences(
            Xtf.numpy(), maxlen=self.max_len, padding='post', dtype="int32",
            truncating='pre', value=self.vocabulary.index('')
        )
        Xtf = convert_to_tensor(Xtf)

        Y = model.predict(Xtf, batch_size=128, verbose=1)[:,0]

        dn_reversed = pd.Series(dn_reversed).take(codes).reset_index(drop=True) # type: ignore
        Y = pd.Series(Y).take(codes).reset_index(drop=True) # type: ignore

        return dn_reversed, Y
