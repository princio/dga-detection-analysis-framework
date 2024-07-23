import { Dn } from "./Dn";

import { WhitelistDn } from "./WhitelistDn";

import { WhitelistList_1 } from "./WhitelistList_1";

import { WhitelistList } from "./WhitelistList";

    export interface Whitelist {
    id: number;
    name: string | null;
    year: number | null;
    count: number | null;
    whitelistDns: WhitelistDn[];
    whitelistLists: WhitelistList[];
    whitelistListS: WhitelistList_1[];
}
