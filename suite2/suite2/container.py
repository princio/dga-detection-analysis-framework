import sys
from logging import basicConfig, DEBUG

from dependency_injector import containers, providers
from dependency_injector.providers import Singleton

from .db import Database
from .message.message_service import MessageService

from .nn.nn_service import NNService
from .lstm.lstm_service import LSTMService
from .dn.dn_service import DNService
from .psl_list.psl_list_service import PSLListService
from .psltrie.psltrie_service import PSLTrieService
from .subprocess.subprocess_service import SubprocessService
from .pcap.pcap_service import PCAPService



class Suite2Container(containers.DeclarativeContainer):
    
    config = providers.Configuration()

    logging = providers.Resource(
        basicConfig,
        level=config.logging,
        stream=sys.stdout,
    )

    db = providers.Resource(
        Database,
        env=config.env,
        host=config.db.host,
        user=config.db.user,
        password=config.db.password,
        dbname=config.db.dbname,
        port=config.db.port,
    )
    lstm_service = Singleton(LSTMService)
    psl_list_service = Singleton(PSLListService, workdir=config.workdir)
    nn_service = Singleton(NNService, db=db)
    subprocess_service = Singleton(SubprocessService, binaries=config.binaries, workdir=config.workdir)
    psltrie_service = Singleton(PSLTrieService, subprocess_service=subprocess_service, psl_list_service=psl_list_service, binary=config.psltrie, workdir=config.workdir)
    message_service = Singleton(MessageService, db=db)
    dn_service = Singleton(DNService, db=db, nn_service=nn_service, lstm_service=lstm_service, psltrie_service=psltrie_service)
    pcap_service = Singleton(
        PCAPService,
        db = db,
        subprocess_service = subprocess_service,
        message_service = message_service,
        dn_service=dn_service
    )

    pass
