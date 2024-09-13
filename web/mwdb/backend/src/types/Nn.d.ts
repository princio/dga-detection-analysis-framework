import { Dn } from "./Dn";

import { DnNn } from "./DnNn";

    export interface Nn {
    id: number;
    name: string;
    folds: number | null;
    epochs: number | null;
    fold: number | null;
    epoch: number | null;
    maxLenghtInput: number;
    units: number;
    directory: string;
    extractor:
    | "domain"
    | "any"
    | "nosfx"
    | "domain_sfx"
    | "icann"
    | "private"
    | "none"
    | "tld"
    | null;
    dnNns: DnNn[];
}
