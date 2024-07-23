
    export interface WhitelistDn {
    id: string;
    dnId: string | null;
    whitelistId: string;
    rankDn: number | null;
    rankBdn: number | null;
    dn: Dn;
    whitelist: Whitelist;
}
