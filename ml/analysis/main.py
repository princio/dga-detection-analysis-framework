from pathlib import Path
import pandas as pd
from plot import Plot
from defs import FetchConfig, NN, Database
import matplotlib
import matplotlib.pyplot as plt
from matplotlib import rc

matplotlib.use('Qt5Agg')

db = Database()


# print(sql_healthy_dataset(config))

pcaps = pd.read_sql("SELECT * FROM PCAPMW WHERE DGA=2", db.engine)[["pcap_id","pcap_name","qr","q","u","r","duration","dga",'mw_name']]

print(pcaps)

plot = Plot(db, config=FetchConfig(
    sps=3600,
    nn=NN.NONE,
    th=0.5,
    table_from=None,
    pcap_offset=0
))

a = plot.by_fp_r(f"healthy data set", maxslots=8)
plt.show()

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