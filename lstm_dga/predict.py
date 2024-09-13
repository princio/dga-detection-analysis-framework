import pandas as pd
import os, time, re, argparse

parser = argparse.ArgumentParser(
                    prog='Predict file',
                    description='The input file should be a list of [new-line] separated domain. It will perform the prediction based on 4 nn, each of them will take a different portion of the domain.',
                    epilog='Text at the bottom of help')

parser.add_argument('outdir')
parser.add_argument('filename')
parser.add_argument('-d', '--debug', action='store_true')      # option that takes a value
parser.add_argument('-b', '--batch', default=500_000, help='The batch size, zero means no batching.')      # option that takes a value

args = parser.parse_args()

print(args)
debug = args.debug
outdir = args.outdir
X_outdir = os.path.join(args.outdir, 'X')
Y_outdir = os.path.join(args.outdir, 'Y')

if not os.path.exists(args.outdir):
    os.mkdir(outdir)

if not os.path.exists(X_outdir):
    os.mkdir(X_outdir)

if not os.path.exists(Y_outdir):
    os.mkdir(Y_outdir)

df_X = pd.read_csv(args.filename, index_col=None)


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
            f'./nns/json_tf2.4/model_{model_name}.json',
            f'./nns/json_tf2.4/model_{model_name}.h5'
        )
    }


batch_size = 1000 if debug else args.batch

batches = df_X.shape[0] // batch_size
batches += 1

print(f"Batch number: {batches}")

times = {
    'lookup': 0.0
}

layer = tf.keras.layers.StringLookup(vocabulary=vocabulary[1:])

for batch in range(batches):

    print(f"Batch {batch+1}/{batches}")

    Y_outpath = os.path.join(Y_outdir, f'Y_{batch}.csv')

    if os.path.exists(Y_outpath):
        print(f"already done.")
        continue

    df_batch = df_X.iloc[batch * batch_size:(batch + 1) * batch_size].copy()

    batch_columns = ['id', 'is_malware', 'is_dga' ]
    for col in df_batch.columns:
        if col in ['id', 'is_malware', 'is_dga']: continue

        model_name = col[0:col.find("_") if col.find("_") >= 0 else len(col)]

        print(model_name)

        b = time.time()
        X = df_batch[col].astype(str).apply(lambda x: '.'.join(x.split('.')[::-1]))
        Xtf = layer(tf.strings.bytes_split(tf.strings.substr(X, 0, max_len)))
        Xtf = pad_sequences(
            Xtf.numpy(), maxlen=max_len, padding='post', dtype="int32",
            truncating='pre', value=vocabulary.index('')
        )
        Xtf = convert_to_tensor(Xtf)
        times['lookup'] += time.time() - b
        df_batch[f'Y_{col}'] = models[model_name]['nn'].predict(Xtf, batch_size=128, verbose=1)[:,0]
        batch_columns.append(f'Y_{col}')
        pass
    

    df_batch[batch_columns].to_csv(Y_outpath, index=None)

    if debug: break

    pass

exit(1)