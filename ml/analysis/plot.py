
from dataclasses import dataclass
from io import BytesIO
from pathlib import Path
from typing import Any, List, Literal, Optional, Tuple, Union, cast

from matplotlib import pyplot as plt
from matplotlib.axes import Axes
import numpy as np
import pandas as pd
from defs import NN, Database, FetchConfig, OnlyFirsts, PacketType, pgshow
import matplotlib.colors as mpcolors

from slot2 import sql_healthy_dataset

# import matplotlib
# matplotlib.use('Qt5Agg')


@dataclass
class PNLabel:
    row: Any
    of: OnlyFirsts
    nn: NN
    pt: PacketType

    def _pt(self, pt: PacketType):
        self.pt = pt
        return self
    
    def lneg(self) -> str:
        return f"{'g' if self.of == OnlyFirsts.GLOBAL else ''}{self.pt}_neg{self.nn}".lower()
    
    def lpos(self) -> str:
        return f"{'g' if self.of == OnlyFirsts.GLOBAL else ''}{self.pt}_pos{self.nn}".lower()
    
    def ltot(self) -> str:
        return f"{'g' if self.of == OnlyFirsts.GLOBAL else ''}{self.pt}".lower()
    
    def neg(self) -> int:
        return self.row[self.ltot()] - self.row[self.lpos()] if self.row is not None  else 0
    
    def pos(self) -> int:
        return self.row[self.lpos()] if self.row is not None else 0
    
    def tot(self) -> int:
        return self.row[self.ltot()] if self.row is not None else 0


@dataclass
class DataConfig:
    pt: PacketType
    of: OnlyFirsts
    nn: NN
    only: Optional[Literal['pos','neg']]
    distinct_nx: bool
    maxslots: Optional[int]
    pcap_offset: int
    rolled_slots: Optional[int]

    def clone(self):
        return DataConfig(
            self.pt,
            self.of,
            self.nn,
            self.only,
            self.distinct_nx,
            self.maxslots,
            self.pcap_offset,
            self.rolled_slots
        )
    pass

@dataclass
class PlotConfig:
    figsize: Tuple[int, int]
    print_every: int = 1
    fullW: Optional[float] = None
    ylabel: str = ''
    print_barlabel: bool = True
    print_ticks_updown: bool = False
    logy : bool =True
    pass

@dataclass
class AxesConfig:
    fetch: FetchConfig
    data: DataConfig
    plot: PlotConfig
    pass

class Plotter:
    def __init__(self, dir: Path, db: Database):
        self.db = db
        self.dir = dir
        pass

    def get_df(self, config: FetchConfig):
        fpath = self.dir.joinpath(f'{config.__hash__()}.csv')
        if Path(fpath).exists():
            df = pd.read_csv(fpath)
        else:
            with self.db.engine.connect() as conn:
                df = pd.read_sql(sql_healthy_dataset(config), conn)
                df.to_csv(fpath)
            pass

        # print(config.pcap_id, config.pcap_offset, df.shape[0], df.slotnum.max())
        # if config.pcap_id == 57:
        #     pgshow(df)
        return df

    def axes(self, ax: Axes, ac: AxesConfig):
        df = self.get_df(ac.fetch)
        dc = ac.data
        pc = ac.plot

        negcolor = (127/255, 127/255, 255/255, 1)
        poscolor = (255/255, 127/255, 127/255, 1)

        upper_slot = df.slotnum.max()
        if upper_slot < 10:
            ac.plot.print_every = 1
        elif upper_slot < 50:
            ac.plot.print_every = 5
        elif upper_slot < 200:
            ac.plot.print_every = 10
        elif upper_slot < 500:
            ac.plot.print_every = 25
        elif upper_slot < 1000:
            ac.plot.print_every = 50

        W = 1
        ticks_label = []
        ticks_pos = []

        is_verbose = (dc.maxslots is None) and upper_slot > 20
        ymax = 0
        fullW = pc.fullW if pc.fullW else (W * (1 if is_verbose else 2))
        t = PNLabel(None, dc.of, dc.nn, dc.pt)

        if ac.data.rolled_slots:
            df = df[[t.lpos(), t.ltot()]].rolling(ac.data.rolled_slots, center=True).sum().fillna(0)
        values = []
        i = 0
        ln = 0
        if dc.pcap_offset:
            df = df.iloc[dc.pcap_offset:]
        for i in range(int(upper_slot+1)):
            t.row = None
            slotnum = None
            try:
                t.row = df.loc[i]
                slotnum = i
            except:
                pass
            i += 1

            slotnum = cast(int, slotnum)
            if dc.maxslots and i > dc.maxslots: break

            text_toprint = not is_verbose or (i == 1 or i % pc.print_every == 0)

            ln += int(text_toprint)


            if text_toprint:
                ticks_pos.append(i * fullW)
                if pc.print_ticks_updown:
                    ticks_label.append((f'\n{i}' if ln % 2 == 0 else f'{i}') if text_toprint else '')
                else:
                    ticks_label.append(f'{i}')

            x = i * fullW

            bars = []
            negs = []
            poss = []
            if not dc.only or dc.only == 'neg':
                if dc.distinct_nx:
                    negs.append((t._pt(PacketType.OK).neg(), negcolor, t))
                    negs.append((t._pt(PacketType.NX).neg(), negcolor, t))
                else:
                    negs.append((t.neg(), negcolor, t))
                    pass
            if dc.only and dc.only == 'pos':
                poss.append((t._pt(PacketType.OK).pos(), poscolor, t))
                poss.append((t._pt(PacketType.NX).pos(), poscolor, t))
            else:
                poss.append((t.pos(), poscolor, t))
                pass
            bars = poss + negs if sum([p[0] for p in poss]) > sum([n[0] for n in negs]) else negs + poss

            bottom = 0
            _row_ = [x]
            
            for bar in bars:
                _row_.append(bar[0])
                rects = ax.bar(x, bar[0], width=W, bottom=0, color=mpcolors.to_rgba(bar[1]))
                bottom += bar[0]
                if pc.print_barlabel and text_toprint:
                    ax.bar_label(rects, labels=[bar[0]], label_type='edge', padding=2,  fontsize='x-small')
                pass
            ymax = ymax if ymax > bottom else bottom
            values.append(_row_)
            pass

        ax.set_xticks(ticks_pos, ticks_label, fontsize='x-small')
        ax.tick_params(axis='x', which='both', pad=-0.01)

        if pc.logy:
            ax.set_yscale('log')
            ax.tick_params(axis='y', which='both', labelsize='x-small')
            ax.grid(True, axis='y', alpha=0.2, color='black')
            # ax.set_axisbelow(True)
        else:
            ax.set_ylim(0, ymax + ymax*0.10)

        cols = ['slotnum']
        if not dc.only or dc.only == 'neg':
            if dc.distinct_nx:
                cols.append(t._pt(PacketType.OK).lneg())
                cols.append(t._pt(PacketType.NX).lneg())
            else:
                cols.append(t.lneg())
        if not dc.only or dc.only == 'pos':
            if dc.distinct_nx:
                cols.append(t._pt(PacketType.OK).lpos())
                cols.append(t._pt(PacketType.NX).lpos())
            else:
                cols.append(t.lpos())

        return pd.DataFrame(values, columns=cols).set_index('slotnum')
    
    def rowpt_colsource(self, matrix: np.ndarray, sharey: Optional[Literal['row','col']]):
        rows, cols = matrix.shape
        fig, axes = plt.subplots(rows, cols, sharex=False)
        axes = cast(np.ndarray, axes)
        if rows == 1:
            axes = cast(List[List[Axes]], [[axis for axis in axes]])
        if cols == 1:
            axes = cast(List[List[Axes]], [[axis] for axis in axes])
        
        dfs = []
        for i in range(rows):
            dfs.append([])
            for j in range(cols):
                df = self.axes(axes[i][j], matrix[i,j])
                dfs[i].append(df)
                pass
            pass

        if sharey == 'row':
            for i in range(rows):
                _max = max([ df.sum(axis=1).max() for df in dfs[i] ])
                for j in range(cols):
                    axes[i][j].set_ylim(0, 1.5 * _max)
                    pass
        elif sharey == 'col':
            _maxs = [ max([ dfs[row][j].sum(axis=1).max() for row in range(rows) ]) for j in range(cols) ]
            for i in range(rows):
                for j in range(cols):
                    axes[i][j].set_ylim(1, 1.2 * _maxs[j])
                    # print(0, 1.5 * _maxs[j])
                    if matrix[i][j].plot.logy:
                        _ = [1,10,100,1000] + [10000] if _maxs[j] > 1000 else []
                        axes[i][j].set_yticks(_, [str(__) for __ in _])
                pass
        elif type(sharey) == tuple:
            _max = dfs[sharey[0]][sharey[1]].sum(axis=1).max()
            for i in range(rows):
                for j in range(cols):
                    axes[i][j].set_ylim(1, 1.2 * _max)
                    # print(0, 1.5 * _maxs[j])

                    if matrix[i][j].plot.logy:
                        _ = [10,100,1000] + [10000] if _max > 1000 else []
                        if j == 0:
                            axes[i][j].set_yticks(_, [str(__) for __ in _])
                        else:
                            axes[i][j].set_yticks(_, ['' for __ in _])
                pass
        for i in range(rows):
            axes[i][0].set_ylabel(f'''{matrix[i,j].plot.ylabel}''',
                                   labelpad=10, rotation='vertical', ha='center', va='center', fontsize='small', weight='bold')
            pass

        
        return fig, axes
    