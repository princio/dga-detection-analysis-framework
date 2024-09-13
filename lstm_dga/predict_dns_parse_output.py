from pathlib import Path
import pandas as pd
import os, time, re, argparse

parser = argparse.ArgumentParser(
                    prog='Predict file',
                    description='The input file should be a list of [new-line] separated domain. It will perform the prediction based on 4 nn, each of them will take a different portion of the domain.',
                    epilog='Text at the bottom of help')

parser.add_argument('nndir')
parser.add_argument('outputdir')
parser.add_argument('inputpath')
parser.add_argument('-d', '--debug', action='store_true')      # option that takes a value
parser.add_argument('-b', '--batch', default=500_000, help='The batch size, zero means no batching.')      # option that takes a value

args = parser.parse_args()

print(args)

batch_size = args.batch
debug = args.debug
nndir = Path(args.nndir)
outputdir = Path(args.outputdir)
inputpath = Path(args.inputpath)
batchdir = Path(f"/tmp/lstm/Y_{inputpath.name}/")

outputpath = outputdir.joinpath(f"{inputpath.stem}.lstm.csv")

if not os.path.exists(outputdir):
    os.makedirs(outputdir, exist_ok=True)

if not os.path.exists(batchdir):
    os.makedirs(batchdir, exist_ok=True)

import tensorflow as tf
from keras.utils import pad_sequences
from tensorflow import convert_to_tensor
from keras.models import load_model as keras_load_model, model_from_json

gpu_devices = tf.config.experimental.list_physical_devices('GPU')
for device in gpu_devices:
    tf.config.experimental.set_memory_growth(device, True)
    pass

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

models = {}
for model_name in [ 'none', 'tld', 'icann', 'private' ]:
    models[model_name] = {
        'name': model_name,
        'nn': load_model_json(
            os.path.join(nndir, f'json_tf2.4/model_{model_name}.json'),
            os.path.join(nndir, f'json_tf2.4/model_{model_name}.h5')
        )
    }
    pass


##############

df = pd.read_csv(inputpath, index_col=False)

df_unique = df.drop_duplicates(subset=["dn"])

X = df_unique[['dn', 'dn_tld', 'dn_icann', 'dn_private']]

X = X.rename(columns={ 'dn': 'dn_none' })

batch_size = 1000 if debug else batch_size

##############

batches = X.shape[0] // batch_size
batches += 1

print(f"Batch number: {batches}")

times = { 'lookup': 0.0 }

layer = tf.keras.layers.StringLookup(vocabulary=vocabulary[1:])

for batch in range(batches):

    print(f"Batch {batch+1}/{batches}")

    Y_batch_path = os.path.join(batchdir, f'batch_{batch}.csv')

    if os.path.exists(Y_batch_path):
        print(f"already done.")
        continue

    df_batch = X.iloc[batch * batch_size:(batch + 1) * batch_size].copy()

    for col in df_batch.columns:

        model_name = col[(col.find("_") + 1) if col.find("_") >= 0 else len(col):]

        print(model_name)

        b = time.time()
        X2 = df_batch[col].astype(str).apply(lambda x: '.'.join(x.split('.')[::-1]))
        Xtf = layer(tf.strings.bytes_split(tf.strings.substr(X2, 0, max_len)))
        Xtf = pad_sequences(
            Xtf.numpy(), maxlen=max_len, padding='post', dtype="int32",
            truncating='pre', value=vocabulary.index('')
        )
        Xtf = convert_to_tensor(Xtf)
        times['lookup'] += time.time() - b
        df_batch[f'Y_{model_name}'] = models[model_name]['nn'].predict(Xtf, batch_size=128, verbose=1)[:,0]
        pass
    
    df_batch = df_batch[['dn_none', 'Y_none', 'Y_tld', 'Y_icann', 'Y_private']]

    df_batch.to_csv(Y_batch_path, index=False)

    if debug: break

    pass


Y_batch_path = os.path.join(batchdir, f'batch_0.csv')
df_batch = pd.read_csv(Y_batch_path, index_col=False)

for batch in range(1, batches):
    Y_batch_path = os.path.join(batchdir, f'Y_{batch}.csv')

    df_batch_current = pd.read_csv(Y_batch_path, index_col=False)

    df_batch = pd.concat([df_batch, df_batch_current], axis=0)
    pass

df.merge(df_batch, left_on='dn', right_on='dn_none').drop(columns="dn_none").to_csv(outputpath, index=False)
