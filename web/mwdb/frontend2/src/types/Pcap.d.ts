import { Dataset } from "./Dataset";

import { Malware } from "./Malware";

    export interface Pcap {
    id: string;
    name: string;
    sha256: string;
    infected: boolean;
    dataset: string;
    day: number | null;
    days: number | null;
    terminal: string | null;
    qr: number;
    q: number;
    r: number;
    u: number;
    fnreqMax: string;
    dgaRatio: number;
    batches: object | null;
    year: number | null;
    duration: number | null;
    mcfpId: string | null;
    mcfpType: 'BOTNET' | 'MIXED' | 'NORMAL' | 'OTHER' | null;
    mcfpInfo: object | null;
    selected: boolean;
    translation: number;
    first_message_time_s: number;
    datasets: Dataset[];
    malware: Malware;
}
