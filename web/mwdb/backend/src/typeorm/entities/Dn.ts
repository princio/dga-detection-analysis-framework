import {
  Column,
  Entity,
  Index,
  OneToMany,
  PrimaryGeneratedColumn,
} from "typeorm";
import { DnNn } from "./DnNn";
import { WhitelistDn } from "./WhitelistDn";

@Index("dn_unique", ["dn"], { unique: true })
@Index("dn2_pkey", ["id"], { unique: true })
@Entity("dn", { schema: "public" })
export class Dn {
  @PrimaryGeneratedColumn({ type: "bigint", name: "id" })
  id: string;

  @Column("character varying", { name: "dn", unique: true, length: 256 })
  dn: string;

  @Column("character varying", { name: "bdn", nullable: true, length: 256 })
  bdn: string | null;

  @Column("text", { name: "tld", nullable: true })
  tld: string | null;

  @Column("text", { name: "icann", nullable: true })
  icann: string | null;

  @Column("text", { name: "private", nullable: true })
  private: string | null;

  @Column("boolean", { name: "dyndns", default: () => "false" })
  dyndns: boolean;

  @Column("integer", { name: "psltrie_rcode", nullable: true })
  psltrieRcode: number | null;

  @OneToMany(() => DnNn, (dnNn) => dnNn.dn)
  dnNns: DnNn[];

  @OneToMany(() => WhitelistDn, (whitelistDn) => whitelistDn.dn)
  whitelistDns: WhitelistDn[];
}
