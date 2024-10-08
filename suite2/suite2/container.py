import sys
from logging import basicConfig, DEBUG

from dependency_injector import containers, providers

from .db import Database
from .message.message_service import MessageService

from .nn.nn_service import NNService
from .lstm.lstm_service import LSTMService
from .dn.dn_service import DBDNService
from .psl_list.psl_list_service import PSLListService
from .psltrie.psltrie_service import PSLTrieService
from .pcap.dns_parse_service import DNSParseService
from .pcap.tcpdump_service import TCPDumpService
from .pcap.tshark_service import TSharkService
from .pcap.pcap_service import PCAPService



class Suite2Container(containers.DeclarativeContainer):
    
    config = providers.Configuration()

    logging = providers.Resource(
        basicConfig,
        level=DEBUG,
        stream=sys.stdout,
    )

    db = providers.Resource(
        Database,
        host=config.db.host,
        user=config.db.user,
        password=config.db.password,
        dbname=config.db.dbname,
        port=config.db.port,
    )

    lstm_service = providers.Singleton(LSTMService)
    psl_list_service = providers.Singleton(PSLListService, workdir=config.workdir)
    psltrie_service = providers.Singleton(PSLTrieService, psl_list_service=psl_list_service, binary=config.psltrie, workdir=config.workdir)

    nn_service = providers.Singleton(NNService, db=db)
    dbdn_service = providers.Singleton(DBDNService, db=db, nn_service=nn_service, lstm_service=lstm_service, psltrie_service=psltrie_service)

    message_service = providers.Singleton(MessageService, db=db, dbdn_service=dbdn_service)

    tcpdump_service = providers.Singleton(TCPDumpService, binary=config.tcpdump, workdir=config.workdir)
    tshark_service = providers.Singleton(TSharkService, binary=config.tshark, workdir=config.workdir)
    dns_parse_service = providers.Singleton(DNSParseService, binary=config.dns_parse, workdir=config.workdir)

    pcap_service = providers.Singleton(
        PCAPService,
        db = db,
        tcpdump_service = tcpdump_service,
        tshark_service = tshark_service,
        dns_parse_service = dns_parse_service,
        message_service = message_service,
    )
    pass
