

import itertools
from optparse import Option
from typing import Dict, List
from matplotlib import pyplot as plt
import numpy as np
from typing import Optional
from defs import NN, FetchConfig, OnlyFirsts, PacketType
from plot import AxesConfig, DataConfig, PlotConfig, Plotter


def plot_rowsource_colposneg(plotter: Plotter, pcaps, pcaps_offset: Optional[Dict[None | int, int]], maxslots = None, rolled = None):
    sources = (
        [ { 'mw_name': 'healthy', 'pcap_id': None }]
        + [ { 'mw_name': mw_name, 'pcap_id': pcap_id } for mw_name, pcap_id in pcaps ]
    )

    pts = [PacketType.Q, PacketType.NX] #, PacketType.NX]
    datas = [DataConfig(*(list(combo))) for combo in list(itertools.product( # type: ignore
        pts, 
        [OnlyFirsts.GLOBAL],
        [NN.NONE],
        [None],
        [False],
        [maxslots],
        [0],
        [rolled]))]

    matrix = np.ndarray((len(sources), len(datas)), dtype=object)
    for s, source in enumerate(sources):
        for d, data in enumerate(datas):
            if pcaps_offset:
                data = data.clone()
                data.pcap_offset = pcaps_offset[int(source['pcap_id']) if source['pcap_id'] else None]
                pass
            fc = FetchConfig(3600 * 8, NN.NONE, 0.5, pcap_id=source['pcap_id'])
            pc = PlotConfig((40,30), 25, 1, f'{source['mw_name']}', False, False, logy=True)
            matrix[s, d] = AxesConfig(fc, data, pc)
            pass
        pass

    return plotter.rowpt_colsource(matrix, sharey=(0,0)) # type: ignore


def plot_pcaps(plotter: Plotter, pcaps, rolled = None):
    for _, pcap in pcaps.iterrows():
        plots = [
            DataConfig(*(list(combo))) for combo in list(itertools.product( # type: ignore
                [PacketType.Q, PacketType.R, PacketType.NX],
                [OnlyFirsts.GLOBAL],
                [NN.NONE],
                ['neg', 'pos'],
                [False],
                [None],
                [0],
                [rolled]
            )
        )]

        fetch = FetchConfig(3600, NN.NONE, 0.5, pcap_id=pcap['pcap_id'])
        matrix = np.ndarray((len(plots), 1), dtype=object)
        for i, plot in enumerate(plots):
            matrix[i, 0] = (fetch, plot, PlotConfig((40,30), 10, 1, pcap['mw_name']))

        fig, axes = plotter.rowpt_colsource(matrix, sharey='col')

        plt.subplots_adjust(wspace=0, hspace=0)
        fig.set_size_inches(25, 12)
        # fig.savefig(f"images/{pcap['mw_name']}.svg", dpi=1000)
        fig.savefig(f"images/{pcap['pcap_id']}_{pcap['mw_name']}.svg", format='svg', bbox_inches="tight")

        print(f"images/{pcap['pcap_id']}_{pcap['mw_name']}.png")
        pass


def plot_pcaps_withoffset(plotter: Plotter, pcaps):
    pcap_offsets_s = [{
        None: 0,
        45: 0,
        54: 375,
        56: 0,
        57: 33,
        52: 0,
        42: 0,
        55: 0,
        46: 380,
        41: 0,
        58: 17,
        37: 0
    },
    {
        None: 0,
        45: 0,
        54: 770,
        56: 0,
        57: 0,
        52: 0,
        42: 0,
        55: 0,
        46: 0,
        41: 0,
        58: 0,
        37: 0
    }]
    for po in pcap_offsets_s:
        fig, axs = plot_rowsource_colposneg(plotter, pcaps, po)
        fig.set_size_inches(20,12)
        fig.tight_layout()
        plt.subplots_adjust(wspace=0, hspace=0)
        plt.show()