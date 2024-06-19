
from IPython.display import display, Markdown, Latex, HTML
import enum, os
import quantiphy as qq

def is_latex():
    return "TO_LATEX" in os.environ

def set_latex():
    os.environ["TO_LATEX"] = "yes"

def unset_latex():
    del os.environ["TO_LATEX"]


def dm(x):
    display(Markdown(x))
    return

def pp(rows):
    if isinstance(rows, list):
        dm("\n".join(rows))
    else:
        dm(rows)
    return

def ptime(v, unit="hour"):
        return qq.Quantity(v * 60, units="min").render(prec=1)

class Label(enum.Enum):
    def __init__(self, *args, **kwargs):
        # super(Tables, self).__init__(*args)
        pass
    
    def label(self):
        if is_latex():
            return f"@@label@:{self.prefix}:{self._}:@"
        else:
            return f"[{self._}]"
        pass

    def ref(self):
        if is_latex():
            return f"@@ref@:{self.prefix}:{self._}:@"
        else:
            return f"_[{self._}]_"
        pass

class AC(enum.Enum):
    NIC = "Not-Infected Capture"
    NICs = "Not-Infected Captures"
    IC = "Infected Capture"
    ICs = "Infected Captures"
    DNS = "Domain Name System"
    CC = "Command&Control"
    DGA = "Domain Name Generation"
    MCFP = "Malware Capture FaciICty Project"
    PCAP = "Packet CAPture"
    CSV = "Comma Separeted Values"
    BIBO = "Bibo"

    def __str__(self):
        if is_latex():
            return f"@@ac@:{self.name}:@"
        else:
            return f"_{self.name}_"
    pass

IC = AC.IC #"infected"
NIC = AC.NIC #"not-infected"

class Cites(enum.Enum):
    CTU_SME_11 = 0
    DOS_DONTS = 1
    STSPH = 2
    
    def __init__(self, *args, **kwargs):
        self._ = self.name.replace("_", "-")
        pass

    def __str__(self):
        if is_latex():
            return f"@@cite@:{self._}:@"
        else:
            return f"_[{self._}]_"
    pass

class Tables(Label):
    TOTAL_Q = "TOTAL_Q"
    AVERAGE_Q = "AVERAGE_Q"
    Q_PER_S = "Q_PER_S"
    Q_PER_S_1D = "Q_PER_S_1D"
    DURATION = "DURATION"
    SLOTS = "SLOTS"
    SLOTS_U = "SLOTS_U"
    TOTALS = "TOTALS"

    def __init__(self, *args, **kwargs):
        self.prefix = "tab"
        self._ = self.name.lower().replace("_", "-")
        pass
    pass

class Figures(Label):
    Q_PER_S = 0
    DURATION = 1
    SLOTS = 2
    SLOTS_PCAP = 3
    SLOTS_DGA = 4
    SLOTS_PCAP_U = 5
    SLOTS_DGA_U = 6
    SLOTS_U = 7

    def __init__(self, *args, **kwargs):
        self.prefix = "fig"
        self._ = self.name.lower().replace("_", "-")
        pass
    pass

class Figure:
    def __init__(self, fig, axs, label, caption):
        self.fig = fig
        self.axs = axs
        self.label = label
        self.caption = caption
        self.ycaption = -0.1
        pass

    def show(self):
        fname = f"{self.label._}.{'pdf' if is_latex() else 'svg'}"
        if not is_latex():
            self.fig.suptitle(self.label.label())
            self.fig.text(.5, self.ycaption, self.caption, ha='center')
            # self.fig.show()
            self.fig.savefig(fname, bbox_inches="tight")
            dm(f"![]({fname} \"Example\")")
        else:
            self.fig.savefig(fname)
            dm(f"@@begin@:figure:@")
            dm(f"@@centering")
            dm(f"@@includegraphics@qwidth=@@textwidthq@ @:{fname}:@")
            dm(f"@@caption@:{self.caption}:@")
            dm(self.label.label())
            dm(f"@@end@:figure:@")
            pass
        pass

class Table:
    def __init__(self, df, label, caption):
        self.df = df.copy()
        self.label = label
        self.caption = caption
        self.columns = None
        pass

    def rename(self, columns):
        self.columns = columns

    def show(self, width=None):
        tmp = self.df.copy()
        if self.columns:
            self.df.rename(columns=self.columns, inplace=True)
        if is_latex():
            self.show_latex()
        else:
            self.show_md(width)
        self.df = tmp
        return
    
    def show_md(self, width):
        s = self.df.style
        caption = f"<i>{self.label.label()}</i>: {self.caption}"
        s = (
            s.set_caption(caption)
            .set_table_styles([
                 dict(selector="caption", props="caption-side: bottom; font-size:1em;")
             ], overwrite=False)
        )
        if width:
            s = s.set_table_attributes(f'style="table-layout: auto; min-width: {width};"')
            pass
        display(HTML(f'<div style="display: flex; justify-content: center;">{s.to_html()}</div>'))
        return

    def show_latex(self):
        dm("@@begin@:table:@")
        dm(f"@@centering")
        dm(self.df.to_markdown())
        dm(f"@@caption@:{self.caption}:@")
        dm(self.label.label())
        dm("@@end@:table:@")
        return
    pass