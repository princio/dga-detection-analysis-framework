from pathlib import Path
import pandas as pd
import os

os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3' 
import tensorflow as tf
from keras.utils import pad_sequences
from tensorflow import convert_to_tensor
from keras.models import model_from_json

max_len = 60

vocabulary = ['', '-', '.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z']

def lstm_univpm_run(s_dn: pd.Series, model_dir: Path):
    path_json = model_dir.joinpath('model.json')
    path_h5 = model_dir.joinpath('model.h5')
    print(path_json)
    print(path_h5)
    with open(path_json, 'r') as fp:
        loaded_model_json = fp.read()
        pass
    model = model_from_json(loaded_model_json)
    model.load_weights(path_h5) # load weights into new model


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


def test(self, dns):
    for nn in self.nns:
        self.load_model(nn)
        if False:
            Xtf = self.layer(tf.strings.bytes_split(tf.strings.substr(dns, 0, max_len)))
            Xtf = pad_sequences(
                Xtf.numpy(), maxlen=max_len, padding='post', dtype="int32",
                truncating='pre', value=vocabulary.index('')
            )
            Xtf = convert_to_tensor(Xtf)
        elif False:
            X = [[ vocabulary.index(c) for c in dn] for dn in dns ]

            Xtf = tf.keras.preprocessing.sequence.pad_sequences(
                X, maxlen=max_len, padding='post', dtype="int32",
                truncating='pre', value=vocabulary.index('')
            )
        else:
            X = [[ vocabulary.index(c) for c in dn] for dn in dns ]

            Xtf = tf.keras.preprocessing.sequence.pad_sequences(
                X, maxlen=max_len, padding='post', dtype="int32",
                truncating='pre', value=vocabulary.index('')
            )
            
            model = tf.keras.models.load_model(oldpaths[nn.extractor])

            Y = model.predict(Xtf, batch_size=128, verbose=1)[:,0]

        print("\n".join([ "%20s\t%10s\t%-100s" % (dns[i], Y[i], Xtf[i]) for i in range(len(X))]))
        pass
    pass