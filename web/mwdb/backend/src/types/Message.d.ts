import { Malware } from "./Malware";

    export interface Message {
    id: string;
    pcapId: string;
    timeS: number;
    fn: string;
    fnReq: string;
    dnId: string;
    qcode: number;
    isR: boolean;
    rcode: number | null;
    server: string | null;
    answer: string | null;
    timeSTranslated: number | null;
    tmpMinTime: number | null;
    pcapInfected: boolean | null;
    pcapMalwareType: "non-dga" | "dga" | "no-malware" | null;
}
