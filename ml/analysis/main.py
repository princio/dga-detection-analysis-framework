from dataclasses import dataclass
from io import BytesIO
import itertools
from pathlib import Path
from typing import Optional, Union, cast
from xml.dom.minidom import parseString
import numpy as np
import pandas as pd
from plot2 import PlotConfig, Plotter, DataConfig
from defs import FetchConfig, NN, Database, OnlyFirsts, PacketType
import matplotlib
import matplotlib.pyplot as plt
from matplotlib import rc
import json
from plots import plot_pcaps, plot_rowsource_colposneg
from slot2 import sql_healthy_dataset

matplotlib.use('Qt5Agg')

db = Database()

pcaps = pd.read_sql("SELECT * FROM PCAPMW WHERE DGA=2 and duration > 8", db.engine)[["pcap_id","pcap_name","qr","q","u","r","duration","dga",'mw_name']]

print(pcaps)

pcaps = pcaps[pcaps['pcap_id'].isin([46, 58, 57, 54])]

cols = [ (None, 'healthy', 0) ] + [ (pcap['pcap_id'], pcap['mw_name'], 0) for _, pcap in pcaps.iterrows() ]

#######################

plotter = Plotter(Path('./tmp'), db)

# plot_pcaps(plotter, pcaps)


fig, axes = plot_rowsource_colposneg(plotter, pcaps, None, maxslots = None, rolled = None)

plt.show()

exit()
#######################

fig, axs = plt.subplots(len(cols), 1, sharey=True)
axs = cast(np.ndarray, axs)
for i, col in enumerate(cols):
    config = FetchConfig(3600, NN.NONE, 0.5, pcap_id=int(col[0]) if col[0] else None, pcap_offset=0)
    
    fpath = f'tmp/{col[1]}_source_{col[0]}_{config.__hash__()}'
    if Path(fpath).exists():
        df = pd.read_csv(fpath)
    else:
        with db.engine.connect() as conn:
            df = pd.read_sql(sql_healthy_dataset(config), conn)
            df.to_csv(fpath)
        pass

    # df8 = df.sort_values(by='slotnum').set_index('slotnum')[['gnx_pos1', 'gok_pos1_dnagg', 'gnx_pos1_dnagg']].rolling(8, min_periods=1).aggregate({
    #     'gnx_pos1': 'sum',
    #     'gok_pos1_dnagg': lambda x: ','.join(x),
    #     'gnx_pos1_dnagg': lambda x: ','.join(x)
    # })

    # print(df8)

    # df8.plot.bar(ax=axs[i])#, logy=True)

    # df8 = df8.fillna(0)[df8 > 15]
    # slot8 = int(df8.idxmin())

    # print(df8.min(), slot8)

    # df = df.iloc[slot8 * 8: (slot8 + 1) * 8]

    if False:
        max = int(df.slotnum.max() - 8)
        df2 = df.set_index('slotnum')
        # s = np.zeros(max, dtype=np.int32)
        bo = []
        for j in range(max):
            upper = j + 8 if j + 8 < max else max
            dftmp = df2.iloc[j:upper]
            sum = dftmp['gnx_pos1'].sum()
            print(f'{j}:{upper}', sum)
            dftmp['gok_pos1_dnagg'] = dftmp['gok_pos1_dnagg'].apply(lambda x: json.loads(x.replace("'", '"')))
            ok_pos_dnagg = [ x for xs in dftmp['gok_pos1_dnagg'].values for x in xs ]
            ok_pos_dnagg_count = len(ok_pos_dnagg)
            dftmp['gnx_pos1_dnagg'] = dftmp['gnx_pos1_dnagg'].apply(lambda x: json.loads(x.replace("'", '"')))
            nx_pos_dnagg = [ x for xs in dftmp['gnx_pos1_dnagg'].values for x in xs ]
            nx_pos_dnagg_count = len(nx_pos_dnagg)
            bo.append([f'{j}:{upper}', sum, ok_pos_dnagg_count, nx_pos_dnagg_count, ok_pos_dnagg, nx_pos_dnagg])

        pd.DataFrame(bo, columns=['rool', 'sum', '#ok_pos_dnagg', '#nx_pos_dnagg', 'ok_pos_dnagg', 'nx_pos_dnagg']).to_csv(f'tmp/source_{col[0]}.rolled.csv')
        pass
    print(i)
    offset = [
        0, 0,
        768, #449,
        0, 33, 0, 0, 31,  379, 0, 31, 0
    ][i]

    df = df.iloc[offset:offset+8].copy()

    by_config(df, col[0], col[1], PacketType.NX, OnlyFirsts.GLOBAL, NN.NONE, axs[i], only_pos=True, maxslots=None)
    axs[i].set_ylabel(f'''
{col[0]}|{col[1]}|{df.index.max()}
''', rotation='horizontal', labelpad=40)
    pass

# fig.tight_layout()
plt.show()

exit()
config = FetchConfig(
    sps=3600,
    nn=NN.NONE,
    th=0.5,
    table_from=None,
    pcap_offset=0
)


cols = [
    (None, 0),
    (54, 770 * 3600),
    (45, 0)
]

fig, axs = plt.subplots(1, len(cols), sharey=True)
axs = cast(np.ndarray, axs)

for i, row in enumerate(cols):
    with db.engine.connect() as conn:
        df = pd.read_sql(sql_healthy_dataset(3600, NN.NONE, 0.5, pcap_id=row[0], pcap_offset=row[1]), conn)

    by_config(df, PacketType.Q, OnlyFirsts.GLOBAL, NN.NONE, axs[i], maxslots=8)
    pass

fig.tight_layout()
plt.show()

txt = f'''
sps=3600
packet type = {PacketType.Q}
nn = {NN.NONE}
th = 0.5
duplicates=keep first
'''
fig.text(.5, -.1, txt, ha='center')
fig.savefig(f"tmp/{fig.__hash__()}", bbox_inches="tight")
buffer = BytesIO()
fig.savefig(buffer, format='svg', bbox_inches="tight")
buffer.seek(0)

exit()
for hourslot in [1, 24]:
    for _,pcap in pcaps.sort_values("pcap_id").iterrows():
        if pcap["pcap_id"] != 54:
            continue

        pcap_offset = 0
        if pcap['pcap_id'] == 54:
            pcap_offset = 770 * 3600

        plot = Plot(db, config=FetchConfig(
            sps=3600 * hourslot,
            nn=NN.NONE,
            th=0.5,
            table_from=pcap['pcap_id'],
            pcap_offset=pcap_offset
        ))


        for maxslots in [None, 8]:
            a = plot.by_fp_r(f"[{pcap['mw_name']}] pcap {pcap['pcap_id']} maxslots={maxslots}", maxslots=maxslots)
            _dir = Path(f'images/both/{f"{maxslots}_slots" if maxslots else "fullrange"}/')
            _dir.mkdir(exist_ok=True, parents=True)
            _dir.joinpath("csv/").mkdir(exist_ok=True, parents=True)
            fp = _dir.joinpath(f'{pcap["pcap_id"]}_{pcap["mw_name"]}')

            a[0].savefig(f'{fp}.{hourslot}.hr.pdf')
            a[1][0].to_csv(_dir.joinpath("csv/").joinpath(f'{fp.name}.{hourslot}hr.global.csv'))
            a[1][1].to_csv(_dir.joinpath("csv/").joinpath(f'{fp.name}.{hourslot}hr.all.csv'))
            plt.show()
            pass
        pass