import { Whitelist } from "./Whitelist";

import { Nn } from "./Nn";

import { WhitelistDn } from "./WhitelistDn";

import { DnNn } from "./DnNn";

    export interface Dn {
    id: string;
    dn: string;
    bdn: string | null;
    tld: string | null;
    icann: string | null;
    private: string | null;
    dyndns: boolean;
    psltrieRcode: number | null;
    dnNns: DnNn[];
    whitelistDns: WhitelistDn[];
}
