

import enum
from io import BytesIO
from shutil import which
from typing import Any, Optional, Tuple, Union, cast
from xml.etree.ElementTree import QName
import matplotlib
from matplotlib.axes import Axes
import matplotlib.pyplot as plt
import pandas as pd
from pyparsing import col
from defs import MWTYPE, NN, Database, FetchConfig, OnlyFirsts, PacketType
import matplotlib.colors as mpcolors
from sqlalchemy import create_engine, text
import quantiphy as qq
from dataclasses import dataclass, replace

from slot2 import sql_healthy_dataset

matplotlib.use('agg')

class SlotField(enum.StrEnum):
    PACKET = "dn"
    PCAP = "pcap"

class Plot:
    def __init__(self, database: Database, config: FetchConfig):
        self.db = database
        self.config: FetchConfig = config
        self.df = None
        pass

    def load(self, slotnum: Optional[int] = None, mwtype: Optional[MWTYPE] = None, bydga:bool=False):
        sql = build_sql(self.config, slotnum, mwtype, bydga)
        with self.db.engine.connect() as conn:
            self.df = pd.read_sql(sql, conn)
        pass

    def bydga(self, field: SlotField = SlotField.PACKET, _range: Optional[Tuple[int, int]] = None):
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
            if _range is None or (slotnum >= _range[0] and slotnum <= _range[1]):
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

        return (svg_image, df, fig)
    
    def bypcap(self, field: SlotField = SlotField.PACKET, _range: Optional[Tuple[int, int]] = None):
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
            if _range is None or (slotnum >= _range[0] and slotnum <= _range[1]):
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

    def byfp(self, _range: Optional[Tuple[int, int]] = None):
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
            if _range is None or (slotnum >= _range[0] and slotnum <= _range[1]):
                ticks_label.append(slotnum)
                ticks_pos.append(slotnum * 4*W)
                for dga in dgas:
                    if True and dga != 0:
                        continue
                    neg, pos = row[("neg", dga)], row[(labelpos, dga)]
                    neg = cast(int, neg)
                    pos = cast(int, pos)
                    rects = axs.bar(slotnum * 4*W + [0, 0, W][dga], neg, color=mpcolors.to_rgba(["blue","orange","red"][dga], 0.2))
                    rects = axs.bar(slotnum * 4*W + [0, 0, W][dga], pos, bottom=neg, color=mpcolors.to_rgba(["blue","orange","red"][dga], 1))
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

        return (svg_image, df, fig)

    def bytp(self, field: SlotField = SlotField.PACKET):
        if self.df is None:
            self.load(bydga=True)
        
        df = cast(pd.DataFrame, self.df).copy()

        labelpos = f"pos_{self.config.nn}"

        if "dga" not in df.columns:
            df["dga"] = 2

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
        
        for slotnum in range(int(df.index.max())):
            ticks_label.append(slotnum + 1)
            ticks_pos.append(slotnum * 4*W)
            try:
                row = df.loc[slotnum]
                neg, pos = row[("neg", 2)], row[(labelpos, 2)]
                neg = cast(int, neg)
                pos = cast(int, pos)
            except:
                neg = 0
                pos = 0
                pass
            rects = axs.bar(slotnum * 4*W, neg, color=mpcolors.to_rgba(["blue","orange","red"][2], 0.2))
            rects = axs.bar(slotnum * 4*W, pos, bottom=neg, color=mpcolors.to_rgba(["blue","orange","red"][2], 1))
            if (pos+neg) > 0:
                axs.bar_label(rects, labels=[qq.Quantity(int(pos)).render(prec=1)], padding=10)
            axs.bar_label(rects, labels=[qq.Quantity(int(pos+neg)).render(prec=1)], padding=0)
            pass
        
        axs.set_xticks(ticks_pos, ticks_label)
        axs.set_ylim(0,5000)

        txt = str(self.config)
        txt += f"\nslotnum max: {df.index.max()}"
        fig.text(.5, -.1, txt, ha='center')
        # fig.savefig(f"tmp/{fig.__hash__()}", bbox_inches="tight")
        
        buffer = BytesIO()
        fig.savefig(buffer, format='svg', bbox_inches="tight")
        axs.set_title(f"pcap {self.config.table_from}")
        buffer.seek(0)

        # Get the SVG image as a string
        svg_image = buffer.getvalue().decode('utf-8')
        
        with open('tmp/svg.txt', 'w') as fp:
            fp.write(svg_image)

        return (svg_image, df, fig)

    def by_fp_r(self, title='', maxslots: Optional[int] = None):
        dfs = []
        fig, axss = plt.subplots(1, 3, figsize=(12, 4), sharey=False, tight_layout=True)

        with self.db.engine.connect() as conn:
            df = pd.read_sql(sql_healthy_dataset(self.config), conn)

        upper_slot = df.slotnum.max()

        for ofenu, of in enumerate([OnlyFirsts.GLOBAL, OnlyFirsts.ALL]):
            axs = axss[ofenu]

            gg = 'g' if of == OnlyFirsts.GLOBAL else ''

            ls = {}
            for oknx in ['ok','nx']:
                ls[oknx] = {
                    'tot': f"{gg}{oknx}",
                    'pos': f"{gg}{oknx}_pos{self.config.nn}",
                    'neg': f"{gg}{oknx}_neg{self.config.nn}",
                    'pos_dnagg': f"g{oknx}_neg{self.config.nn}_dnagg",
                    'neg_dnagg': f"g{oknx}_neg{self.config.nn}_dnagg",
                }
                pass

            df[f'{ls['nx']['neg_dnagg']}_count'] = df[ls['nx']['neg_dnagg']].apply(lambda x: len(set(x)))

            for oknx in ['ok','nx']:
                df[ls[oknx]['neg']] = df[ls[oknx]['tot']] - df[ls[oknx]['pos']]
            
            # fig, axs = plt.subplots(figsize=(10, 6), tight_layout=True)
            
            W = 1
            ticks_label = []
            ticks_pos = []


            is_verbose = (maxslots is None) and upper_slot > 20
            # if _range and self.config.sps == 3600 and f'{self.config.pcap_id}' == '54':
            #     df = df.loc[770:780]
            ymax = 0
            i = 0
            fullW = W * (2 if is_verbose else 4)
            for slotnum, row in df.iterrows():
                slotnum = cast(int, slotnum)
                i += 1
                if maxslots and i > maxslots: break

                text_toprint = not is_verbose or i % 5 == 0

                if text_toprint:
                    ticks_label.append(slotnum)
                    ticks_pos.append(slotnum * fullW)
                    pass

                for enu, oknx in enumerate(['ok', 'nx']):
                    x = slotnum * fullW + [-W/2, W/2][enu]
                    try:
                        neg = cast(int, row[ls[oknx]['neg']])
                        pos = cast(int, row[ls[oknx]['pos']])
                        ymax = (neg+pos) if ymax < (neg+pos) else ymax
                    except:
                        neg = 0
                        pos = 0
                        pass
                    rects = axs.bar(x, neg, width=W, color=mpcolors.to_rgba("blue", [1, 0.2][enu]))
                    rects = axs.bar(x, pos, width=W, bottom=neg, color=mpcolors.to_rgba("red", [1, 0.2][enu]))
                    if text_toprint:
                        if (pos+neg) > 0:
                            axs.bar_label(rects, labels=[pos], padding=10)
                        axs.bar_label(rects, labels=[neg], padding=0)
                        pass
                    pass

                pass
            
            axs.set_xticks(ticks_pos, ticks_label)
            # axs.set_ylim(0,5000)
            axs.set_ylim(0,ymax + ymax*0.10)

            txt = str(self.config)
            txt += f"\nslotnum max: {df.index.max()}"
            fig.text(.5, -.1, txt, ha='center')
            # fig.savefig(f"tmp/{fig.__hash__()}", bbox_inches="tight")
            axs.set_xlabel(str(self.config) + '\n' + 'slotnum max: ' + str(df.index.max()))
            buffer = BytesIO()
            fig.savefig(buffer, format='svg', bbox_inches="tight")
            axs.set_title(gg + ' ' + title)
            buffer.seek(0)

            # Get the SVG image as a string
            svg_image = buffer.getvalue().decode('utf-8')
            
            with open('tmp/svg.txt', 'w') as fp:
                fp.write(svg_image)

            dfs.append((df))
            pass
        return fig, dfs
    

    def by_r(self, title: str = '', maxslots: Optional[int] = None):
        dfs = []
        fig, axss = plt.subplots(1, 2, figsize=(12, 4), sharey=False, tight_layout=True)

        for ofenu, of in enumerate([OnlyFirsts.GLOBAL, OnlyFirsts.ALL]):
            axs = axss[ofenu]
            self.config = replace(self.config, onlyfirsts=of)
            print(maxslots, self.config)
            self.df = None
            self.load(bydga=True)

            df = cast(pd.DataFrame, self.df).copy()
                # df.index = df.index - 770
            labelpos = {
                'ok': f"r_ok_pos_{self.config.nn}",
                'nx': f"r_nx_pos_{self.config.nn}"
            }

            if "dga" not in df.columns:
                df["dga"] = 2

            dgas = df["dga"].drop_duplicates().values

            df = df.groupby(["dga", "slotnum"]).agg({
                f"r_ok": "sum",
                f"r_nx": "sum",
                f"{labelpos['ok']}": 'sum',
                f"{labelpos['nx']}": 'sum',
                f"r_ok_pos_{self.config.nn}_dnagg": lambda x: x,
                f"r_ok_neg_{self.config.nn}_dnagg": lambda x: x
            }).unstack(0).fillna(0)#.reset_index().copy()


            
            df[('r_ok_pos_1_dnagg_tot', 2)] = df[('r_ok_pos_1_dnagg', 2)].apply(lambda x: len(set(x)))

            for dga in dgas:
                for oknx in labelpos:
                    df[f"r_{oknx}_neg", dga] = df[f"r_{oknx}", dga] - df[labelpos[oknx], dga]
            
            # fig, axs = plt.subplots(figsize=(10, 6), tight_layout=True)
            
            W = 1
            ticks_label = []
            ticks_pos = []


            # if _range and self.config.sps == 3600 and f'{self.config.pcap_id}' == '54':
            #     df = df.loc[770:780]
            
            ymax = 0
            i = 0
            for slotnum, row in df.iterrows():
                slotnum = cast(int, slotnum)
                i += 1
                if maxslots and i > maxslots: break
                ticks_label.append(slotnum)
                ticks_pos.append(slotnum * 4*W)
                for enu, oknx in enumerate(['ok', 'nx']):
                    try:
                        neg, pos = row[(f"r_{oknx}_neg", 2)], row[(labelpos[oknx], 2)]
                        neg = cast(int, neg)
                        pos = cast(int, pos)
                        ymax = (neg+pos) if ymax < (neg+pos) else ymax
                    except:
                        neg = 0
                        pos = 0
                        pass
                    rects = axs.bar(slotnum * 4*W + [-W/2, W/2][enu], neg, color=mpcolors.to_rgba("blue", [1,0.2][enu]))
                    rects = axs.bar(slotnum * 4*W + [-W/2, W/2][enu], pos, bottom=neg, color=mpcolors.to_rgba("red", [1,0.2][enu]))
                    if (pos+neg) > 0:
                        axs.bar_label(rects, labels=[pos], padding=10)
                    axs.bar_label(rects, labels=[neg], padding=0)
                    pass

                pass
            
            axs.set_xticks(ticks_pos, ticks_label)
            # axs.set_ylim(0,5000)
            axs.set_ylim(0,ymax + ymax*0.10)

            txt = str(self.config)
            txt += f"\nslotnum max: {df.index.max()}"
            fig.text(.5, -.1, txt, ha='center')
            # fig.savefig(f"tmp/{fig.__hash__()}", bbox_inches="tight")
            
            buffer = BytesIO()
            fig.savefig(buffer, format='svg', bbox_inches="tight")
            axs.set_title(title)
            buffer.seek(0)

            # Get the SVG image as a string
            svg_image = buffer.getvalue().decode('utf-8')
            
            with open('tmp/svg.txt', 'w') as fp:
                fp.write(svg_image)

            dfs.append((df))
            pass
        return fig, dfs
