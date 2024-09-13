

import enum
from io import BytesIO
from typing import Optional, cast
import matplotlib
import matplotlib.pyplot as plt
import pandas as pd
from defs import MWTYPE, NN, Database, SlotConfig
from slot import build_sql
import matplotlib.colors as mpcolors
from sqlalchemy import create_engine, text
import quantiphy as qq

matplotlib.use('agg')

class SlotField(enum.StrEnum):
    PACKET = "dn"
    PCAP = "pcap"

class Plot:
    def __init__(self, database: Database, config):
        self.db = database
        self.config = config
        self.df = None
        pass

    def load(self, slotnum: Optional[int] = None, mwtype: Optional[MWTYPE] = None, bydga:bool=False):
        sql = build_sql(self.config, slotnum, mwtype, bydga)
        with self.db.engine.connect() as conn:
            self.df = pd.read_sql(sql, conn)
        pass

    def bydga(self, field: SlotField = SlotField.PACKET):
        if self.df is None:
            self.load(bydga=True)
        
        df = cast(pd.DataFrame, self.df).copy()

        labelpos = f"pos_{self.config.nn}"

        if "dga" not in df.columns:
            df["dga"] = 1

        dgas = df["dga"].drop_duplicates().values

        df = df.groupby(["dga", "slotnum"]).agg({
            f"total": "sum",
            f"{labelpos}": "sum"
        }).unstack(0).fillna(0)#.reset_index().copy()
        
        for dga in dgas:
            df["neg", dga] = df["total", dga] - df[labelpos, dga]
        
        fig, axs = plt.subplots(figsize=(10, 6), tight_layout=True)
        
        W = 1
        ticks_label = []
        ticks_pos = []
        for slotnum, row in df.iterrows():
            slotnum = cast(int, slotnum)
            if slotnum >= self.config.range[0] and slotnum <= self.config.range[1]:
                ticks_label.append(slotnum)
                ticks_pos.append(slotnum * 4*W)
                for dga in dgas:
                    # if True and dga == 1:
                    #     continue
                    neg, pos = row[("neg", dga)], row[(labelpos, dga)]
                    neg = cast(int, neg)
                    pos = cast(int, pos)
                    rects = axs.bar(slotnum * 4*W + [-W, 0, W][dga], neg, color=mpcolors.to_rgba(["blue","orange","red"][dga], 0.2))
                    rects = axs.bar(slotnum * 4*W + [-W, 0, W][dga], pos, bottom=neg, color=mpcolors.to_rgba(["blue","orange","red"][dga], 1))
                    if (pos+neg) > 0:
                        axs.bar_label(rects, labels=[qq.Quantity(int(pos)).render(prec=1)], padding=10)
                    axs.bar_label(rects, labels=[qq.Quantity(int(pos+neg)).render(prec=1)], padding=0)
            pass
        
        axs.set_xticks(ticks_pos, ticks_label)

        txt = str(self.config)
        txt += f"\nslotnum max: {df.index.max()}"
        fig.text(.5, -.1, txt, ha='center')
        # fig.savefig(f"tmp/{fig.__hash__()}", bbox_inches="tight")
        
        buffer = BytesIO()
        fig.savefig(buffer, format='svg', bbox_inches="tight")
        buffer.seek(0)

        # Get the SVG image as a string
        svg_image = buffer.getvalue().decode('utf-8')
        
        with open('tmp/svg.txt', 'w') as fp:
            fp.write(svg_image)

        return svg_image, df
    
    def bypcap(self, field: SlotField = SlotField.PACKET):
        if self.df is None:
            self.load()
        
        df = cast(pd.DataFrame, self.df).copy()

        labelpos = f"pos_{self.config.nn}"
        labelneg = f"neg"


        df = df.groupby(["pcap_id", "dga", "slotnum"]).agg({
            f"total": "sum",
            f"{labelpos}": "sum"
        }).reset_index()#.unstack(0).fillna(0)#.reset_index().copy()

        df[labelneg] = df["total"] - df[labelpos]

        # print(df.reset_index())
        
        fig, axs = plt.subplots(figsize=(10, 6), tight_layout=True)

        df = df[df.dga == 2]
        
        W = 1
        ticks_label = []
        ticks_pos = []
        slotnums = df.slotnum.drop_duplicates().values
        for slotnum in slotnums:
            slotnum = cast(int, slotnum)
            if slotnum >= self.config.range[0] and slotnum <= self.config.range[1]:
                df_slotnum = df[df.slotnum == slotnum]
                ticks_label.append(slotnum)
                ticks_pos.append(slotnum * 4*W)

                bottom = 0
                for index, row in df_slotnum.iterrows():
                    print(index, "\n", row, "\n\n", sep="")

                    neg, pos = row[labelneg], row[labelpos]
                    neg, pos = cast(int, neg), cast(int, pos)
                    total = neg + pos

                    if total > 0:
                        rects = axs.bar(slotnum * 4*W, total, bottom=bottom)
                        bottom += total
                        axs.bar_label(rects, labels=[row["pcap_id"]], padding=0, label_type='center')
                        pass
                    pass
            pass
        
        axs.set_xticks(ticks_pos, ticks_label)

        txt = str(self.config)
        txt += f"\nslotnum max: {df[df.total > 0].slotnum.max()}"
        fig.text(.5, -.1, txt, ha='center')
        # fig.savefig(f"tmp/{fig.__hash__()}", bbox_inches="tight")
        
        buffer = BytesIO()
        fig.savefig(buffer, format='svg', bbox_inches="tight")
        buffer.seek(0)

        # Get the SVG image as a string
        svg_image = buffer.getvalue().decode('utf-8')
        
        with open('tmp/svg.txt', 'w') as fp:
            fp.write(svg_image)

        return svg_image, df