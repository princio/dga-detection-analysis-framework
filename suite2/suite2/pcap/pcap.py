


class PCAP:
    def __init__(self, id, name, sha256, infected, dataset, year):
        self.id = id
        self.name = name
        self.sha256 = sha256
        self.infected = infected
        self.dataset = dataset
        self.year = year
        return