import pandas as pd
import os, time, re, argparse
import tensorflow as tf
from keras.utils import pad_sequences
from tensorflow import convert_to_tensor
from keras.models import load_model as keras_load_model, model_from_json
from pslregex import PSLdict

gpu_devices = tf.config.experimental.list_physical_devices('GPU')
for device in gpu_devices:
    tf.config.experimental.set_memory_growth(device, True)
    pass


psldict = PSLdict()
psldict.init(download=False, update=False)


def exec_psl(domains):

    domains = [ d.lower() for d in domains ]

    suffixes = [ psldict.match(dn) for dn in domains ]

    tlds = [ None if dn.count('.') == 0 or dn[-1] == '.' else dn[dn.rfind('.')+1:] for dn in domains ]
    icanns = [ s['icann']['suffix'] if s['icann'] is not None else None for s in suffixes if s is not None ]
    privates = [ s['private']['suffix'] if s['private'] is not None else None for s in suffixes if s is not None ]

    # https://stackoverflow.com/a/26987741
    regex = re.compile(r'^(((?!\-))(xn\-\-)?[a-z0-9\-_]{0,61}[a-z0-9]{1,1}\.)*(xn\-\-)?([a-z0-9\-]{1,61}|[a-z0-9\-]{1,30})\.[a-z]{2,}$')

    invalids = [ regex.match(dn) is None for dn in domains ]

    df = pd.Series(domains, name='dn').to_frame()

    df['tld'] = tlds
    df['icann'] = icanns
    df['private'] = privates
    df['invalid'] = invalids

    return df

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

models = [
    ('/home/princio/Repo/princio/malware-detection/refactor/nns/NONE', 'none'),
    ('/home/princio/Repo/princio/malware-detection/refactor/nns/TLD', 'tld'),
    ('/home/princio/Repo/princio/malware-detection/refactor/nns/ICANN', 'icann'),
    ('/home/princio/Repo/princio/malware-detection/refactor/nns/PRIVATE', 'private'),
]

models = []
for model_name in [ 'none', 'tld', 'icann', 'private' ]:
    models.append({
        'psl': model_name,
        'nn': load_model_json(
            f'/home/princio/Repo/princio/malware-detection/refactor/nns/json_tf2.4/model_{model_name}.json',
            f'/home/princio/Repo/princio/malware-detection/refactor/nns/json_tf2.4/model_{model_name}.h5'
        )
    })

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


if not debug:
    domains = pd.read_csv(args.filename, header=None, names=['dn']).dn.tolist()
else:
    domains = pd.read_csv(args.filename, header=None, names=['dn'], nrows=1000).dn.tolist()
    print('Debug mode one (takes only 1000 rows)')



file_psl = os.path.join(args.outdir, f'pslregex.csv')
print('Performing pslregex...', end='')
if not os.path.exists(file_psl):
    df_dn = exec_psl(domains)
    df_dn.to_csv(file_psl)
    print('Done')
    pass
else:
    df_dn = pd.read_csv(file_psl)
    print('Already done')
    pass

df_dn = pd.read_csv(file_psl, index_col=0)

batch_size = args.batch

batches = df_dn.shape[0] // batch_size
batches += 1

print(f"Batch number: {batches}")

times = {
    'lookup': 0.0
}

layer = tf.keras.layers.StringLookup(vocabulary=vocabulary[1:])

for batch in range(batches):

    print(f"Batch {batch+1}/{batches}")

    X_outpath = os.path.join(X_outdir, f'X_{batch}.csv')
    Y_outpath = os.path.join(Y_outdir, f'Y_{batch}.csv')

    if os.path.exists(Y_outpath):
        print(f"already done.")
        continue

    if not os.path.exists(X_outpath):
        df_batch = df_dn.iloc[batch * batch_size:(batch + 1)*batch_size].copy()
        
        df_batch['_tld'] = df_batch[['tld']].fillna('@@@').apply(lambda row: '' if row['tld'] == '@@@' else row['tld'], axis=1)
        df_batch['_icann'] = df_batch[['_tld', 'icann']].fillna('@@@').apply(lambda row: row['_tld'] if row['icann'] == '@@@' else row['icann'], axis=1)
        df_batch['_private'] = df_batch[['_icann', 'private']].fillna('@@@').apply(lambda row: row['_icann'] if row['private'] == '@@@' else row['private'], axis=1)

        df_batch['X_none'] = df_batch['dn']
        df_batch['X_tld'] = df_batch[['dn', '_tld']].apply(lambda row: row['dn'].replace('.' + row['_tld'], ''), axis=1)
        df_batch['X_icann'] = df_batch[['dn', '_icann']].apply(lambda row: row['dn'].replace('.' + row['_icann'], ''), axis=1)
        df_batch['X_private'] = df_batch[['dn', '_private']].apply(lambda row: row['dn'].replace('.' + row['_private'], ''), axis=1)

        df_batch.to_csv(X_outpath)
        print(f"X-ing done")
        pass
    else:
        df_batch = pd.read_csv(X_outpath, index_col=0)
        print(f"X-ing already done.")
        pass

    # df_batch = df_batch.iloc[0:1000]
    
    df_batch['X_none'] = df_batch['dn']

    for model in models:
        b = time.time()
        X = df_batch[f'X_{model["psl"]}'].astype(str).apply(lambda x: '.'.join(x.split('.')[::-1]))
        Xtf = layer(tf.strings.bytes_split(tf.strings.substr(X, 0, max_len)))
        Xtf = pad_sequences(
            Xtf.numpy(), maxlen=max_len, padding='post', dtype="int32",
            truncating='pre', value=vocabulary.index('')
        )
        Xtf = convert_to_tensor(Xtf)
        times['lookup'] += time.time() - b
        df_batch[f'Y_{model["psl"]}'] = model['nn'].predict(Xtf, batch_size=128, verbose=1)[:,0]
        pass

    df_batch[[
        'dn', 'invalid',
        'tld', 'icann', 'private',
        'Y_none', 'Y_tld', 'Y_icann', 'Y_private'
        ]].to_csv(Y_outpath)

    pass

exit(1)

layer = tf.keras.layers.StringLookup(vocabulary=vocabulary[1:])

print(f"Batch number: {batches}")

for batch in range(batches):
    if os.path.exists(f'./out/Y_{batch}.csv'):
        continue

    print(f"Batch {batch+1}/{batches}")

    for pslremove in ['none', 'tld', 'icann', 'private']:
        b = time.time()
        X = df_dn[f'X_{pslremove}'].apply(lambda x: '.'.join(x.split('.')[::-1]))
        Xtf = layer(tf.strings.bytes_split(tf.strings.substr(X, 0, max_len)))
        Xtf = pad_sequences(
            Xtf.numpy(), maxlen=max_len, padding='post', dtype="int32",
            truncating='pre', value=vocabulary.index('')
        )
        Xtf = convert_to_tensor(Xtf)
        times['lookup'] += time.time() - b

        b = time.time()
        X = df_dn[f'X_{pslremove}'].apply(lambda x: '.'.join(x.split('.')[::-1]))
        Xvoc = X.apply(lambda x: [ vocabulary.index(c) if c in vocabulary else 0 for c in x[:max_len] ])
        Xvoc = pad_sequences(
            Xvoc.values.tolist(), maxlen=max_len, padding='post', dtype="int32",
            truncating='pre', value=vocabulary.index('')
        )
        Xvoc = convert_to_tensor(Xvoc)
        times['voc'] += time.time() - b

        print(tf.experimental.numpy.all(Xtf == Xvoc))
        print()
        pass

print(times['lookup'] / 4)